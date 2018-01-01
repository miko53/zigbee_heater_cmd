
#include <xc.h>
#include "global.h"
#include "leds.h"

void leds_glitch(uint8_t leds)
{
  LATD |= leds;
  __delay_ms(50);
  __delay_ms(50);
  LATD &= ~leds;
}
