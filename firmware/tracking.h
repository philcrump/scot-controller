#ifndef __TRACKING_H__
#define __TRACKING_H__

/* TODO: Fix this mess */
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

void tracking_elevation_set_demand(float new_elevation_demand_degrees);

void tracking_azimuth_set_demand(float new_azimuth_demand_degrees);

THD_FUNCTION(tracking_thread, arg);

#endif /* __TRACKING_H__ */