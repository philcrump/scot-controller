#include "main.h"

#include <string.h>
/*
  CAN_MCR_ABOM - Automatic Bus-Off Management
  CAN_MCR_AWUM - Automatic Wake-Up Management
  CAN_MCR_TXFP - Transmit FIFO Priority Mode (set = chronological, cleared = indentifier)

  CAN_BTR_LBKM - Loopback Mode (RX only sees TX)
  CAN_BTR_SILM - Silent Mode (TX doesn't go on the wire)
  CAN_BTR_SJW - Set Re-synchonisation Jump Width
  CAN_BTR_TS1 / CAN_BTR_TS2 - Set Time Segments
  CAN_BTR_BRP - Set time quantum prescaler
  Timing ( http://www.bittiming.can-wiki.info/ )
  STM32F746:
   - PLL = 216MHz
   - HPRE AHB Prescaler = 1, therefore AHB = 216MHz
   - PPRE1 APB1 Low Speed Prescaler = 4, therefore APB1 = 54MHz
   - PCLK = 54MHz, target baudrate = 1Mbps, therefore 54 clocks / bit
      tq = (_BRP+1) = 3 clocks
      Sync _SJW = 1 = 2*3 = 6 clocks clocks
      Segment 1 (sync <-> sample) _TS1 = 13x3 = 39 clocks
      Segment 2 (sample <-> transmission) _TS2 = 2x3 = 6 clocks
*/

static bool can_initialised = false;

static const CANConfig can_cfg = {
  .mcr = CAN_MCR_TXFP | CAN_MCR_ABOM,
  .btr = CAN_BTR_SJW(2) | CAN_BTR_TS2(2) | CAN_BTR_TS1(13) | CAN_BTR_BRP(2)
};

/*
static const CANFilter can_filter = {
  .filter = 1, // Number of filter bank to be programmed
  .mode = 0, // 0 = mask, 1 = list
  .scale = 0, // 0 = 16bits, 1 = 32bits
  .assignment = 0, // (must be set to 0)
  .register1 = 0x0010, // identifier 1
  .register2 = 0x07C0 // mask if mask mode, identifier2 if list mode
};
*/

void can_init(void)
{
  if(can_initialised)
  {
    return;
  }

  //canSTM32SetFilters(&CAND1, 1, 1, &can_filter);

  canStart(&CAND1, &can_cfg);

  can_initialised = true;
}

void can_send_reset(const uint16_t target_sid)
{
  CANTxFrame txmsg;

  txmsg.IDE = CAN_IDE_STD; // Identifier Type: Standard
  txmsg.SID = target_sid; // Standard Identifier Value (11bits)
  txmsg.RTR = CAN_RTR_DATA; // Frame Type
  txmsg.DLC = 5; // Data Length (max = 8)
  txmsg.data8[0] = 'R';
  txmsg.data8[1] = 'E';
  txmsg.data8[2] = 'S';
  txmsg.data8[3] = 'E';
  txmsg.data8[4] = 'T';

  canTransmitTimeout(&CAND1, CAN_ANY_MAILBOX, &txmsg, TIME_IMMEDIATE);
}

static void can_rx_process(CANRxFrame *message)
{
  if(message->RTR == CAN_RTR_DATA
    && message->IDE == CAN_IDE_STD)
  {
    if(message->SID == 0x010 && message->DLC == 3)
    {
      /* Elevation Resolver Position & Fault Message */
      elevation_raw = message->data16[0];
      elevation_fault_raw = message->data8[3];

      if(elevation_fault_raw != 0x00)
      {
        device_elresolver = DEVICE_FAULT;
      }
      else
      {
        device_elresolver = DEVICE_OK;
      }
    }
    else if(message->SID == 0x013 && message->DLC == 8)
    {
      /* Elevation Resolver Sysinfo Message */
    }
    else if(message->SID == 0x014 && message->DLC == 8)
    {
      /* Elevation Motor Info Message */
      uint32_t elevation_motorfault;
      uint32_t elevation_motorcontrol;

      elevation_pwm = message->data16[0];
      elevation_current = message->data16[2];
      el_limit_1 = (message->data8[7] >> 7) & 0x01;
      el_limit_2 = (message->data8[7] >> 6) & 0x01;
      elevation_motorfault = (message->data8[7] >> 5) & 0x01;
      elevation_motorcontrol = (message->data8[7] >> 4) & 0x01;

      /* Derive ESTOP status */
      if((el_limit_1 == 1) && (el_limit_2 == 1))
      {
        estop = ESTOP_HALT;
      }
      else
      {
        estop = ESTOP_OK;
      }

      /* Derive Device Status */
      if(elevation_motorfault == 0x01)
      {
        device_elmotor = DEVICE_FAULT;
      }
      else if(elevation_motorcontrol == 0x00)
      {
        /* Motor Control inhibited */
        device_elmotor = DEVICE_INACTIVE;
      }
      else
      {
        device_elmotor = DEVICE_OK;
      }
    }
    else if(message->SID == 0x020 && message->DLC == 3)
    {
      /* Azimuth Resolver Position & Fault Message */
      azimuth_raw = message->data16[0];
      azimuth_fault_raw = message->data8[3];

      if(azimuth_fault_raw != 0x00)
      {
        device_azresolver = DEVICE_FAULT;
      }
      else
      {
        device_azresolver = DEVICE_OK;
      }
    }
    else if(message->SID == 0x023 && message->DLC == 8)
    {
      /* Azimuth Resolver Sysinfo Message */
    }
    else if(message->SID == 0x024 && message->DLC == 8)
    {
      /* Azimuth Motor Info Message */

      uint32_t azimuth_motorfault;
      uint32_t azimuth_motorcontrol;
      uint32_t _az_limit_1, _az_limit_2;

      azimuth_pwm = message->data16[0];
      azimuth_current = message->data16[2];
      _az_limit_1 = (message->data8[7] >> 7) & 0x01;
      _az_limit_2 = (message->data8[7] >> 6) & 0x01;
      azimuth_motorfault = (message->data8[7] >> 5) & 0x01;
      azimuth_motorcontrol = (message->data8[7] >> 4) & 0x01;

      /* Derive ESTOP status */
      if((_az_limit_1 == 1) && (_az_limit_2 == 1))
      {
        estop = ESTOP_HALT;
      }
      else
      {
        estop = ESTOP_OK;
      }

      /* Derive Device Status */
      if(azimuth_motorfault == 0x01)
      {
        device_azmotor = DEVICE_FAULT;
      }
      else if(azimuth_motorcontrol == 0x00)
      {
        /* Motor Control inhibited */
        device_azmotor = DEVICE_INACTIVE;
      }
      else
      {
        device_azmotor = DEVICE_OK;
      }
    }

    ip_send_canmessage(message);
  }
}

THD_FUNCTION(can_rx_service_thread, arg)
{
  (void)arg;

  chRegSetThreadName("can_rx_service");

  msg_t result;
  CANRxFrame rxmsg;

  can_init();

  while(1)
  {
    result = canReceiveTimeout(&CAND1, CAN_ANY_MAILBOX, &rxmsg, TIME_MS2I(100));
    if(result == MSG_OK)
    {
      /* Message Received */
      can_rx_process(&rxmsg);
    }

    watchdog_feed(WATCHDOG_DOG_CANRX);
  }
}