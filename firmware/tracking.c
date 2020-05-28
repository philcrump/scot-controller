#include "main.h"

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
    //tracking_recalculate();
  }
}

void tracking_azimuth_set_demand(float new_azimuth_demand_degrees)
{
  if(new_azimuth_demand_degrees >= 0.0 && new_azimuth_demand_degrees < 360.0)
  {
    azimuth_demand_degrees = new_azimuth_demand_degrees;
    //tracking_recalculate();
  }
}

THD_FUNCTION(tracking_thread, arg)
{
  (void)arg;

  while(true)
  {
    /***** WARNING, FOR DEBUGGING ONLY *****/
    elevation_raw = azimuth_raw;

    tracking_recalculate();

    watchdog_feed(WATCHDOG_DOG_TRACKING);
    chThdSleepMilliseconds(10);
  }
}