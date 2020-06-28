#include "main.h"

#include <lwip/dhcp.h>
#include <lwip/tcpip.h>
#include <lwip/apps/sntp.h>

#include <string.h>

static struct netif* _ip_netif_ptr = NULL;

static bool _ip_link_is_up = false;

// eshail.batc.org.uk - 185.83.169.27
static const ip_addr_t ipaddr_ntp_goonhilly = {
  .addr = (27 << 24) | (169 << 16) | (83 << 8) | (185)
};

uint32_t app_ip_link_status(void)
{
  if(_ip_link_is_up && _ip_netif_ptr != NULL)
  {
    if(netif_dhcp_data(_ip_netif_ptr)->state == 10) // DHCP_STATE_BOUND = 10
    {
      return APP_IP_LINK_STATUS_BOUND;
    }
    else
    {
      return APP_IP_LINK_STATUS_UPBUTNOIP;
    }
  }
  else
  {
    return APP_IP_LINK_STATUS_DOWN;
  }
}

void ip_link_up_cb(void *p)
{
  _ip_netif_ptr = (struct netif*)p;

  dhcp_start(_ip_netif_ptr);

  _ip_link_is_up = true;
}

void ip_link_down_cb(void *p)
{
  _ip_netif_ptr = (struct netif*)p;

  dhcp_stop(_ip_netif_ptr);

  _ip_link_is_up = false;
}

static void _user_ip_sntp_start(void *arg)
{
  (void)arg;

  /* Set Primary server (can be overwritten by DNS) */
  sntp_setserver(0, &ipaddr_ntp_goonhilly);
  /* Also set as fallback after DHCP-populated servers */
  sntp_setserver(2, &ipaddr_ntp_goonhilly);
  sntp_init();
}

static void _user_ip_sntp_stop(void *arg)
{
  (void)arg;
  
  sntp_stop();
}

static bool _user_ip_services_running = false;
THD_FUNCTION(user_ip_services_thread, arg)
{
  (void)arg;

  chRegSetThreadName("user_ip_services");

  while(true)
  {
    if(_ip_netif_ptr != NULL)
    {
      if(!_user_ip_services_running
          && app_ip_link_status() == APP_IP_LINK_STATUS_BOUND)
      {
        /* Start Services */
        tcpip_callback(_user_ip_sntp_start, NULL);

        _user_ip_services_running = true;
      }
      else if(_user_ip_services_running
          && app_ip_link_status() != APP_IP_LINK_STATUS_BOUND)
      {
        /* Stop Services */
        tcpip_callback(_user_ip_sntp_stop, NULL);

        _user_ip_services_running = false;
      }
    }
    watchdog_feed(WATCHDOG_DOG_USERIPSRVS);
    chThdSleepMilliseconds(100);
  }
};

uint32_t app_ip_service_sntp_status(void)
{
  uint32_t i;
  uint8_t result = 0;
  for (i = 0; i < SNTP_MAX_SERVERS; i++)
  {
    result |= (sntp_getreachability(i) & 0x3);
  }
  if((result & 0x1) > 0 && !app_time_syncing)
  {
    return APP_IP_SERVICE_SNTP_STATUS_SYNCED;
  }
  else if((result & 0x2) > 0)
  {
    return APP_IP_SERVICE_SNTP_STATUS_POLLING;
  }
  else
  {
    return APP_IP_SERVICE_SNTP_STATUS_DOWN;
  }
}

typedef struct {
  bool waiting;
  CANRxFrame canFrame;
  mutex_t mutex;
  condition_variable_t condition;
} udp_tx_queue_t;

static udp_tx_queue_t udp_tx_queue =
{
  .waiting = false,
  .mutex = _MUTEX_DATA(udp_tx_queue.mutex),
  .condition = _CONDVAR_DATA(udp_tx_queue.condition)
};

static struct udp_pcb *_udp_tx_pcb_ptr = NULL;
static struct pbuf *_udp_pbuf_ptr;

static void _udp_tx(void *arg)
{
  (void)arg;

  if(_udp_tx_pcb_ptr == NULL)
  {
    _udp_tx_pcb_ptr = udp_new();
  }

  udp_sendto(_udp_tx_pcb_ptr, _udp_pbuf_ptr, IP_ADDR_BROADCAST, 11234);
}

