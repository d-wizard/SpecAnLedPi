/* Copyright 2024 Dan Williams. All Rights Reserved.
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
#include "SpotLightStrip.h"

static int LED_BOUNDARIES[SpotLightStrip::NUM_ZONES][2] = 
{
   {0,70},     // FRONT
   {50,120},   // FRONT_CORNER
   {92,150},   // MIDDLE_FRONT
   {140,197},  // MIDDLE_BACK
   {170,240},  // BACK_CORNER
   {220,296}   // BACK
};

SpotLightStrip::SpotLightStrip(std::shared_ptr<LedStrip> ledStrip, int spotLights, uint8_t brightness)
   : m_ledStrip(ledStrip)
   , m_spotLights(spotLights)
   , m_brightness(brightness)
{
   setStrip();
}

SpotLightStrip::~SpotLightStrip()
{
   m_spotLights = 0;
   setStrip();
}

void SpotLightStrip::setSpotLights(int spotLights)
{
   m_spotLights = spotLights;
   setStrip();
}

void SpotLightStrip::setBrightness(uint8_t brightness)
{
   m_brightness = brightness;
   setStrip();
}

void SpotLightStrip::setStrip()
{
   const size_t numLeds = m_ledStrip->getNumLeds();
   
   // Create Vector that will define which LEDs are on and which are off. Start them all off (i.e. color black).
   SpecAnLedTypes::tRgbColor black;
   black.u32 = SpecAnLedTypes::COLOR_BLACK;
   SpecAnLedTypes::tRgbVector ledColors(numLeds, black);

   // Check which zones are set.
   for(int zoneIndex = 0; zoneIndex < SpotLightStrip::NUM_ZONES; ++zoneIndex)
   {
      int mask = 1 << zoneIndex;
      if(m_spotLights & mask)
      {
         for(int ledIndex = LED_BOUNDARIES[zoneIndex][0]; ledIndex <= LED_BOUNDARIES[zoneIndex][1]; ++ledIndex)
         {
            if(ledIndex >= 0 && ledIndex < int(numLeds))
            {
               ledColors[ledIndex].rgb.r = m_brightness;
               ledColors[ledIndex].rgb.g = m_brightness;
               ledColors[ledIndex].rgb.b = m_brightness;
            }
         }
      }
   }

   // Update the LED strip.
   m_ledStrip->set(ledColors);
}
