#ifndef __INTERFACE_UDP_H__
#define __INTERFACE_UDP_H__

#define APP_IP_LINK_STATUS_DOWN         0
#define APP_IP_LINK_STATUS_UPBUTNOIP    1
#define APP_IP_LINK_STATUS_BOUND        2

uint32_t app_ip_link_status(void);

#define APP_IP_SERVICE_SNTP_STATUS_DOWN        0
#define APP_IP_SERVICE_SNTP_STATUS_POLLING     1
#define APP_IP_SERVICE_SNTP_STATUS_SYNCED      2

uint32_t app_ip_service_sntp_status(void);

THD_FUNCTION(user_ip_services_thread, arg);

void ip_link_up_cb(void *p);
void ip_link_down_cb(void *p);

void ip_send_canmessage(CANRxFrame *message_ptr);

THD_FUNCTION(udp_tx_service_thread, arg);

THD_FUNCTION(udp_rx_service_thread, arg);

#endif /* __INTERFACE_UDP_H__ */