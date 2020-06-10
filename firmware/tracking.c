#include "main.h"

/* for each in [azimuth, elevation]:
   false, false - default, also triggered by UDP motor control packets, controller does no tracking
   true,  false - triggered by manual parameter entry, controller does tracking to manually entered position
   false, true  - triggered by UDP position control packets, controller does tracking to UDP position
*/
bool tracking_azimuth_local_control = false;
bool tracking_azimuth_remote_control = false;

bool tracking_elevation_local_control = false;
bool tracking_elevation_remote_control = false;

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

int16_t elevation_pwm = 0x0000;
int16_t elevation_current = 0x0000;

int16_t azimuth_pwm = 0x0000;
int16_t azimuth_current = 0x0000;

limit_t el_limit_1 = LIMIT_UNKNOWN;
limit_t el_limit_2 = LIMIT_UNKNOWN;

estop_t estop = ESTOP_UNKNOWN;

tracking_control_t control_elevation = CONTROL_REMOTE_MOTOR;
tracking_control_t control_azimuth = CONTROL_REMOTE_MOTOR;

remotedevice_t device_elresolver = DEVICE_UNKNOWN;
remotedevice_t device_azresolver = DEVICE_UNKNOWN;
remotedevice_t device_elmotor = DEVICE_UNKNOWN;
remotedevice_t device_azmotor = DEVICE_UNKNOWN;

static void tracking_recalculate(void)
{
  /* Calculate Elevation Values */
  elevation_degrees = (float)elevation_raw * (360.0 / 65536.0);
  if((elevation_degrees - elevation_demand_degrees) > 180.0)
  {
    elevation_error_degrees = (elevation_degrees - (360.0 + elevation_demand_degrees));
  }
  else if((elevation_degrees - elevation_demand_degrees) < -180.0)
  {
    elevation_error_degrees = (360.0 + elevation_degrees) - elevation_demand_degrees;
  }
  else
  {
    elevation_error_degrees = elevation_degrees - elevation_demand_degrees;
  }

  /* Calculate Azmimuth Values */
  azimuth_degrees = (float)azimuth_raw * (360.0 / 65536.0);
  if((azimuth_degrees - azimuth_demand_degrees) > 180.0)
  {
    azimuth_error_degrees = azimuth_degrees - (360.0 + azimuth_demand_degrees);
  }
  else if((azimuth_degrees - azimuth_demand_degrees) < -180.0)
  {
    azimuth_error_degrees = (360.0 + azimuth_degrees) - azimuth_demand_degrees;
  }
  else
  {
    azimuth_error_degrees = azimuth_degrees - azimuth_demand_degrees;
  }
}

void tracking_elevation_set_demand(float new_elevation_demand_degrees)
{
  if(new_elevation_demand_degrees >= 0.0 && new_elevation_demand_degrees < 360.0)
  {
    elevation_demand_degrees = new_elevation_demand_degrees;
    tracking_elevation_local_control = true;
    //tracking_recalculate();
  }
}

void tracking_azimuth_set_demand(float new_azimuth_demand_degrees)
{
  if(new_azimuth_demand_degrees >= 0.0 && new_azimuth_demand_degrees < 360.0)
  {
    azimuth_demand_degrees = new_azimuth_demand_degrees;
    tracking_azimuth_local_control = true;
    //tracking_recalculate();
  }
}

THD_FUNCTION(tracking_thread, arg)
{
  (void)arg;

  while(true)
  {
    tracking_recalculate();

    watchdog_feed(WATCHDOG_DOG_TRACKING);
    chThdSleepMilliseconds(10);
  }
}