/*
 * File:   leds.h
 * Author: mickael
 *
 * Created on 10 décembre 2017, 17:02
 */

#ifndef LEDS_H
#define	LEDS_H

#include <xc.h>
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define     LED_YELLOW      (0x01)
#define     LED_RED         (0x02)
#define     LED_GREEN       (0x04)


#define     leds_set(leds)   LATD |= leds;
#define     leds_reset(leds) LATD &= ~(leds);

extern void leds_glitch(uint8_t leds);


#ifdef	__cplusplus
}
#endif

#endif	/* LEDS_H */

