
#include <xc.h>
#include <stdint.h>
#include "zb.h"

#define ZB_UNKNOWN_16B_ADDR    (0xFFFE)


uint8_t zb_doChecksum(uint8_t* frame, uint8_t size)
{
  uint8_t checksum;

  checksum = 0xFF;
  for (uint8_t i = 0; i < size; i++)
  {
    checksum -= frame[i];
  }

  return checksum;
}

BOOL zb_decodage(uint8_t* frame, uint8_t frameSize, zigbee_decodedFrame* decodedFrame)
{
  BOOL bCorrectlyDecoded;
  uint8_t checksum;
  bCorrectlyDecoded = FALSE;

  if (frameSize >= 1)
  {
    checksum = zb_doChecksum(frame, frameSize - 1);
    if (checksum != frame[frameSize - 1])
    {
      //fprintf(stdout, "Checksum KO\n");
      bCorrectlyDecoded = FALSE;
    }
    else
    {
      bCorrectlyDecoded = TRUE;
    }
  }

  if (bCorrectlyDecoded)
  {
    decodedFrame->type = frame[0];
    switch (frame[0])
    {
      case ZIGBEE_AT_COMMAND_RESPONSE:
        decodedFrame->frameID = frame[1];
        decodedFrame->AT[0] = frame[2];
        decodedFrame->AT[1] = frame[3];
        decodedFrame->status = frame[4];
        decodedFrame->size = frameSize - 6;
        if (decodedFrame->size <= 0)
        {
          decodedFrame->data = NULL;
        }
        else
        {
          decodedFrame->data = &frame[5];
        }
        break;

      case ZIGBEE_MODEM_STATUS:
        decodedFrame->status = frame[1];
        bCorrectlyDecoded = TRUE;
        break;

      case ZIGBEE_TRANSMIT_STATUS:
        decodedFrame->frameID = frame[1];
        decodedFrame->status = frame[5];
        break;

      case ZIGBEE_RECEIVE_PACKET:
        /*
        decodedFrame->zbAddress[0] = frame[1];
        decodedFrame->zbAddress[1] = frame[2];
        decodedFrame->zbAddress[2] = frame[3];
        decodedFrame->zbAddress[3] = frame[4];
        decodedFrame->zbAddress[4] = frame[5];
        decodedFrame->zbAddress[5] = frame[6];
        decodedFrame->zbAddress[6] = frame[7];
        decodedFrame->zbAddress[7] = frame[8];
        decodedFrame->zb16bAddress[0] = frame[9];
        decodedFrame->zb16bAddress[1] = frame[10];
        */
        //frame[11] => options
        decodedFrame->size = frameSize - 13;
        if (decodedFrame->size <= 0)
        {
          decodedFrame->data = NULL;
        }
        else
        {
          decodedFrame->data = &frame[12];
        }
        break;

      default:
        bCorrectlyDecoded = FALSE;
        break;
    }
  }

  return bCorrectlyDecoded;
}

