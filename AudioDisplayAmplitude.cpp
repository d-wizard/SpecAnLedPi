/* Copyright 2020, 2022 - 2023 Dan Williams. All Rights Reserved.
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
#include "AudioDisplayAmplitude.h"

AudioDisplayAmp::AudioDisplayAmp(size_t sampleRate, size_t frameSize, size_t numDisplayPoints, eAmpDisplayType displayType, float fullFadeTime, ePeakType peakType, bool mirror):
   AudioDisplayBase(frameSize, numDisplayPoints, peakType == E_PEAK_GRAD_MIN ? 1.0 : 0.5, mirror),
   NUM_LEDS(m_displayPoints.size()),
   MAX_LED_INDEX(NUM_LEDS-1),
   m_displayType(displayType),
   m_grad_fadeAwayFactor(double(NUM_LEDS)*double(frameSize)/(double(sampleRate)*double(fullFadeTime))),
   m_peak_type(peakType),
   m_peak_fadeFactorStart(m_grad_fadeAwayFactor*3/70), // This gets the peak to fade from max led to nothing about 5x slower than the gradient.
   m_peak_fadeFactorCurrent(m_peak_fadeFactorStart)
{
   if(m_peak_type != E_PEAK_NONE)
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

void AudioDisplayAmp::fillInPeak()
{
   uint16_t peakFadeColor = 0;
   bool useSavedPeakForLowerValues = false;

   switch(m_peak_type)
   {
      default:
      break;
      case E_PEAK_GRAD_MAX:
         peakFadeColor = 0xFFFF;
      break;
      case E_PEAK_GRAD_MIN:
         peakFadeColor = 0;
      break;
      case E_PEAK_GRAD_MID_CONST:
      {
         if(m_grad_maxPosition >= 0)
            peakFadeColor = (m_grad_maxPosition * 0xFFFF) / MAX_LED_INDEX;
         useSavedPeakForLowerValues = true;
      }
      break;
      case E_PEAK_GRAD_MID_CHANGE:
      {
         float oldPeakPos = m_peak_position;
         if(oldPeakPos > MAX_LED_INDEX)
            oldPeakPos = MAX_LED_INDEX;
         else if(oldPeakPos < 0)
            oldPeakPos = 0;
         peakFadeColor = (oldPeakPos * float(0xFFFF)) / float(MAX_LED_INDEX);
      }
      break;
   }

   m_peak_position -= m_peak_fadeFactorCurrent;
   if(m_grad_maxPosition > m_peak_position)
   {
      // Reset 
      m_peak_fadeFactorCurrent = m_peak_fadeFactorStart;
      m_peak_position = m_grad_maxPosition;

      if(useSavedPeakForLowerValues)
      {
         m_peak_savedFadeColor = peakFadeColor; // Update saved color now that the peak has been reset.
      }
   }
   if(!useSavedPeakForLowerValues)
   {
      m_peak_savedFadeColor = peakFadeColor;
   }

   if(m_peak_position < NO_COLOR_MIN_INDEX)
      m_peak_position = NO_COLOR_MIN_INDEX;
   
   int desiredPeakLed = m_peak_position + 0.5;
   if(desiredPeakLed == m_grad_maxPosition && m_grad_maxPosition < MAX_LED_INDEX)
      desiredPeakLed++;

   // Define where the peak color and where it will go.
   m_overridePoints[0] = m_peak_savedFadeColor;
   m_overrideStart = desiredPeakLed;

   // Accelerate the Fade down of the peak.
   m_peak_fadeFactorCurrent += (m_peak_fadeFactorStart*0.15);

}

void AudioDisplayAmp::fillInDisplayPoints(int gain)
{
   // Use the most recent gradient max.
   int newGradMaxLed = m_maxAudioPcmSample * gain * NUM_LEDS;
   newGradMaxLed >>= 17; // Scale down to keep within bounds.
   if(newGradMaxLed > MAX_LED_INDEX)
      newGradMaxLed = MAX_LED_INDEX;
   else if(newGradMaxLed <= 0)
      newGradMaxLed = NO_COLOR_MIN_INDEX; // Make silence display nothing.

   // Fade away the current Gradient Max LED Position value.
   m_grad_maxPosition -= m_grad_fadeAwayFactor;
   if(m_grad_maxPosition < NO_COLOR_MIN_INDEX)
      m_grad_maxPosition = NO_COLOR_MIN_INDEX;

   // Check if new value is higher than the "faded-away" version of the Gradient Max LED Position value.
   if(newGradMaxLed > m_grad_maxPosition)
      m_grad_maxPosition = newGradMaxLed;

   // Determine how to display the amplitude (default to E_SCALE)
   int delta = 0;
   int divisor = m_grad_maxPosition;
   switch(m_displayType)
   {
      case E_SCALE:
      default:
         delta = 0;
         divisor = m_grad_maxPosition;
      break;
      case E_MIN_SAME:
         delta = 0;
         divisor = MAX_LED_INDEX;
      break;
      case E_MAX_SAME:
         delta = MAX_LED_INDEX - m_grad_maxPosition;
         divisor = MAX_LED_INDEX;
      break;
   }

   // Make sure not to divide by zero.
   if(divisor < 1)
      divisor = 1;

   for(int i = 0; i <= m_grad_maxPosition; ++i)
   {
      m_displayPoints[i] = ((i+delta) * 0xFFFF) / divisor;
   }
   m_numNonBlackPoints = m_grad_maxPosition < 0 ? 0 : m_grad_maxPosition+1;

   if(m_peak_type != E_PEAK_NONE)
   {
      fillInPeak();
   }
}

