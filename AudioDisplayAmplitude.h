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
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <memory>
#include "specAnLedPiTypes.h"
#include "colorScale.h"
#include "AudioDisplayBase.h"

class AudioDisplayAmp : public AudioDisplayBase
{
public:
   typedef enum
   {
      E_SCALE,
      E_MIN_SAME,
      E_MAX_SAME
   }eAmpDisplayType;
   
   typedef enum
   {
      E_PEAK_NONE,
      E_PEAK_GRAD_MAX,
      E_PEAK_GRAD_MIN,
      E_PEAK_GRAD_MID_CONST,
      E_PEAK_GRAD_MID_CHANGE
   }ePeakType;
   
   AudioDisplayAmp(size_t sampleRate, size_t frameSize, size_t numDisplayPoints, eAmpDisplayType displayType, float fullFadeTime, ePeakType peakType, bool mirror = false);

private:
   static constexpr int NO_COLOR_MIN_INDEX = -2; // Set to 2 less than mininum valid index (0). This is to ensure that if a peak is used it will also be able to be less than 0.
   const int NUM_LEDS;
   const int MAX_LED_INDEX;

   // Make uncopyable
   AudioDisplayAmp();
   AudioDisplayAmp(AudioDisplayAmp const&);
   void operator=(AudioDisplayAmp const&);

   bool processPcm(const SpecAnLedTypes::tPcmSample* samples) override;

   void fillInDisplayPoints(int gain) override;

   void fillInPeak();

   eAmpDisplayType m_displayType = E_SCALE;
   int m_maxAudioPcmSample = 0;

   float m_grad_fadeAwayFactor = 0;
   float m_grad_maxPosition = 0;

   ePeakType m_peak_type = E_PEAK_NONE;
   float m_peak_fadeFactorStart = 0;
   float m_peak_fadeFactorCurrent = 0;
   float m_peak_position = 0;
   uint16_t m_peak_savedFadeColor = 0;

};

