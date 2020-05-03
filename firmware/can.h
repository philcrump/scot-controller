#ifndef __CAN_H__
#define __CAN_H__

void can_init(void);

THD_FUNCTION(can_rx_service_thread, arg);

#endif /* __CAN_H__ */