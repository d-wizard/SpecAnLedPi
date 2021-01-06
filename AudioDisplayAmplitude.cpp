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

AudioDisplayAmp::AudioDisplayAmp(size_t frameSize, size_t numDisplayPoints, eAmpDisplayType displayType, float gradient_fadeAwayFactor, float peak_fadeAwayFactor):
   AudioDisplayBase(frameSize, numDisplayPoints, displayType == E_MAX_SAME ? 1.0 : 0.5),
   m_displayType(displayType),
   m_grad_fadeAwayFactor(gradient_fadeAwayFactor),
   m_addPeak(peak_fadeAwayFactor > 0.0),
   m_peak_fadeFactorStart(peak_fadeAwayFactor),
   m_peak_fadeFactorCurrent(peak_fadeAwayFactor)
{
   if(m_addPeak)
   {
      m_overridePoints.resize(1); //Set room for the slowly falling peak.
   }
}

bool AudioDisplayAmp::processPcm(const SpecAnLedTypes::tPcmSample* samples)
{
   int maxSamp = 0;
   for(size_t i = 0; i < m_frameSize; ++i)
   {
      auto mag = std::abs(samples[i]);
      if(mag > maxSamp)
         maxSamp = mag;
   }
   m_maxAudioPcmSample = maxSamp;
   return true;
}

void AudioDisplayAmp::fillInDisplayPoints(int gain)
{
   constexpr int NoColorMin = -2;
   const int numLeds = m_displayPoints.size();
   const int maxLedIndex = numLeds-1;

   // Use the most recent gradient max.
   int newGradMaxLed = m_maxAudioPcmSample * gain * numLeds;
   newGradMaxLed >>= 17; // Scale down to keep within bounds.
   if(newGradMaxLed > maxLedIndex)
      newGradMaxLed = maxLedIndex;
   else if(newGradMaxLed <= 0)
      newGradMaxLed = NoColorMin; // Make silence display nothing.

   // Fade away the current Gradient Max LED Position value.
   m_grad_maxPosition -= m_grad_fadeAwayFactor;
   if(m_grad_maxPosition < NoColorMin)
      m_grad_maxPosition = NoColorMin;

   // Check if new value is higher than the "faded-away" version of the Gradient Max LED Position value.
   if(newGradMaxLed > m_grad_maxPosition)
      m_grad_maxPosition = newGradMaxLed;

   int grad_maxLedPosition = m_grad_maxPosition;

   // Determine how to display the amplitude (default to E_SCALE)
   int delta = 0;
   int divisor = grad_maxLedPosition;
   uint16_t peakFadeColor = 0;
   bool useSavedPeakForLowerValues = false;
   switch(m_displayType)
   {
      case E_SCALE:
      default:
         delta = 0;
         divisor = grad_maxLedPosition;
         peakFadeColor = 0xFFFF;
      break;
      case E_MIN_SAME:
         delta = 0;
         divisor = maxLedIndex;
         if(grad_maxLedPosition >= 0)
            peakFadeColor = ((grad_maxLedPosition+delta) * 0xFFFF) / divisor;
         useSavedPeakForLowerValues = true;
      break;
      case E_MAX_SAME:
         delta = maxLedIndex - grad_maxLedPosition;
         divisor = maxLedIndex;
         peakFadeColor = 0;
      break;
   }

   // Make sure not to divide by zero.
   if(divisor < 1)
      divisor = 1;

   for(int i = 0; i <= grad_maxLedPosition; ++i)
   {
      m_displayPoints[i] = ((i+delta) * 0xFFFF) / divisor;
   }
   m_numNonBlackPoints = grad_maxLedPosition < 0 ? 0 : grad_maxLedPosition+1;

   if(m_addPeak)
   {
      m_peak_position -= m_peak_fadeFactorCurrent;
      if(m_grad_maxPosition > m_peak_position)
      {
         // Reset 
         m_peak_fadeFactorCurrent = m_peak_fadeFactorStart;
         m_peak_position = m_grad_maxPosition;

         if(useSavedPeakForLowerValues)
         {
            m_peak_savedFadeColor = peakFadeColor;
         }
      }
      if(!useSavedPeakForLowerValues)
      {
         m_peak_savedFadeColor = peakFadeColor;
      }

      if(m_peak_position < NoColorMin)
         m_peak_position = NoColorMin;
      
      int desiredPeakLed = m_peak_position + 0.5;
      if(desiredPeakLed == grad_maxLedPosition && grad_maxLedPosition < maxLedIndex)
         desiredPeakLed++;

      // Define where the peak color and where it will go.
      m_overridePoints[0] = m_peak_savedFadeColor;
      m_overrideStart = desiredPeakLed;

      // Accelerate the Fade down of the peak.
      m_peak_fadeFactorCurrent += (m_peak_fadeFactorStart*0.03);
   }
}

