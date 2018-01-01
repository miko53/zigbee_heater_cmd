/*
 * File:   global.h
 * Author: mickael
 *
 * Created on 10 décembre 2017, 17:16
 */

#ifndef GLOBAL_H
#define	GLOBAL_H

#ifdef	__cplusplus
extern "C" {
#endif

//set Frequency to 8MHz
#define _XTAL_FREQ   (8000000)

#define START_WATCHDOG()  (WDTCONbits.SWDTEN = 1)
#define STOP_WATCHDOG()   (WDTCONbits.SWDTEN = 0)

#define START_IDLE() (OSCCONbits.IDLEN = 1)
#define STOP_IDLE()  (OSCCONbits.IDLEN = 0)

#define DEEP_SLEEP()   do { START_WATCHDOG();SLEEP();STOP_WATCHDOG(); } while (0);
#define IDLE_SLEEP()   do { START_IDLE();SLEEP();STOP_IDLE(); } while (0);

#define MIN(a,b) (((a)<(b))?(a):(b))

typedef enum
{
  HEAT_CONFORT,
  HEAT_ECO,
  HEAT_HG,
  HEAT_STOP
} heaterOrder;


#ifdef	__cplusplus
}
#endif

#endif	/* GLOBAL_H */