THD_FUNCTION(udp_tx_service_thread, arg)
{
  (void)arg;

  chRegSetThreadName("udp_tx_service");

  uint8_t *_udp_pbuf_payload_ptr;
  RTCDateTime _datetime;

  _udp_pbuf_ptr = pbuf_alloc(PBUF_TRANSPORT, (4+4+1+8), PBUF_REF);
  _udp_pbuf_payload_ptr = (uint8_t *)_udp_pbuf_ptr->payload;

  while(true)
  {
    watchdog_feed(WATCHDOG_DOG_UDP_TX);
    chMtxLock(&udp_tx_queue.mutex);

    while(!udp_tx_queue.waiting)
    {
      watchdog_feed(WATCHDOG_DOG_UDP_TX);
      
      if(chCondWaitTimeout(&udp_tx_queue.condition, TIME_MS2I(100)) == MSG_TIMEOUT)
      {
        /* Re-acquire Mutex */
        chMtxLock(&udp_tx_queue.mutex);
      }
    }

    *(uint32_t *)(&_udp_pbuf_payload_ptr[4]) = udp_tx_queue.canFrame.SID;
    _udp_pbuf_payload_ptr[8] = udp_tx_queue.canFrame.DLC;
    memcpy(&(_udp_pbuf_payload_ptr[9]), udp_tx_queue.canFrame.data8, 8);

    udp_tx_queue.waiting = false;
    chMtxUnlock(&udp_tx_queue.mutex);

    if(!_ip_link_is_up)
    {
      /* Discard packet */
      continue;
    }

    rtcGetTime(&RTCD1, &_datetime); 
    *(uint32_t *)(&_udp_pbuf_payload_ptr[0]) = _datetime.millisecond;

    tcpip_callback(_udp_tx, NULL);
  }
}

void ip_send_canmessage(CANRxFrame *message_ptr)
{
  chMtxLock(&udp_tx_queue.mutex);

  memcpy(&(udp_tx_queue.canFrame), message_ptr, sizeof(CANRxFrame));
  udp_tx_queue.waiting = true;

  chCondSignal(&udp_tx_queue.condition);
  chMtxUnlock(&udp_tx_queue.mutex);
}



typedef struct {
  bool waiting;
  uint8_t commandFrame[64];
  uint32_t commandLength;
  mutex_t mutex;
  condition_variable_t condition;
} udp_rx_queue_t;

static udp_rx_queue_t udp_rx_queue = {
  .waiting = false,
  .mutex = _MUTEX_DATA(udp_rx_queue.mutex),
  .condition = _CONDVAR_DATA(udp_rx_queue.condition)
};

static const uint8_t udp_rx_magic[4] = { 0x42, 0x45, 0x50, 0x49 }; /* BEPI */
static void _udp_rx_cb(void *arg, struct udp_pcb *upcb, struct pbuf *p, const struct ip4_addr *addr, u16_t port)
{
  (void)arg;
  (void)upcb;
  (void)addr;
  (void)port;
  
  if(p == NULL)
  {
    return;
  }

  /* Length Limits */
  if(p->len < 5 || p->len > 64)
  {
    pbuf_free(p);
    return;
  }

  /* Check Magic string */
  if(memcmp(p->payload, udp_rx_magic, 4) != 0)
  {
    pbuf_free(p);
    return;
  }

  chMtxLock(&udp_rx_queue.mutex);

  memcpy(udp_rx_queue.commandFrame, p->payload, p->len);
  udp_rx_queue.commandLength = p->len;
  udp_rx_queue.waiting = true;

  chCondSignal(&udp_rx_queue.condition);
  chMtxUnlock(&udp_rx_queue.mutex);

  pbuf_free(p);
}

static struct udp_pcb *_udp_rx_pcb_ptr;
static void _udp_rx_init(void *arg)
{
  (void)arg;

  _udp_rx_pcb_ptr = udp_new();
  udp_bind(_udp_rx_pcb_ptr, IP_ADDR_ANY, 777);
  udp_recv(_udp_rx_pcb_ptr, &_udp_rx_cb, (void *)0);
}

static uint8_t udp_rx_commandFrame[64];
static uint32_t udp_rx_commandLength;
THD_FUNCTION(udp_rx_service_thread, arg)
{
  (void)arg;

  chRegSetThreadName("udp_rx_service");

  /* Set up UDP socket with callback */
  tcpip_callback(_udp_rx_init, NULL);

  /* Wait for callback to be fired */
  while(true)
  {
    watchdog_feed(WATCHDOG_DOG_UDP_RX);
    chMtxLock(&udp_rx_queue.mutex);

    while(!udp_rx_queue.waiting)
    {
      watchdog_feed(WATCHDOG_DOG_UDP_RX);

      if(chCondWaitTimeout(&udp_rx_queue.condition, TIME_MS2I(100)) == MSG_TIMEOUT)
      {
        /* Re-acquire Mutex */
        chMtxLock(&udp_rx_queue.mutex);
      }
    }

    memcpy(udp_rx_commandFrame, udp_rx_queue.commandFrame, udp_rx_queue.commandLength);
    udp_rx_commandLength = udp_rx_queue.commandLength;
    udp_rx_queue.waiting = false;

    chMtxUnlock(&udp_rx_queue.mutex);

    /* Process Command */
    if(udp_rx_commandFrame[4] == 'S')
    {
      /* Motor Speed Command */
      if(udp_rx_commandFrame[5] == 'A')
      {
        /* Azimuth Axis */
        CANTxFrame txmsg;

        txmsg.IDE = CAN_IDE_STD;
        txmsg.SID = 0x2B;
        txmsg.RTR = CAN_RTR_DATA;
        txmsg.DLC = 2;
        txmsg.data16[0] = *(int16_t *)(&udp_rx_commandFrame[6]);

        canTransmitTimeout(&CAND1, CAN_ANY_MAILBOX, &txmsg, TIME_IMMEDIATE);
      }
    }
  }
}