#include "main.h"

#include <lwip/dhcp.h>
#include <lwip/tcpip.h>

#include <string.h>

static bool ip_is_up = false;

void ip_link_up_cb(void *p)
{
  dhcp_start((struct netif*)p);

  ip_is_up = true;

  screen_draw_ethernet_up();
}

void ip_link_down_cb(void *p)
{
  dhcp_stop((struct netif*)p);

  ip_is_up = false;

  screen_draw_ethernet_down();
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

void _udp_tx(void *arg)
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

  uint8_t *_udp_pbuf_payload_ptr;

  _udp_pbuf_ptr = pbuf_alloc(PBUF_TRANSPORT, (4+1+8), PBUF_REF);
  _udp_pbuf_payload_ptr = (uint8_t *)_udp_pbuf_ptr->payload;

  while(true)
  {
    chMtxLock(&udp_tx_queue.mutex);

    while(!udp_tx_queue.waiting)
    {
      chCondWait(&udp_tx_queue.condition);
    }

    *(uint32_t *)(&_udp_pbuf_payload_ptr[0]) = udp_tx_queue.canFrame.SID;
    _udp_pbuf_payload_ptr[4] = udp_tx_queue.canFrame.DLC;
    memcpy(&(_udp_pbuf_payload_ptr[5]), udp_tx_queue.canFrame.data8, 8);

    udp_tx_queue.waiting = false;

    chMtxUnlock(&udp_tx_queue.mutex);

    if(!ip_is_up)
    {
      continue;
    }

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


static struct udp_pcb *_udp_rx_pcb_ptr = NULL;

typedef struct {
  bool waiting;
  uint8_t commandFrame[64];
  uint32_t commandLength;
  mutex_t mutex;
  condition_variable_t condition;
} udp_rx_queue_t;

udp_rx_queue_t udp_rx_queue = {
  .waiting = false,
  .mutex = _MUTEX_DATA(udp_rx_queue.mutex),
  .condition = _CONDVAR_DATA(udp_rx_queue.condition)
};



static const uint8_t udp_rx_magic[4] = { 0x42, 0x45, 0x50, 0x49 }; /* BEPI */
static void _udp_rx_cb(void *arg, struct udp_pcb *upcb, struct pbuf *p, const struct ip4_addr *addr, u16_t port)
{
  /* Length Limits */
  if(p->len < 5 || p-> len > 64)
  {
    return;
  }

  /* Check Magic string */
  if(memcmp(p->payload, udp_rx_magic, 4) != 0)
  {
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

static void _udp_rx_init(void *arg)
{
  (void)arg;

  _udp_rx_pcb_ptr = udp_new();
  udp_bind(_udp_rx_pcb_ptr, IP_ADDR_ANY, 777);
  udp_recv(_udp_rx_pcb_ptr, &_udp_rx_cb, (void *)0);
}

extern uint32_t debug_number;

static uint8_t udp_rx_commandFrame[64];
static uint32_t udp_rx_commandLength;
THD_FUNCTION(udp_rx_service_thread, arg)
{
  (void)arg;

  /* Set up UDP socket with callback */
  tcpip_callback(_udp_rx_init, NULL);

  /* Wait for callback to be fired */
  while(true)
  {
    chMtxLock(&udp_rx_queue.mutex);

    while(!udp_rx_queue.waiting)
    {
      chCondWait(&udp_rx_queue.condition);
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
        txmsg.data16[0] = *(uint16_t *)(&udp_rx_commandFrame[7]);

        canTransmitTimeout(&CAND1, CAN_ANY_MAILBOX, &txmsg, TIME_IMMEDIATE);
      }
    }
  }
}