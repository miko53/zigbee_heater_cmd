
#include <xc.h>
#include "global.h"
#include "leds.h"

void leds_glitch(uint8_t leds)
{
  LATA |= leds;
  __delay_ms(50);
  __delay_ms(50);
  LATA &= ~leds;
}
