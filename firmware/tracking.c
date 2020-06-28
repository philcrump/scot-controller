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

/*** JOYSTICK ADC ***/

#define JOYSTICK_ADC_CHANNELS         2
#define JOYSTICK_ADC_BUFFER_DEPTH     1024
static adcsample_t joystick_adc_samples[JOYSTICK_ADC_CHANNELS * JOYSTICK_ADC_BUFFER_DEPTH];

uint32_t joystick_adc_elevation = 0x7FF/2;
uint32_t joystick_adc_azimuth = 0x7FF/2;

static void joystick_adc_callback(ADCDriver *adcp)
{
  uint32_t i, j;
  uint32_t elevationSum = 0;
  uint32_t azimuthSum = 0;

  if(adcIsBufferComplete(adcp))
  {
    j = JOYSTICK_ADC_BUFFER_DEPTH / 2;
  }
  else
  {
    j = 0;
  }

  for(i = 0; i < JOYSTICK_ADC_BUFFER_DEPTH / 2; i+=2)
  {
    elevationSum += joystick_adc_samples[i+j];
    azimuthSum += joystick_adc_samples[i+j+1];
  }

  joystick_adc_elevation = elevationSum / (JOYSTICK_ADC_BUFFER_DEPTH / 2);
  joystick_adc_azimuth = azimuthSum / (JOYSTICK_ADC_BUFFER_DEPTH / 2);
}

static const ADCConversionGroup joystick_adc_cfg = {
  TRUE,                     //circular buffer mode
  JOYSTICK_ADC_CHANNELS,    //Number of the analog channels
  joystick_adc_callback,              //Callback function
  NULL,         //Error callback
  0,                        /* CR1 */
  ADC_CR2_SWSTART,          /* CR2 */
  0,
  ADC_SMPR2_SMP_AN8(ADC_SAMPLE_480) | ADC_SMPR2_SMP_AN7(ADC_SAMPLE_480),
  0,
  0,
  ADC_SQR1_NUM_CH(JOYSTICK_ADC_CHANNELS),
  0,
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN7)   | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN8)
};

static int16_t adcToPWM(uint16_t adc)
{
  if(adc < 0x380)
  {
    return -((1024 * (0x380 - adc)) / 0x37F);
  }
  else if(adc > 0x480)
  {
    return ((1024 * (adc - 0x480)) / 0x37F);
  }
  else
  {
    return 0;
  }
}

extern void ip_send_joystick(uint32_t az, uint32_t el);

THD_FUNCTION(tracking_thread, arg)
{
  (void)arg;

  control_elevation = CONTROL_NONE;
  control_azimuth = CONTROL_NONE;

  adcStart(&ADCD3, NULL);
  adcStartConversion(&ADCD3, &joystick_adc_cfg, joystick_adc_samples, JOYSTICK_ADC_BUFFER_DEPTH);

  while(true)
  {
    /* Joystick Switch Enable */
    if(!palReadLine(LINE_JOYSTICK_NENABLE))
    {
      control_elevation = CONTROL_LOCAL_VELOCITY;
      control_azimuth = CONTROL_LOCAL_VELOCITY;
    }
    else
    {
      control_elevation = CONTROL_NONE;
      control_azimuth = CONTROL_NONE;
    }

    tracking_recalculate();

    /* Elevation Control Loop */
    if(control_elevation == CONTROL_LOCAL_VELOCITY)
    {
      /* Read Joystick Value */
      can_send_pwm(0x01B, adcToPWM(joystick_adc_elevation));
      can_send_pwm(0x02B, adcToPWM(joystick_adc_azimuth));
    }
    //ip_send_joystick(joystick_adc_elevation, joystick_adc_azimuth);

    watchdog_feed(WATCHDOG_DOG_TRACKING);
    chThdSleepMilliseconds(10);
  }
}