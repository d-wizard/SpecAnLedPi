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
#include <assert.h>
#include "AudioDisplayBase.h"
#include "gradientToScale.h"


AudioDisplayBase::AudioDisplayBase(size_t frameSize, size_t numDisplayPoints, float firstLedBrightness, bool mirror):
   m_numForwardPoints(mirror ? (numDisplayPoints+1)/ 2 : numDisplayPoints),
   m_numReflectionPoints(numDisplayPoints - m_numForwardPoints),
   m_frameSize(frameSize),
   m_displayPoints(m_numForwardPoints),
   m_numDisplayPoints(m_numForwardPoints),
   m_numNonBlackPoints(m_numForwardPoints),
   m_firstLedBrightness(m_numForwardPoints),
   m_pointsBrightness(m_numForwardPoints, 1.0), // Init to no modification of brightness for all Display Points
   m_mirror(mirror)
{

}

void AudioDisplayBase::setGradient(ColorGradient::tGradient& gradient, bool reverseGrad)
{
   // Convert input gradient and set brightness.
   std::vector<ColorScale::tColorPoint> colors;
   if(reverseGrad)
   {
      auto revGrad = Convert::reverseGradient(gradient);
      Convert::convertGradientToScale(revGrad, colors);
   }
   else
   {
      Convert::convertGradientToScale(gradient, colors);
   }
   ColorScale::tBrightnessScale brightPoints{{m_firstLedBrightness,0},{1,1}}; // Scale brightness.

   // Set the member variable for defining LED colors.
   std::unique_lock<std::mutex> lock(m_colorScaleMutex);
   m_colorScale.reset(new ColorScale(colors, brightPoints));
}

bool AudioDisplayBase::parsePcm(const SpecAnLedTypes::tPcmSample* samples, size_t numSamp)
{
   // For now only handling inputs that match the frame size.
   assert(numSamp == m_frameSize);
   return processPcm(samples);
}

void AudioDisplayBase::fillInLeds(SpecAnLedTypes::tRgbVector& ledColors, float brightness, int gain)
{
   fillInDisplayPoints(gain); // Fill in m_displayPoints
   
   // Convert m_displayPoints to color via colorScale
   std::unique_lock<std::mutex> lock(m_colorScaleMutex);
   for(size_t i = 0; i < m_numNonBlackPoints; ++i)
   {
      ledColors[m_numReflectionPoints+i] = m_colorScale->getColor(m_displayPoints[i], brightness * m_pointsBrightness[i]);
   }
   for(size_t i = m_numNonBlackPoints; i < m_numDisplayPoints; ++i)
   {
      ledColors[m_numReflectionPoints+i].u32 = SpecAnLedTypes::COLOR_BLACK;
   }

   // Check for Override Points
   int overridePoints_num = m_overridePoints.size();
   if((m_overrideStart + overridePoints_num) <= int(ledColors.size()) && m_overrideStart >= 0)
   {
      for(int i = 0; i < overridePoints_num; ++i)
      {
         ledColors[m_numReflectionPoints+i+m_overrideStart] = m_colorScale->getColor(m_overridePoints[i], brightness * m_pointsBrightness[i]);
      }
   }

   // Copy over the relection points.
   if(m_mirror)
   {
      size_t convertVal = m_numForwardPoints + m_numReflectionPoints - 1;
      for(size_t i = 0; i < m_numReflectionPoints; ++i)
      {
         ledColors[i] = ledColors[convertVal - i];
      }
   }
}

