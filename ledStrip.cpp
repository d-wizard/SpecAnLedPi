/* Copyright 2020 Dan Williams. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "ledStrip.h"


LedStrip::LedStrip(size_t numLeds, eRgbOrder order, unsigned gpio):
   m_numLeds(numLeds)
{
   memset(&m_ledStrip, 0, sizeof(m_ledStrip));
   m_ledStrip.freq = WS2811_TARGET_FREQ;
   m_ledStrip.dmanum = 10; // Default.
   m_ledStrip.channel[0].gpionum = gpio;
   m_ledStrip.channel[0].count = m_numLeds;
   m_ledStrip.channel[0].brightness = 0xFF;

   int strip_type;
   switch(order)
   {
      default:
      case RGB:
         strip_type = WS2811_STRIP_RGB;
      break;
      case RBG:
         strip_type = WS2811_STRIP_RBG;
      break;
      case GRB:
         strip_type = WS2811_STRIP_GRB;
      break;
      case GBR:
         strip_type = WS2811_STRIP_GBR;
      break;
      case BRG:
         strip_type = WS2811_STRIP_BRG;
      break;
      case BGR:
         strip_type = WS2811_STRIP_BGR;
      break;
   }
   m_ledStrip.channel[0].strip_type = strip_type;

   // Initialize the LED strip.
   ws2811_init(&m_ledStrip);
}

LedStrip::~LedStrip()
{
   clear();
   ws2811_fini(&m_ledStrip);
}

void LedStrip::set(const SpecAnLedTypes::tRgbVector& ledColors)
{
   size_t numToSet = std::min(m_numLeds, ledColors.size());
   for(size_t i = 0; i < numToSet; ++i)
   {
      m_ledStrip.channel[0].leds[i] = ledColors[i].u32;
   }

   ws2811_render(&m_ledStrip);
}

void LedStrip::clear()
{
   SpecAnLedTypes::tRgbVector off;
   off.resize(m_numLeds);
   memset(&off[0].u32, 0, sizeof(off[0].u32)*m_numLeds);
   set(off);
}

