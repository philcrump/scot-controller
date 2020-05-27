#include "main.h"

#include <math.h>

#include "lwipthread.h"

int32_t app_time_ms_correction = 0;

uint16_t azimuth_raw = 0x0000;
uint16_t elevation_raw = 0x0000;
uint8_t azimuth_fault_raw = 0x00;
uint8_t elevation_fault_raw = 0x00;
float azimuth_degrees = 0.0;
float elevation_degrees = 0.0;

float azimuth_demand_degrees = 0.0;
float elevation_demand_degrees = 0.0;

float azimuth_error_degrees = 0.0;
float elevation_error_degrees = 0.0;

static THD_WORKING_AREA(screen_service_wa, 2048);
static THD_WORKING_AREA(can_rx_service_wa, 128);
static THD_WORKING_AREA(udp_tx_service_wa, 128);
static THD_WORKING_AREA(udp_rx_service_wa, 128);
static THD_WORKING_AREA(user_ip_services_wa, 128);

static uint8_t _macAddress[] = {0xC2, 0xAF, 0x51, 0x03, 0xCF, 0x46};
static const lwipthread_opts_t lwip_opts = {
  .macaddress = _macAddress,
  .address = 0,
  .netmask = 0,
  .gateway = 0,
  .addrMode = NET_ADDRESS_DHCP,
  .ourHostName = "scot-controller",
  .link_up_cb = ip_link_up_cb,
  .link_down_cb = ip_link_down_cb
};

uint32_t stack_screen_free = 0;
uint32_t stack_can_rx_free = 0;


static uint32_t thread_stack(uint8_t *_wa, uint32_t _wa_size)
{
  uint32_t free;

  for(free = 0; free < _wa_size; free++)
  {
    if(((uint8_t *)_wa)[sizeof(thread_t) + free] != 0x55)
    {
      break;
    }
  }

  return free;
}


int main(void)
{
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /* Set up Screen */
  chThdCreateStatic(screen_service_wa, sizeof(screen_service_wa), NORMALPRIO, screen_service_thread, NULL);

  /* Set up CAN */
  chThdCreateStatic(can_rx_service_wa, sizeof(can_rx_service_wa), NORMALPRIO, can_rx_service_thread, NULL);

  /* Set up IP stack */
  lwipInit(&lwip_opts);

  /* Set up UDP TX */
  chThdCreateStatic(udp_tx_service_wa, sizeof(udp_tx_service_wa), NORMALPRIO, udp_tx_service_thread, NULL);

  /* Set up UDP RX */
  chThdCreateStatic(udp_rx_service_wa, sizeof(udp_rx_service_wa), NORMALPRIO, udp_rx_service_thread, NULL);

  /* Set up IP Services Thread */
  chThdCreateStatic(user_ip_services_wa, sizeof(user_ip_services_wa), NORMALPRIO, user_ip_services_thread, NULL);

  while(true)
  {
    stack_screen_free = thread_stack((uint8_t *)screen_service_wa, 2048);
    stack_can_rx_free = thread_stack((uint8_t *)can_rx_service_wa, 128);
    chThdSleepMilliseconds(500);
  }
}
