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
#pragma once
#include <memory>
#include <vector>
#include <chrono>
#include <stdint.h>
#include "ledStrip.h"

class SpotLightStrip
{
public:
   // Public Types.
   static constexpr int NUM_ZONES = 6;

   static constexpr int FRONT = 1;
   static constexpr int FRONT_CORNER = 2;
   static constexpr int MIDDLE_FRONT = 4;
   static constexpr int MIDDLE_BACK = 8;
   static constexpr int BACK_CORNER = 16;
   static constexpr int BACK = 32;
   static constexpr int ALL = FRONT | FRONT_CORNER | MIDDLE_FRONT | MIDDLE_BACK | BACK_CORNER | BACK;


public:
   SpotLightStrip(std::shared_ptr<LedStrip> ledStrip, int spotLights, uint8_t brightness = 20);
   virtual ~SpotLightStrip();

   void setSpotLights(int spotLights);
   void setBrightness(uint8_t brightness);

private:
   std::shared_ptr<LedStrip> m_ledStrip;
   int m_spotLights;
   uint8_t m_brightness;

   void setStrip();

};
