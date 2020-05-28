#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#define WATCHDOG_DOG_MAIN        0
#define WATCHDOG_DOG_CANRX       1
#define WATCHDOG_DOG_TRACKING    2
#define WATCHDOG_DOG_SCREEN      3
#define WATCHDOG_DOG_USERIPSRVS  4
#define WATCHDOG_DOG_UDP_TX      5
#define WATCHDOG_DOG_UDP_RX      6


#define WATCHDOG_MASK       ((1 << WATCHDOG_DOG_MAIN) \
                             | (1 << WATCHDOG_DOG_CANRX) \
                             | (1 << WATCHDOG_DOG_TRACKING) \
                             | (1 << WATCHDOG_DOG_SCREEN) \
                             | (1 << WATCHDOG_DOG_USERIPSRVS) \
                             | (1 << WATCHDOG_DOG_UDP_TX) \
                             | (1 << WATCHDOG_DOG_UDP_RX))

void watchdog_init(void);

THD_FUNCTION(watchdog_service_thread, arg);

void watchdog_feed(uint32_t dog);

#endif /* __WATCHDOG_H__ */