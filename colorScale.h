/* Copyright 2020, 2023 Dan Williams. All Rights Reserved.
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

#include <stddef.h>
#include "specAnLedPiTypes.h"

class ColorScale
{
public:

   static constexpr float FULL_SCALE = 65536.0f;
   static constexpr float MIN_RESOLUTION = 1.0f / FULL_SCALE;

   typedef struct 
   {
      SpecAnLedTypes::tRgbColor color;
      float startPoint; // Inclusive (0 to 1)
   }tColorPoint;
   
   typedef struct tBrightnessPoint
   {
      float brightness; // 0 to 1
      float startPoint; // Inclusive (0 to 1)
      
      bool operator>(const tBrightnessPoint& rhs) const
      {
         return startPoint > rhs.startPoint;
      }
      bool operator<(const tBrightnessPoint& rhs) const
      {
         return startPoint < rhs.startPoint;
      }
      bool operator>=(const tBrightnessPoint& rhs) const
      {
         return !(operator<(rhs));
      }
      bool operator<=(const tBrightnessPoint& rhs) const
      {
         return !(operator>(rhs));
      }
   }tBrightnessPoint;

   typedef std::vector<tColorPoint> tColorScale;
   typedef std::vector<tBrightnessPoint> tBrightnessScale;

   static void DuplicateColorScale(tColorScale& colorScaleInOut, unsigned numCopies, bool mirror);
   static void DuplicateBrightness(tBrightnessScale& brightInOut, unsigned numCopies, bool mirror);

   ColorScale(tColorScale& colorPoints, tBrightnessScale& brightnessPoints);
   virtual ~ColorScale();

   SpecAnLedTypes::tRgbColor getColor(uint16_t value, float brightness, bool skipBrightnessNormalization = false);

private:
   // Make uncopyable
   ColorScale();
   ColorScale(ColorScale const&);
   void operator=(ColorScale const&);

   typedef struct
   {
      int32_t start; // Inclusive
      int32_t end;   // Exclusive
   }tPointRange;
   
   typedef struct
   {
      float start;
      float end;
   }tValueRange;

   // Returns index in points that value is within.
   int pointIndex(const tPointRange* points, int numPoints, uint16_t value);

   float getScaledValue(std::vector<tValueRange>& values, std::vector<tPointRange>& points, int index, uint16_t value);

   std::vector<tValueRange> m_red;
   std::vector<tValueRange> m_green;
   std::vector<tValueRange> m_blue;
   std::vector<tPointRange> m_colorPoints;

   std::vector<tValueRange> m_brightness;
   std::vector<tPointRange> m_brightnessPoints;



};



