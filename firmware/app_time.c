#include "main.h"

bool app_time_syncing = false;

static virtual_timer_t time_set_vtimer;
RTCDateTime time_set_timespec;

void time_set_time_cb(void *arg)
{
  (void)arg;
  
  rtcSetTime(&RTCD1, &time_set_timespec);

  RTCD1.rtc->CR &= ~RTC_CR_ALRAIE;
  RTCD1.rtc->CR &= ~RTC_CR_ALRAE;

  app_time_syncing = false;
}

void time_set_timer_toset(uint32_t milliseconds)
{
  chSysLock();

  chVTDoSetI(&time_set_vtimer, TIME_MS2I(milliseconds), time_set_time_cb, NULL);

  chSysUnlock();
}