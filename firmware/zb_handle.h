/*
 * File:   zb_handle.h
 * Author: mickael
 *
 * Created on 12 juillet 2015, 16:11
 */

#ifndef ZB_HANDLE_H
#define	ZB_HANDLE_H

#include <stdint.h>
#include <xc.h>
#include "global.h"

#ifdef	__cplusplus
extern "C" {
#endif

//RB0 = _RESET
//RB1 = SLEEP_RQ
#define XBEE_RESET_OFF()  (LATBbits.LATB0 = 1)
#define XBEE_RESET_ON()   (LATBbits.LATB0 = 0)
#define XBEE_SLEEP_RQ()   (LATBbits.LATB1 = 1)
#define XBEE_WAKE_UP()    (LATBbits.LATB1 = 0)

typedef enum
{
  ZB_STATUS_NOT_JOINED,
  ZB_STATUS_JOINED,
} zb_statusT;

extern void zb_handle(void);
extern zb_statusT zb_handle_getStatus(void);
extern void zb_handle_sendData();
extern void zb_handle_sendDbgData();
extern BOOL zb_handle_waitAck();
extern void zb_handle_resetStatus();
extern void zb_handle_setHeaterCommand(heaterOrder heater, uint8_t id);
extern heaterOrder zb_handle_getHeaterCommand(void);
extern void zb_handle_setPayloadBuffer(uint8_t* buffer, uint8_t size);
extern void zb_handle_resetPayloadSize(void);
extern uint8_t zb_handle_getLastReceivedPayloadSize(void);

//extern void zb_handle_force_disassociation();

#ifdef	__cplusplus
}
#endif

#endif	/* ZB_HANDLE_H */

