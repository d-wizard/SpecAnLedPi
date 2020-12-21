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

#include <cmath>
#include "AudioDisplayAmplitude.h"

AudioDisplayAmp::AudioDisplayAmp(size_t frameSize, size_t numDisplayPoints, float fadeAwayFactor):
   AudioDisplayBase(frameSize, numDisplayPoints),
   m_fadeAwayFactor(fadeAwayFactor)
{

}

bool AudioDisplayAmp::processPcm(const SpecAnLedTypes::tPcmSample* samples)
{
   int peak = 0;
   for(size_t i = 0; i < m_frameSize; ++i)
   {
      auto mag = std::abs(samples[i]);
      if(mag > peak)
         peak = mag;
   }
   m_peak = peak;
   return true;
}

void AudioDisplayAmp::fillInDisplayPoints(int gain)
{
   size_t numLeds = m_displayPoints.size();
   size_t maxIndex = numLeds-1;

   // Use the most recent peak to determine 
   size_t newPeakLed = m_peak * gain * numLeds;
   newPeakLed >>= 18;
   if(newPeakLed > maxIndex)
      newPeakLed = maxIndex;
   else if(newPeakLed < 1)
      newPeakLed = 1;

   // Let the 
   m_ledToUse -= m_fadeAwayFactor;
   if(m_ledToUse < 1)
      m_ledToUse = 1;

   if(newPeakLed > m_ledToUse)
      m_ledToUse = newPeakLed;

   size_t peakLed = m_ledToUse + 0.5;

   for(size_t i = 0; i <= peakLed; ++i)
   {
      m_displayPoints[i] = (i * 0xFFFF) / peakLed;
   }

   for(size_t i = peakLed+1; i < numLeds; ++i)
   {
      m_displayPoints[i] = 0;
   }
}

