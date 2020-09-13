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

#include <stddef.h>
#include <vector>
#include "colorScale.h"

class ColorGradient
{
public:
   static constexpr float FULL_SCALE = 1.0;
   typedef enum
   {
      E_GRAD_HUE,
      E_GRAD_SATURATION,
      E_GRAD_LIGHTNESS,
      E_GRAD_POSITION,
      E_GRAD_REACH,
      E_GRAD_INVALID
   }eGradientOptions;

   typedef struct
   {
      float hue;
      float saturation;
      float lightness;
      float position; // Note: first must be position 0, last must be position 1
      float reach;
   }tGradientPoint;
   
   ColorGradient(size_t numPoints);
   virtual ~ColorGradient();

   void updateGradient(eGradientOptions option, float value, int pointIndex);
   void updateGradientDelta(eGradientOptions option, float delta, int pointIndex);

   std::vector<tGradientPoint> getGradient();

   size_t getNumPoints() {return m_gradPoints.size();}
private:
   ColorGradient(); // No default constructor.

   static constexpr float MIN_INCREMENT = 0.0078125; // 1/128

   std::vector<tGradientPoint> m_gradPoints;
   int m_numZones;

   void setHue(  float value, size_t pointIndex);
   void setSat(  float value, size_t pointIndex);
   void setLight(float value, size_t pointIndex);
   void setPos(  float value, size_t pointIndex);
   void setReach(float value, size_t pointIndex);

   // Keep track of previous orientation for when position / reach change.
   eGradientOptions m_previousOption = E_GRAD_INVALID;
   int m_previousIndex = -1;
   std::vector<tGradientPoint> m_previousGradPoints;
   void storePrevSettings(eGradientOptions option, int pointIndex);
   void locationChanged(size_t pointIndex);

   float getLoLimit(size_t pointIndex);
   float getHiLimit(size_t pointIndex);

   

};

