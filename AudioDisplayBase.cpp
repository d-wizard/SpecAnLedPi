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
#include <assert.h>
#include "AudioDisplayBase.h"


AudioDisplayBase::AudioDisplayBase(size_t frameSize, size_t numDisplayPoints):
   m_frameSize(frameSize),
   m_displayPoints(numDisplayPoints),
   m_numDisplayPoints(numDisplayPoints),
   m_numNonBlackPoints(numDisplayPoints)
{

}

bool AudioDisplayBase::parsePcm(const SpecAnLedTypes::tPcmSample* samples, size_t numSamp)
{
   // For now only handling inputs that match the frame size.
   assert(numSamp == m_frameSize);
   return processPcm(samples);
}

void AudioDisplayBase::fillInLeds(SpecAnLedTypes::tRgbVector& ledColors, std::unique_ptr<ColorScale>& colorScale, float brightness, int gain)
{
   fillInDisplayPoints(gain); // Fill in m_displayPoints
   
   // Convert m_displayPoints to color via colorScale
   for(size_t i = 0; i < m_numNonBlackPoints; ++i)
   {
      ledColors[i] = colorScale->getColor(m_displayPoints[i], brightness);
   }
   for(size_t i = m_numNonBlackPoints; i < m_numDisplayPoints; ++i)
   {
      ledColors[i].u32 = SpecAnLedTypes::COLOR_BLACK;
   }
}

