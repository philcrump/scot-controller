#ifndef __INTERFACE_UDP_H__
#define __INTERFACE_UDP_H__

void ip_link_up_cb(void *p);
void ip_link_down_cb(void *p);

void ip_send_canmessage(CANRxFrame *message_ptr);

THD_FUNCTION(udp_tx_service_thread, arg);

THD_FUNCTION(udp_rx_service_thread, arg);

#endif /* __INTERFACE_UDP_H__ */