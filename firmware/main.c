
#include <xc.h>
#include "config.h"
#include "global.h"
#include "leds.h"
#include <stdarg.h>
#include "uart.h"
#include "zb_handle.h"
#include "timer.h"
#include "heatcmd.h"


#define ZIGBEE_PAYLOAD_CMD_SIZE         (20)
static uint8_t zigbee_command_payload[ZIGBEE_PAYLOAD_CMD_SIZE];
static BOOL decode_zb_payload(uint8_t* buffer, uint8_t size, heaterOrder* cmd);
static void applyHeaterCommand(heaterOrder cmd, uint8_t id);
static void manageHeaterCommand(void);


static void main_loop(void);
static void test_loop(void);


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


void main(void)
{
  OSCTUNE = 0x00;

  //set up internal clock to 8MHz
  OSCCON  = 0x70;

  //configure PORT A in output for led display
  PORTA = 0x0;
  TRISA = 0x0;

  //put RB0 and RB1 in output
  //RB1 = _RESET
  //RB0 = SLEEP_RQ
  LATB = 0x00;
  TRISB = 0xFC;

  //heater on RC0 and RC1
  LATC = 0x00;
  TRISC = 0xFC;

  uart_setup();
  INTCONbits.PEIE = 1; //activate peripherical interrupt
  INTCONbits.GIE = 1; //activate global interrupt

  XBEE_WAKE_UP();
  XBEE_RESET_ON();
  XBEE_RESET_OFF();

  leds_glitch(LED_RED);


  applyHeaterCommand(HEAT_STOP, 0);
  manageHeaterCommand();

  main_loop();
  test_loop();

  /*   while (1)
     {
         LATA |= (LED_YELLOW);
         for(int i = 0; i < 1000; i++) NOP();
         LATA |= (LED_RED);
         for(int i = 0; i < 1000; i++) NOP();
         LATA |= (LED_GREEN);
         for(int i = 0; i < 1000; i++) NOP();
         LATA &= ~(LED_YELLOW | LED_RED | LED_GREEN);
         for(int i = 0; i < 1000; i++) NOP();
     }*/
}


static void test_loop(void)
{
  //test_loop
  while (1)
  {
      leds_set(LED_GREEN|LED_RED);
     // timer0_wait_1s();
      delay_s(1);
      leds_reset(LED_GREEN|LED_RED);
      //timer0_wait_1s();
      delay_s(1);
  }
}


static void main_loop(void)
{
  zb_statusT zb_status;
  uint16_t counter_s;

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
      heaterOrder cmd;
      receivedSize = zb_handle_getLastReceivedPayloadSize();
      if (receivedSize != 0)
      {
        //decode
        //update command
        //reset size of received data
        if (decode_zb_payload(zigbee_command_payload, receivedSize, &cmd) == TRUE)
        {
          leds_glitch(LED_YELLOW);
          applyHeaterCommand(cmd, 0);
          zb_handle_sendData();
        }
        zb_handle_resetPayloadSize();
      }
    }

    //envoyer le status toutes les minutes trente à la station (heartbeat)
    if (zb_status == ZB_STATUS_JOINED)
    {
      if (counter_s == 0)
      {
        //send status
        zb_handle_sendData();
      }
      else
      {
        if (counter_s >= 90)
        {
          counter_s = -1;
        }
      }
      counter_s++;
    }
    else
    {
      counter_s = 0;
    }

    manageHeaterCommand();
    timer0_wait_1s();
  }
}

heaterOrder currentCommand;
heaterOrder previousCommand = -1; //intialize with invalid value


static void applyHeaterCommand(heaterOrder cmd, uint8_t id)
{
  currentCommand = cmd;
  zb_handle_setHeaterCommand(cmd, id);
}

static uint16_t confortCounter = 0;

static void manageHeaterCommand(void)
{
  if (currentCommand != previousCommand)
  {
    switch (currentCommand)
    {
      case HEAT_CONFORT:
        //confort = pas de signal
        heat_reset(HEAT_MINUS | HEAT_PLUS);
        break;

      case HEAT_CONFORT_M1:
        //confort minus 1 = full during 3s and nothing until 5min expires
        heat_set(HEAT_MINUS | HEAT_PLUS);
        break;

      case HEAT_CONFORT_M2:
        //confort minus 2 = full during 7s and nothing until 5min expires
        heat_set(HEAT_MINUS | HEAT_PLUS);
        break;

      case HEAT_ECO:
        //eco = full alternance
        heat_set(HEAT_MINUS | HEAT_PLUS);
        break;

      case HEAT_HG:
        //hg = 1/2 alternance negative
        heat_reset(HEAT_PLUS);
        heat_set(HEAT_MINUS);
        break;

      case HEAT_STOP:
        //arret = 1/2 alternance positive
        heat_set(HEAT_PLUS);
        heat_reset(HEAT_MINUS);
        break;

      default:
        break;
    }
    previousCommand = currentCommand;
    confortCounter = 0;
  }
  else
  {
    switch (currentCommand)
    {
      case HEAT_CONFORT_M1:
        if (confortCounter >= 3)
        {
          heat_reset(HEAT_MINUS | HEAT_PLUS);
        }
        else
        {
          heat_set(HEAT_MINUS | HEAT_PLUS);
        }
        break;

      case HEAT_CONFORT_M2:
        if (confortCounter >= 7)
        {
          heat_reset(HEAT_MINUS | HEAT_PLUS);
        }
        else
        {
          heat_set(HEAT_MINUS | HEAT_PLUS);
        }
        break;

      default:
        //do nothing so time evolution for another commands.
        break;
    }
  }

  if (confortCounter >= 300)
  {
    confortCounter = 0;
  }
  else
  {
    confortCounter++;
  }
}

static BOOL decode_zb_payload(uint8_t* buffer, uint8_t size, heaterOrder* cmd)
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
        (buffer[3] != 0x81) || //commande de chauffage
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