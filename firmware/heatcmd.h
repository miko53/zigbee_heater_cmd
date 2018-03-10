/* 
 * File:   heatcmd.h
 * Author: mickael
 *
 * Created on 2 janvier 2018, 13:17
 */

#ifndef HEATCMD_H
#define	HEATCMD_H

#include <xc.h>
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define     HEAT_MINUS      (0x02)
#define     HEAT_PLUS       (0x01)

#define     heat_set(heatcmd)   LATC |= heatcmd;
#define     heat_reset(heatcmd) LATC &= ~(heatcmd);

#endif	/* HEATCMD_H */

