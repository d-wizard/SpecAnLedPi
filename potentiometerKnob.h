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
#include <stdint.h>
#include <memory>
#include <cmath>
#include "potentiometerAdc.h"

class PotentiometerKnob
{
public:
   PotentiometerKnob(std::shared_ptr<PotentiometerAdc> pot, int32_t resolution, float outputScalar = 1.0):
      m_pot(pot),
      m_resolution(resolution),
      m_outputScalar(outputScalar)
   {
   }

   PotentiometerKnob(std::shared_ptr<SeeedAdc8Ch12Bit> adc, int adcNum, int32_t resolution, float outputScalar = 1.0):
      m_pot(new PotentiometerAdc(adc, adcNum)),
      m_resolution(resolution),
      m_outputScalar(outputScalar)
   {
   }

   bool getInt(int32_t& knobValue)
   {
      bool changed = !m_validRead;
      auto raw = m_pot->getRaw();
      auto knobPoint = getKnobPoint(raw);
      auto rawDelta = abs(m_prevRaw - raw);

      if(rawDelta >= Hysteresis && m_prevKnobPoint != knobPoint)
      {
         changed = true;
      }

      if(changed)
      {
         m_prevRaw = raw;
         m_prevKnobPoint = knobPoint;
      }
      m_validRead = true;

      knobValue = m_prevKnobPoint;
      return changed;
   }
   int32_t getInt()
   {
      int32_t retVal;
      getInt(retVal);
      return retVal;
   }

   bool getFlt(float& value)
   {
      int32_t knobValue;
      bool changed = getInt(knobValue);
      value = float(knobValue) / float(m_resolution) * m_outputScalar;
      return changed;
   }
   float getFlt()
   {
      float retVal;
      getFlt(retVal);
      return retVal;
   }

private:
   static constexpr PotentiometerAdc::adcRaw_t Hysteresis = 30;

   std::shared_ptr<PotentiometerAdc> m_pot;
   int32_t m_resolution;
   float m_outputScalar;

   bool m_validRead = false;
   PotentiometerAdc::adcRaw_t m_prevRaw = 0;
   int32_t m_prevKnobPoint = 0;
   
   int32_t getKnobPoint(PotentiometerAdc::adcRaw_t adcVal)
   {
      return (int32_t(adcVal) * (m_resolution+1)) >> PotentiometerAdc::AdcResolution;
   }
   
};

