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

AudioDisplayAmp::AudioDisplayAmp(size_t frameSize, size_t numDisplayPoints, eAmpDisplayType displayType, float fadeAwayFactor, float peakFadeAwayFactor):
   AudioDisplayBase(frameSize, numDisplayPoints, 0.5),
   m_displayType(displayType),
   m_fadeAwayFactor(fadeAwayFactor),
   m_addPeak(peakFadeAwayFactor > 0.0),
   m_peakFadeFactorStart(peakFadeAwayFactor),
   m_peakFadeFactorCurrent(peakFadeAwayFactor)
{
   if(m_addPeak)
   {
      m_overridePoints.resize(1); //Set room for the slowly falling peak.
   }
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
   m_measuredPeak = peak;
   return true;
}

void AudioDisplayAmp::fillInDisplayPoints(int gain)
{
   size_t numLeds = m_displayPoints.size();
   size_t maxIndex = numLeds-1;

   // Use the most recent peak to determine 
   size_t newPeakLed = m_measuredPeak * gain * numLeds;
   newPeakLed >>= 17;
   if(newPeakLed > maxIndex)
      newPeakLed = maxIndex;
   else if(newPeakLed < 1)
      newPeakLed = 1;

   // Fade away the current LED value.
   m_ledToUse -= m_fadeAwayFactor;
   if(m_ledToUse < 1)
      m_ledToUse = 1;

   // Check for new peak.
   if(newPeakLed > m_ledToUse)
      m_ledToUse = newPeakLed;

   size_t peakLed = m_ledToUse + 0.5;

   // Determine how to display the amplitude (default to E_SCALE)
   size_t delta = 0;
   size_t divisor = peakLed;
   uint16_t peakFadeColor = 0;
   bool useSavedPeakForLowerValues = false;
   switch(m_displayType)
   {
      case E_SCALE:
      default:
         delta = 0;
         divisor = peakLed;
         peakFadeColor = 0xFFFF;
      break;
      case E_MIN_SAME:
         delta = 0;
         divisor = maxIndex;
         peakFadeColor = ((peakLed+delta) * 0xFFFF) / divisor;
         useSavedPeakForLowerValues = true;
      break;
      case E_PEAK_SAME:
         delta = maxIndex - peakLed;
         divisor = maxIndex;
         peakFadeColor = 0;
      break;
   }

   for(size_t i = 0; i <= peakLed; ++i)
   {
      m_displayPoints[i] = ((i+delta) * 0xFFFF) / divisor;
   }
   m_numNonBlackPoints = peakLed+1;

   if(m_addPeak)
   {
      m_ledToUsePeak -= m_peakFadeFactorCurrent;
      if(m_ledToUse > m_ledToUsePeak)
      {
         // Reset 
         m_peakFadeFactorCurrent = m_peakFadeFactorStart;
         m_ledToUsePeak = m_ledToUse;

         if(useSavedPeakForLowerValues)
         {
            m_savedPeakFadeColor = peakFadeColor;
         }
      }
      if(!useSavedPeakForLowerValues)
      {
         m_savedPeakFadeColor = peakFadeColor;
      }

      if(m_ledToUsePeak < 1)
         m_ledToUsePeak = 1;
      
      size_t desiredPeakLed = m_ledToUsePeak + 0.5;
      if(desiredPeakLed == peakLed && peakLed < maxIndex)
         desiredPeakLed++;

      m_overridePoints[0] = m_savedPeakFadeColor;
      m_overrideStart = desiredPeakLed;

      // Accelerate the Fade down of the peak.
      m_peakFadeFactorCurrent += (m_peakFadeFactorStart*0.03);
   }
}

