/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_LWIPOPTS_H__
#define LWIP_HDR_LWIPOPTS_H__

#define LWIP_DHCP                       1
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_LINK_POLL_INTERVAL         TIME_MS2I(500)

/* SNTP-related settings */
#define LWIP_DHCP_GET_NTP_SRV           1
#define LWIP_DHCP_MAX_NTP_SERVERS       2
#define SNTP_MAX_SERVERS                (2+1)
#define SNTP_SERVER_DNS                 0
#define SNTP_CHECK_RESPONSE             2
#define SNTP_COMP_ROUNDTRIP             1
#define SNTP_STARTUP_DELAY              0
#define SNTP_UPDATE_DELAY               (10*60*1000)

#define SNTP_GET_SYSTEM_TIME(sec, us)   do { \
    RTCDateTime timespec_; \
    struct tm tm_; \
    u32_t tm_ms_; \
    rtcGetTime(&RTCD1, &timespec_); \
    rtcConvertDateTimeToStructTm(&timespec_, &tm_, &tm_ms_); \
    (sec) = mktime(&tm_); \
    (us) = tm_ms_ * 1000; \
  } while (0)
#define SNTP_SET_SYSTEM_TIME_US(sec, us)    do { \
    extern bool app_time_syncing; \
    app_time_syncing = true; \
    time_t sec_; \
    struct tm tm_; \
    uint32_t ms_, ms_to_wait; \
    sec_ = (sec); \
    ms_ = ((us) / 1000); \
    ms_to_wait = 1000 - ms_; /* Number of milliseconds to put in timer */ \
    sec_ += 1; /* Set time to set at the top of the next second */ \
    if(ms_to_wait < 200) { /* Too close for comfort, go for next second */ \
        sec_ += 1; \
        ms_to_wait += 1000; \
    } \
    gmtime_r(&sec_, &tm_); \
    extern RTCDateTime time_set_timespec; \
    rtcConvertStructTmToDateTime(&tm_, 0, &time_set_timespec); \
    extern void time_set_timer_toset(uint32_t milliseconds); \
    time_set_timer_toset(ms_to_wait); \
  } while (0)

/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#define PBUF_POOL_SIZE                  32
#define MEMP_NUM_PBUF                   32

/* Fixed settings mandated by the ChibiOS integration.*/
#include "static_lwipopts.h"

/* Optional, application-specific settings.*/
#if !defined(TCPIP_MBOX_SIZE)
#define TCPIP_MBOX_SIZE                 MEMP_NUM_PBUF
#endif
#if !defined(TCPIP_THREAD_STACKSIZE)
#define TCPIP_THREAD_STACKSIZE          1024
#endif

/* Use ChibiOS specific priorities. */
#if !defined(TCPIP_THREAD_PRIO)
#define TCPIP_THREAD_PRIO               (NORMALPRIO)
#endif
#if !defined(LWIP_THREAD_PRIORITY)
#define LWIP_THREAD_PRIORITY            (LOWPRIO + 1)
#endif

#endif /* LWIP_HDR_LWIPOPTS_H__ */ 