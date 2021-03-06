#include <stdint.h>
#include <string.h>
#include "zb_handle.h"
#include "uart.h"
#include "zb.h"
#include "leds.h"
#include "global.h"
//#include "timer.h"
#include "uart_loc.h"

#define MAX_SIZE_FRAME                (50)

#define SENSOR_PROTOCOL_DATA_TYPE     (0x00)
#define SENSOR_PROTOCOL_DBG_TYPE      (0x01)
#define SENSOR_HYT221_TEMP            (0x01)
#define SENSOR_HYT221_HUM             (0x02)
#define SENSOR_VOLTAGE                (0x03)
#define ACT_HEATER                    (0x81)

#define STATUS_NORMAL_DATA            (0x03)
#define STATUS_INVALID                (0x00)

#define OFFSET_SIZE                      (1)
#define OFFSET_FRAMEID                   (4)
#define OFFSET_COUNTER                  (18)
#define OFFSET_HEATER_STATUS            (21)
#define OFFSET_HEATER_CMD               (22)

/*const uint8_t zb_forceDisassociation[] =
{
  ZB_START_DELIMITER, 0x00, 0x04, ZIGBEE_API_AT_CMD, 0x00, 'F', 'R', 0x5f
};*/

static uint8_t zb_frameToSend[] =
{
  ZB_START_DELIMITER,
  0, //size
  4, //size without the checksum
  ZIGBEE_API_TRANSMIT_REQUEST,
  2, //frameID
  0x00, //coordinator @1
  0x00, //coordinator @2
  0x00, //coordinator @3
  0x00, //coordinator @4
  0x00, //coordinator @5
  0x00, //coordinator @6
  0x00, //coordinator @7
  0x00, //coordinator @8
  0xFF, //coord 16bits ZIGBEE_UNKNOWN_16B_ADDR
  0xFE, //coord 16bits ZIGBEE_UNKNOWN_16B_ADDR
  0x00, //broadcast radius
  0x00, //option
  //payload
  SENSOR_PROTOCOL_DATA_TYPE,
  0x00,//counter
  0x01,//number of sensor/actuator
  ACT_HEATER,
  0x00, //status
  0x00, //command H
  0x00, //command L
  //and checksum
  0x00
};

//#define DBG_FRAME
//#define USE_FRAME_ID

#ifdef DBG_FRAME
static uint8_t zb_dbgFrame[] =
{
  ZB_START_DELIMITER,
  0, //size
  4, //size without the checksum
  ZIGBEE_API_TRANSMIT_REQUEST,
  0, //frameID
  0x00, //coordinator @1
  0x00, //coordinator @2
  0x00, //coordinator @3
  0x00, //coordinator @4
  0x00, //coordinator @5
  0x00, //coordinator @6
  0x00, //coordinator @7
  0x00, //coordinator @8
  0xFF, //coord 16bits ZIGBEE_UNKNOWN_16B_ADDR
  0xFE, //coord 16bits ZIGBEE_UNKNOWN_16B_ADDR
  0x00, //broadcast radius
  0x00, //option
  //payload
  SENSOR_PROTOCOL_DBG_TYPE,
  0x00,//counter
  0x00,
  0x00,
  0x00,
  //and checksum
  0x00
};
#endif

#ifdef USE_FRAME_ID
static uint8_t zb_frameID = 1;
static uint8_t zb_currentFrameID;
static int8_t zb_currentAck;
#endif /* USE_FRAME_ID */

static uint8_t zb_counter;
#ifdef DBG_FRAME
static uint8_t zb_dbg1;
#endif

typedef struct
{
  uint8_t status;
  uint8_t id;
  uint16_t heater;
} heaterCommand;

static heaterCommand heater_data;
static zb_statusT zb_status = ZB_STATUS_NOT_JOINED;
static uint8_t* zb_payload = NULL;
static uint8_t zb_payloadSize;
static uint8_t zb_payloadReceivedSize;
static uint8_t zb_frameReceived[MAX_SIZE_FRAME];

static void zigbee_appendChecksum(uint8_t* buffer, uint8_t* sizeFrame);

zb_statusT zb_handle_getStatus(void)
{
  return zb_status;
}

void zb_handle_setHeaterCommand(heaterOrder heater, uint8_t id)
{
  heater_data.heater = heater;
  heater_data.id = id;
  heater_data.status = STATUS_NORMAL_DATA;
}

heaterOrder zb_handle_getHeaterCommand(void)
{
  //TODO manage the status if necc.
  return  heater_data.heater;
}

void zb_handle_resetStatus()
{
  heater_data.status = STATUS_INVALID;
}

void zb_handle_setPayloadBuffer(uint8_t* buffer, uint8_t size)
{
  zb_payload = buffer;
  zb_payloadSize = size;
}

void zb_handle_resetPayloadSize(void)
{
  zb_payloadReceivedSize = 0;
}


uint8_t zb_handle_getLastReceivedPayloadSize(void)
{
  return zb_payloadReceivedSize;
}

