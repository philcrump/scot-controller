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
      /* Azimuth Resolver Position & Fault Message */
      azimuth_raw = ((uint16_t)message->data8[0] << 8) | (uint16_t)message->data8[1];
      azimuth_fault_raw = message->data8[3];
    }
    else if(message->SID == 0x013 && message->DLC == 8)
    {
      /* Azimuth Resolver Sysinfo Message */
    }
    else if(message->SID == 0x020 && message->DLC == 3)
    {
      /* Elevation Resolver Position & Fault Message */
      elevation_raw = ((uint16_t)message->data8[0] << 8) | (uint16_t)message->data8[1];
      elevation_fault_raw = message->data8[3];
    }
    else if(message->SID == 0x023 && message->DLC == 8)
    {
      /* Elevation Resolver Sysinfo Message */
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

    //watchdog_feed(WATCHDOG_DOG_CANRX);
  }
}