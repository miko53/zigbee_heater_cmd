
#include <xc.h>
#include "config.h"
#include "global.h"
#include "leds.h"
#include <stdarg.h>
#include "uart.h"
#include "zb_handle.h"
#include "timer.h"

static void main_loop(void);
//static void test_loop(void);

/*
static void delay_s(uint8_t s)
{
    uint8_t loop;
    uint8_t nb_sec;
    for(nb_sec = 0; nb_sec < s; nb_sec++)
    {
        for(loop =0; loop < 20; loop++)
        {
            __delay_ms(50);
        }
    }
}
*/

void main(void)
{
  OSCTUNE = 0x00;

  //set up internal clock to 8MHz
  OSCCON  = 0x70;

  //configure PORT D in output for led display
  PORTD = 0x0;
  TRISD = 0x0;

  //put RB0 and RB1 in output
  //put RB2 also in output
  //RB0 = _RESET
  //RB1 = SLEEP_RQ
  //RB2 = BATT_SENSOR_ENABLED  ===> not present for cmd
  LATB = 0x00;
  TRISB = 0xF8;

  uart_setup();


  INTCONbits.PEIE = 1; //activate peripherical interrupt
  INTCONbits.GIE = 1; //activate global interrupt

  XBEE_WAKE_UP();
  XBEE_RESET_ON();
  XBEE_RESET_OFF();

  leds_glitch(LED_GREEN);

  //xbee wait API modem status frame associated and display green led.
  zb_handle_setHeaterCommand(HEAT_STOP, 0);

  main_loop();

  //test_loop();
}

/*
static void test_loop(void)
{
  //test_loop
  while (1)
  {
      leds_set(LED_GREEN|LED_RED|LED_YELLOW);
      timer0_wait_1s();
      //delay_s(1);
      leds_reset(LED_GREEN|LED_RED|LED_YELLOW);
      timer0_wait_1s();
      //delay_s(1);
  }
}
*/

#define ZIGBEE_PAYLOAD_CMD_SIZE         (20)
static uint8_t zigbee_command_payload[ZIGBEE_PAYLOAD_CMD_SIZE];
static BOOL decode_zb_payload(uint8_t* buffer, uint8_t size, uint8_t* cmd);

static void main_loop(void)
{
  zb_statusT zb_status;
  uint32_t counter_s;

  counter_s = 0;

  zb_handle_setPayloadBuffer(zigbee_command_payload, ZIGBEE_PAYLOAD_CMD_SIZE);

  while (1)
  {
    //reception etat de la liaison + reception nouvelle ordre si recu
    zb_handle();
    zb_status = zb_handle_getStatus();
    switch (zb_status)
    {
      case ZB_STATUS_NOT_JOINED:
        leds_glitch(LED_GREEN);
        break;

      case ZB_STATUS_JOINED:
        leds_set(LED_GREEN);
        break;

      default:
        break;
    }

    if (zb_status == ZB_STATUS_JOINED)
    {
      uint8_t receivedSize;
      uint8_t cmd;
      receivedSize = zb_handle_getLastReceivedPayloadSize();
      if (receivedSize != 0)
      {
        //decode
        //update command
        //reset size of received data
        if (decode_zb_payload(zigbee_command_payload, receivedSize, &cmd) == TRUE)
        {
          leds_glitch(LED_YELLOW);
          zb_handle_setHeaterCommand(cmd, 0);
          zb_handle_sendData();
        }
        zb_handle_resetPayloadSize();
      }
    }

    //envoyer le status toutes les minutes trente à la station (heartbeat)
    if (((counter_s % 90) == 0) &&
        (zb_status == ZB_STATUS_JOINED))
    {
      //send status
      zb_handle_sendData();
    }

    timer0_wait_1s();
    counter_s++;
  }
}

static BOOL decode_zb_payload(uint8_t* buffer, uint8_t size, uint8_t* cmd)
{
  BOOL rc;
  rc = FALSE;

  if (size == 7)
  {
    rc = TRUE;
  }

  if (rc)
  {
    if ((buffer[0] != 0x00) || //DB protocol
        (buffer[2] != 0x01) || //1 actionneur
        (buffer[3] != 0x81) || //command de chauffage
        (buffer[5] != 0x00) )  //id = 0
    {
      rc = FALSE;
    }
  }

  if (rc)
  {
    if (buffer[6] > HEAT_STOP)
    {
      rc = FALSE;
    }
    else
    {
      *cmd = buffer[6];
    }
  }

  return rc;
}