void zb_handle_sendData()
{
#ifdef USE_FRAME_ID
  zb_currentFrameID = zb_frameID;
  zb_frameID++;
  if (zb_frameID == 0)
  {
    zb_frameID = 1;
  }
  zb_currentAck = -1;
  zb_frameToSend[OFFSET_FRAMEID] = zb_currentFrameID;
#else
  zb_frameToSend[OFFSET_FRAMEID] = 0;
#endif /* USE_FRAME_ID */

  zb_frameToSend[OFFSET_COUNTER] = zb_counter++;
  zb_frameToSend[OFFSET_HEATER_STATUS]   = heater_data.status;
  zb_frameToSend[OFFSET_HEATER_CMD]   = heater_data.id;
  zb_frameToSend[OFFSET_HEATER_CMD + 1] = heater_data.heater;
  zb_frameToSend[OFFSET_SIZE] = ((sizeof(zb_frameToSend) - 1 - ZB_HEADER_SIZE) & 0xFF00) >> 8;
  zb_frameToSend[OFFSET_SIZE + 1] = ((sizeof(zb_frameToSend) - 1 - ZB_HEADER_SIZE) & 0x00FF);

  uint8_t frameSize = sizeof(zb_frameToSend) - 1;
  zigbee_appendChecksum(zb_frameToSend, &frameSize);

  uart_write(frameSize, zb_frameToSend);
}

/*void zb_handle_force_disassociation()
{
  uart_write(sizeof(zb_forceDisassociation), zb_forceDisassociation);
}*/

#ifdef DBG_FRAME
void zb_handle_sendDbgData()
{
  zb_dbgFrame[19] = zb_currentAck;
  zb_dbgFrame[20] = zb_dbg1;
  zb_dbgFrame[21] = zb_currentFrameID;
  uint8_t frameSize = sizeof(zb_dbgFrame) - 1;
  zb_dbgFrame[OFFSET_SIZE] = ((frameSize - ZB_HEADER_SIZE) & 0xFF00) >> 8;
  zb_dbgFrame[OFFSET_SIZE + 1] = ((frameSize - ZB_HEADER_SIZE) & 0x00FF);
  zigbee_appendChecksum(zb_dbgFrame, &frameSize);
  uart_write(frameSize, zb_dbgFrame);
  zb_dbg1 = 0;
}
#endif

static void zigbee_appendChecksum(uint8_t* buffer, uint8_t* sizeFrame)
{
  buffer[*sizeFrame] = zb_doChecksum(&buffer[ZB_HEADER_SIZE], &buffer[*sizeFrame] - &buffer[ZB_HEADER_SIZE]);
  (*sizeFrame)++;
}


void zb_handle(void)
{
  BOOL bSuccess;
  uint16_t sizeOfNextData;
  zigbee_decodedFrame decodedFrame;

  bSuccess = uart_read(zb_frameReceived, ZB_HEADER_SIZE);
  if (bSuccess)
  {
    if (zb_frameReceived[0] == ZB_START_DELIMITER)
    {
      sizeOfNextData = (((uint16_t) zb_frameReceived[1]) << 8) | (zb_frameReceived[2]);
      if (sizeOfNextData <= (MAX_SIZE_FRAME - ZB_HEADER_SIZE))
      {
        bSuccess = uart_read(zb_frameReceived + ZB_HEADER_SIZE, sizeOfNextData + 1);  //+1 for the checksum
      }
      else
      {
        leds_glitch(LED_RED);
        bSuccess = FALSE;
      }
    }
    else
    {
      bSuccess = FALSE;
    }
  }

  if (bSuccess)
  {
    bSuccess = zb_decodage(zb_frameReceived + ZB_HEADER_SIZE, sizeOfNextData + 1, &decodedFrame);
  }

  if (bSuccess)
  {
    switch (decodedFrame.type)
    {
      case ZIGBEE_API_AT_CMD:
        break;

      case ZIGBEE_API_TRANSMIT_REQUEST:
        break;

      case ZIGBEE_AT_COMMAND_RESPONSE:
        break;

      case ZIGBEE_MODEM_STATUS:
        if (decodedFrame.status == 0x02)
        {
          zb_status = ZB_STATUS_JOINED;
          //leds_glitch(LED_GREEN);
        }
        else if (decodedFrame.status == 0x03)
        {
          zb_status = ZB_STATUS_NOT_JOINED;
          //leds_glitch(LED_RED);
        }
        break;

      case ZIGBEE_TRANSMIT_STATUS:
#ifdef USE_FRAME_ID
        if (zb_currentFrameID == decodedFrame.frameID)
        {
          zb_currentAck = decodedFrame.status;
        }
        else
        {
          ;
        }
#endif /* USE_FRAME_ID */
        break;

      case ZIGBEE_RECEIVE_PACKET:
        if ((decodedFrame.data != NULL) &&
            ((zb_payload != NULL) && (zb_payloadSize != 0)) &&
            (zb_payloadReceivedSize == 0))
        {
          uint8_t* ptr = zb_payload;
          zb_payloadReceivedSize = MIN(decodedFrame.size, zb_payloadSize);
          //copy in reception buffer to be decoded by main part.
          for (uint8_t i = 0 ; i < zb_payloadReceivedSize; i++)
          {
            *ptr++ = decodedFrame.data[i];
          }
        }
        else
        {
          leds_glitch(LED_RED);
        }
        break;

      default:
        break;
    }
  }

}

BOOL zb_handle_waitAck(void)
{
#ifdef USE_FRAME_ID
  uint8_t retryCounter;
  BOOL bAckReceived;

  bAckReceived = FALSE;
  retryCounter = 0;
  //5
  //19 max retry = 3 --> 4.8s
  timer0_wait_262ms();

  while ((retryCounter < 18) && (bAckReceived == FALSE))
  {
    zb_handle();
    if (zb_currentAck == 0)
    {
      bAckReceived = TRUE;
    }

    timer0_wait_262ms(); //wait end of transmission
    retryCounter++;
  }

  return bAckReceived;
#else
  //timer0_wait_262ms();
  //timer0_wait_262ms();
  return TRUE;
#endif
}
