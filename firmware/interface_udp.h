#ifndef __INTERFACE_UDP_H__
#define __INTERFACE_UDP_H__

#define APP_IP_SERVICE_SNTP_STATUS_DOWN        0
#define APP_IP_SERVICE_SNTP_STATUS_POLLING     1
#define APP_IP_SERVICE_SNTP_STATUS_SYNCED      2

uint32_t app_ip_service_sntp_status(void);

THD_FUNCTION(user_ip_services_thread, arg);

void ip_send_canmessage(CANRxFrame *message_ptr);

THD_FUNCTION(udp_tx_service_thread, arg);

THD_FUNCTION(udp_rx_service_thread, arg);

#endif /* __INTERFACE_UDP_H__ */