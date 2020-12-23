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
      E_PEAK_SAME
   }eAmpDisplayType;
   
   AudioDisplayAmp(size_t frameSize, size_t numDisplayPoints, eAmpDisplayType displayType, float fadeAwayFactor, float peakFadeAwayFactor = 0.0);

private:
   // Make uncopyable
   AudioDisplayAmp();
   AudioDisplayAmp(AudioDisplayAmp const&);
   void operator=(AudioDisplayAmp const&);

   bool processPcm(const SpecAnLedTypes::tPcmSample* samples) override;

   void fillInDisplayPoints(int gain) override;

   eAmpDisplayType m_displayType = E_SCALE;
   int m_measuredPeak = 0;

   float m_fadeAwayFactor = 0;
   float m_ledToUse = 0;

   bool m_addPeak = false;
   float m_peakFadeFactorStart = 0;
   float m_peakFadeFactorCurrent = 0;
   float m_ledToUsePeak = 0;
   uint16_t m_savedPeakFadeColor = 0;

};

