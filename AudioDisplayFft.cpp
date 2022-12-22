/* Copyright 2020, 2022 Dan Williams. All Rights Reserved.
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
#include <math.h>
#include "AudioDisplayFft.h"



AudioDisplayFft::AudioDisplayFft(size_t sampleRate, size_t frameSize, size_t numDisplayPoints, eFftColorDisplay colorDisplay, bool mirror):
   AudioDisplayBase(frameSize, numDisplayPoints, colorDisplay == E_BRIGHTNESS_MAG ? 1.0 : 0.0, mirror),
   m_fftResult(nullptr),
   m_brightDisplayType(colorDisplay)
{
   // FFT Stuff
   m_fftRun.reset(new FftRunRate(sampleRate, frameSize, 150.0));

   tFftModifiers mod;
   mod.startFreq = 300;
   mod.stopFreq = 12000;
   mod.clipMin = 0;
   mod.clipMax = 5000;
   mod.logScale = false;
   mod.attenLowFreqs = true;
   mod.attenLowStartLevel = 0.2;
   mod.attenLowStopFreq = 6000;
   mod.fadeAwayAmount = (colorDisplay == E_BRIGHTNESS_MAG ? 50 : 30);
   m_fftModifier.reset(new FftModifier(sampleRate, frameSize, m_numDisplayPoints, mod));

}

bool AudioDisplayFft::processPcm(const SpecAnLedTypes::tPcmSample* samples)
{
   m_fftResult = m_fftRun->run(samples, m_frameSize);
   return m_fftResult != nullptr;
}

void AudioDisplayFft::fillInDisplayPoints(int gain)
{
   if(m_fftResult != nullptr)
   {
      gain *= 6;
      m_fftModifier->modify(m_fftResult->data());

      if(m_brightDisplayType == E_GRADIENT_MAG)
      {
         for(size_t i = 0 ; i < m_numDisplayPoints; ++i)
         {
            int32_t ledVal = (int32_t)m_fftResult->data()[i]*gain;
            if(ledVal > 0xFFFF)
               ledVal = 0xFFFF;
            m_displayPoints[i] = ledVal;
         }
      }
      else // E_BRIGHTNESS_MAG
      {
         for(size_t i = 0 ; i < m_numDisplayPoints; ++i)
         {
            int32_t brightVal = (int32_t)m_fftResult->data()[i]*gain;
            if(brightVal > 0x10000)
               brightVal = 0x10000;
            m_pointsBrightness[i] = float(brightVal) / float(0x10000);
            m_pointsBrightness[i] = pow(m_pointsBrightness[i], 1.8); // reduce smaller brightness values much more the higher brightness value to give a bigger distinction, since brightness is the only indicator.

            m_displayPoints[i] = (0xFFFF * i + ((m_numDisplayPoints-1)>>1)) / (m_numDisplayPoints-1);
         }
      }
   }
   m_fftResult = nullptr;
}