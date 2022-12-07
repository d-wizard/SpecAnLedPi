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
#include <mutex>
#include "specAnLedPiTypes.h"
#include "colorGradient.h"
#include "colorScale.h"

class AudioDisplayBase
{
public:
   AudioDisplayBase(size_t frameSize, size_t numDisplayPoints, float firstLedBrightness = 0.0, bool mirror = false);

   void setGradient(ColorGradient::tGradient& gradient, bool reverseGrad);

   size_t getFrameSize(){return m_frameSize;}

   bool parsePcm(const SpecAnLedTypes::tPcmSample* samples, size_t numSamp);

   void fillInLeds(SpecAnLedTypes::tRgbVector& ledColors, float brightness, int gain);

private:
   // Make uncopyable
   AudioDisplayBase();
   AudioDisplayBase(AudioDisplayBase const&);
   void operator=(AudioDisplayBase const&);

   virtual bool processPcm(const SpecAnLedTypes::tPcmSample* samples) = 0;

   virtual void fillInDisplayPoints(int gain) = 0;

protected:
   size_t m_numForwardPoints;
   size_t m_numReflectionPoints;
   size_t m_frameSize;
   std::vector<uint16_t> m_displayPoints;
   size_t m_numDisplayPoints;

   size_t m_numNonBlackPoints; // All LEDs after this value will be set to Black.

   std::vector<uint16_t> m_overridePoints;
   int m_overrideStart = -1;


   float m_firstLedBrightness;
   std::unique_ptr<ColorScale> m_colorScale;
   std::mutex m_colorScaleMutex;

   // Brightness modifier.
   std::vector<float> m_pointsBrightness;

   bool m_mirror = false;
};
