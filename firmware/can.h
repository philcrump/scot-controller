#ifndef __CAN_H__
#define __CAN_H__

void can_init(void);

void can_send_pwm_elevation(const int16_t pwm);
void can_send_pwm_azimuth(const int16_t pwm);

THD_FUNCTION(can_rx_service_thread, arg);

#endif /* __CAN_H__ */