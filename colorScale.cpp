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
#include <math.h>
#include "colorScale.h"

#define FULL_SCALE (65536.0f)

ColorScale::ColorScale(std::vector<tColorPoint>& colorPoints, std::vector<tBrightnessPoint>& brightnessPoints)
{
   size_t colorsSize = colorPoints.size()-1;
   m_red.resize(colorsSize);
   m_green.resize(colorsSize);
   m_blue.resize(colorsSize);
   m_colorPoints.resize(colorsSize);
   for(size_t i = 0; i < colorsSize; ++i)
   {
      bool first = (i == 0);
      bool last = (i == (colorsSize-1));
      int32_t startPoint = first ? 0 : colorPoints[i].startPoint * FULL_SCALE;
      int32_t endPoint   = last ? FULL_SCALE : colorPoints[i+1].startPoint * FULL_SCALE;

      // endPoint should keep getting bigger.
      assert( endPoint >= startPoint );
      assert( endPoint <= FULL_SCALE );

      // Store colors.
      m_red[i].start   = colorPoints[i  ].color.rgb.r;
      m_red[i].end     = colorPoints[i+1].color.rgb.r;
      m_green[i].start = colorPoints[i  ].color.rgb.g;
      m_green[i].end   = colorPoints[i+1].color.rgb.g;
      m_blue[i].start  = colorPoints[i  ].color.rgb.b;
      m_blue[i].end    = colorPoints[i+1].color.rgb.b;

      // Store off range where this is valid.
      m_colorPoints[i].start = startPoint;
      m_colorPoints[i].end   = endPoint;
   }

   size_t brightnessSize = brightnessPoints.size()-1;
   m_brightness.resize(brightnessSize);
   m_brightnessPoints.resize(brightnessSize);
   float maxBrightness = 255.0f * sqrtf(3); // 3 channels (red, green, blue), hence the square root of 3.
   for(size_t i = 0; i < brightnessSize; ++i)
   {
      bool first = (i == 0);
      bool last = (i == (brightnessSize-1));
      int32_t startPoint = first ? 0 : brightnessPoints[i].startPoint * FULL_SCALE;
      int32_t endPoint = last ? FULL_SCALE : brightnessPoints[i+1].startPoint * FULL_SCALE;

      // endPoint should keep getting bigger.
      assert( endPoint > startPoint );
      assert( endPoint <= FULL_SCALE );

      // Store brightness.
      m_brightness[i].start = brightnessPoints[i  ].brightness * maxBrightness;
      m_brightness[i].end   = brightnessPoints[i+1].brightness * maxBrightness;

      // Store off range where this is valid.
      m_brightnessPoints[i].start = startPoint;
      m_brightnessPoints[i].end   = endPoint;
   }
}

ColorScale::~ColorScale()
{

}


SpecAnLedTypes::tRgbColor ColorScale::getColor(uint16_t value, float brightness)
{
   int colorIndex = pointIndex(m_colorPoints.data(), m_colorPoints.size(), value);
   int brghtIndex = pointIndex(m_brightnessPoints.data(), m_brightnessPoints.size(), value);

   float desiredBrightness = getScaledValue(m_brightness, m_brightnessPoints, brghtIndex, value);

   float red   = getScaledValue(  m_red, m_colorPoints, colorIndex, value);
   float green = getScaledValue(m_green, m_colorPoints, colorIndex, value);
   float blue  = getScaledValue( m_blue, m_colorPoints, colorIndex, value);

   float startBrightness = sqrtf(red*red + green*green + blue*blue);
   float brightnessScalar = desiredBrightness * brightness / startBrightness;

   red   *= brightnessScalar;
   green *= brightnessScalar;
   blue  *= brightnessScalar;

   if(red   > 255) red   = 255;
   if(green > 255) green = 255;
   if(blue  > 255) blue  = 255;

   SpecAnLedTypes::tRgbColor retVal;
   retVal.rgb.r = (uint8_t)(red+0.5);
   retVal.rgb.g = (uint8_t)(green+0.5);
   retVal.rgb.b = (uint8_t)(blue+0.5);
   return retVal;
}


// Returns index in points that value is within.
int ColorScale::pointIndex(const tPointRange* points, int numPoints, uint16_t value)
{
   if(numPoints > 1)
   {
      int tryIndex = numPoints >> 1;
      const tPointRange* tryPoints = &points[tryIndex];
      if(value < tryPoints->start)
      {
         // Before try point.
         return pointIndex(points, tryIndex, value);
      }
      else if (value >= tryPoints->end)
      {
         // After try point.
         return pointIndex(tryPoints, numPoints-tryIndex, value) + tryIndex;
      }
      else
      {
         return tryIndex;
      }
   }
   else
   {
      assert(value >= points[0].start && value < points[0].end);
   }
   return 0;
}


float ColorScale::getScaledValue(std::vector<tValueRange>& values, std::vector<tPointRange>& points, int index, uint16_t value)
{
   tPointRange* pointEntry = &points[index];
   tValueRange* valueEntry = &values[index];

   assert(pointEntry->start <= value && pointEntry->end > value);

   int32_t pointRange = pointEntry->end - pointEntry->start;
   float    valueRange = valueEntry->end - valueEntry->start;

   int32_t passedStart = value - pointEntry->start;

   return valueEntry->start + (float)passedStart * valueRange / (float)pointRange;
}

