#ifndef __TRACKING_H__
#define __TRACKING_H__

/* TODO: Fix this mess */
extern bool tracking_azimuth_local_control;
extern bool tracking_azimuth_remote_control;

extern bool tracking_elevation_local_control;
extern bool tracking_elevation_remote_control;

extern uint16_t azimuth_raw;
extern uint16_t elevation_raw;

extern uint8_t azimuth_fault_raw;
extern uint8_t elevation_fault_raw;

extern float azimuth_degrees;
extern float elevation_degrees;

extern float azimuth_demand_degrees;
extern float elevation_demand_degrees;

extern float azimuth_error_degrees;
extern float elevation_error_degrees;

extern int16_t elevation_pwm;
extern int16_t elevation_current;

extern int16_t azimuth_pwm;
extern int16_t azimuth_current;

typedef enum {
    LIMIT_UNKNOWN,
    LIMIT_OK,
    LIMIT_HALT
} limit_t;

extern limit_t el_limit_1;
extern limit_t el_limit_2;

typedef enum {
    ESTOP_UNKNOWN,
    ESTOP_OK,
    ESTOP_HALT
} estop_t;

extern estop_t estop;

typedef enum {
    CONTROL_LOCAL_POSITION,
    CONTROL_REMOTE_POSITION,
    CONTROL_REMOTE_MOTOR
} tracking_control_t;

extern tracking_control_t control_elevation;
extern tracking_control_t control_azimuth;

typedef enum {
    DEVICE_UNKNOWN,
    DEVICE_FAULT,
    DEVICE_INACTIVE,
    DEVICE_OK
} remotedevice_t;

extern remotedevice_t device_elresolver;
extern remotedevice_t device_azresolver;
extern remotedevice_t device_elmotor;
extern remotedevice_t device_azmotor;

void tracking_elevation_set_demand(float new_elevation_demand_degrees);

void tracking_azimuth_set_demand(float new_azimuth_demand_degrees);

THD_FUNCTION(tracking_thread, arg);

#endif /* __TRACKING_H__ */