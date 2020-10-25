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
#pragma once

#include <memory>
#include "seeed_adc_8chan_12bit.h"

class PotentiometerAdc
{
public:
   static constexpr int AdcResolution = 12;
   typedef uint16_t adcRaw_t;

   PotentiometerAdc(std::shared_ptr<SeeedAdc8Ch12Bit> adc, int adcNum):
      m_adc(adc),
      m_adcNum(adcNum)
   {}

   // Delete constructors / operations that should not be allowed.
   PotentiometerAdc() = delete;
   PotentiometerAdc(PotentiometerAdc const&) = delete;
   void operator=(PotentiometerAdc const&) = delete;

   adcRaw_t getRaw()
   {
      return m_adc->getAdcValue(m_adcNum);
   }
   float getFlt()
   {
      return (float)m_adc->getAdcValue(m_adcNum) / (float)((1<<AdcResolution)-1);
   }

private:

   std::shared_ptr<SeeedAdc8Ch12Bit> m_adc;
   int m_adcNum;

};
