/* Copyright 2023 Dan Williams. All Rights Reserved.
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
#include "AmbientDisplay.h"
#include "gradientToScale.h"


AmbientDisplay::AmbientDisplay(ColorGradient::tGradient& grad, ColorScale::tBrightnessScale& brightness):
   m_grad_orig(grad.begin(), grad.end()),
   m_grad_current(grad.begin(), grad.end()),
   m_bright_orig(brightness.begin(), brightness.end()),
   m_bright_current(brightness.begin(), brightness.end())
{
}

AmbientDisplay::~AmbientDisplay()
{

}

void AmbientDisplay::gradient_shift(float shiftValue)
{
   if(shiftValue == 0.0)
      return; // Early return since there is nothing to do.

   // Update the current shift value.
   m_gradShiftVal = getNewShiftValue(m_gradShiftVal, shiftValue);
   size_t finalSize = m_grad_orig.size();

   // Determine where the new start point will be after the shift is applied.
   size_t newBeginIndex = 0;
   bool shiftIsPositive = (m_gradShiftVal > 0);
   for(size_t i = 0; i < finalSize; ++i)
   {
      float shiftedValue = m_grad_orig[i].position + m_gradShiftVal;
      if( ( shiftIsPositive && shiftedValue >  1.0) || 
         (!shiftIsPositive && shiftedValue >= 0.0) )
      {
         newBeginIndex = i;
         break;
      }
   }

   // Re-order
   m_grad_current.assign(m_grad_orig.begin()+newBeginIndex, m_grad_orig.end());
   m_grad_current.insert(m_grad_current.end(), m_grad_orig.begin(), m_grad_orig.begin()+newBeginIndex);
   assert(m_grad_current.size() == finalSize);

   // Fix the Position Values that have be Shifted.
   for(size_t i = 0; i < finalSize; ++i)
   {
      m_grad_current[i].position += m_gradShiftVal;
      if(m_grad_current[i].position >= 1.0)
      {
         // A value of 1 is allowed, but only if it is the last point in the gradient.
         bool exactlyOne = (m_grad_current[i].position == 1.0);
         bool lastPoint = (i == (finalSize-1));
         if(!exactlyOne || !lastPoint)
            m_grad_current[i].position -= 1.0;
      }
      else if(m_grad_current[i].position < 0.0)
         m_grad_current[i].position += 1.0;
   }

   // See if new first and last values have to added (first must be at zero and last must be at one)
   auto beginVal = m_grad_current[0];
   auto endVal = m_grad_current[m_grad_current.size()-1];

   assert(beginVal.position >= 0.0);
   assert(endVal.position <= 1.0);

   if(beginVal.position > 0.0)
   {
      // Need to define a new beginning point that is actually at zero.
      float minPos = endVal.position - 1.0;
      float maxPos = beginVal.position;

      ColorGradient::tGradientPoint gradPoint;
      gradPoint.position   = 0.0;
      gradPoint.lightness  = getMidPoint(minPos, endVal.lightness,  maxPos, beginVal.lightness,  gradPoint.position);
      gradPoint.saturation = getMidPoint(minPos, endVal.saturation, maxPos, beginVal.saturation, gradPoint.position);
      gradPoint.reach      = getMidPoint(minPos, endVal.reach,      maxPos, beginVal.reach,      gradPoint.position);
      gradPoint.hue        = getHuePoint(minPos, endVal.hue,        maxPos, beginVal.hue,        gradPoint.position);
      m_grad_current.insert(m_grad_current.begin(), gradPoint);
   }
   if(endVal.position < 1.0)
   {
      // Need to define a new ending point that is actually one.
      float minPos = endVal.position;
      float maxPos = beginVal.position + 1.0;

      ColorGradient::tGradientPoint gradPoint;
      gradPoint.position   = 1.0;
      gradPoint.lightness  = getMidPoint(minPos, endVal.lightness,  maxPos, beginVal.lightness,  gradPoint.position);
      gradPoint.saturation = getMidPoint(minPos, endVal.saturation, maxPos, beginVal.saturation, gradPoint.position);
      gradPoint.reach      = getMidPoint(minPos, endVal.reach,      maxPos, beginVal.reach,      gradPoint.position);
      gradPoint.hue        = getHuePoint(minPos, endVal.hue,        maxPos, beginVal.hue,        gradPoint.position);
      m_grad_current.push_back(gradPoint);
   }
}

void AmbientDisplay::brightness_shift(float shiftValue)
{
   if(shiftValue != 0.0)
   {
      m_brightShiftVal = getNewShiftValue(m_brightShiftVal, shiftValue);

      // Start over from the original.
      m_bright_current.assign(m_bright_orig.begin(), m_bright_orig.end());
   }
}

void AmbientDisplay::toRgbVect(SpecAnLedTypes::tRgbVector& ledColors, size_t numLeds)
{
   std::vector<ColorScale::tColorPoint> colors;
   ledColors.resize(numLeds);

   Convert::convertGradientToScale(m_grad_current, colors);

   ColorScale colorScale(colors, m_bright_current);

   float deltaBetweenPoints = (float)65535/(float)(numLeds-1);
   for(size_t i = 0 ; i < numLeds; ++i)
   {
      ledColors[i] = colorScale.getColor((float)i * deltaBetweenPoints, 1.0);
   }
}

float AmbientDisplay::getNewShiftValue(float currentShift, float newShiftVal)
{
   currentShift += newShiftVal;
   if(currentShift > 1.0)
      currentShift = fmod(currentShift, 1.0); // Modulo to keep between 0.0 and 1.0
   else if(currentShift < -1.0)
      currentShift = -fmod(-currentShift, 1.0); // Don't remember how mod works on negative numbers to convert to positive before mod then convert back.
   return currentShift;
}

float AmbientDisplay::getMidPoint(float minPoint_pos, float minPoint_val, float maxPoint_pos, float maxPoint_val, float midPoint_pos)
{
   if(maxPoint_pos == minPoint_pos)
      return (minPoint_val + maxPoint_val) / 2.0 ; // Return early to prevent divide by zero. Just return the average between the two points.

   float midPointScaled = (midPoint_pos - minPoint_pos) / (maxPoint_pos - minPoint_pos);
   return midPointScaled * (maxPoint_val - minPoint_val) + minPoint_val;
}

float AmbientDisplay::getHuePoint(float minPoint_pos, float minPoint_hue, float maxPoint_pos, float maxPoint_hue, float midPoint_pos)
{
   float midPoint_hue = getMidPoint(minPoint_pos, minPoint_hue, maxPoint_pos, maxPoint_hue, midPoint_pos);

   float hueDelta = abs(maxPoint_hue - minPoint_hue);
   if(hueDelta > 0.5)
   {
      // The real mid point is half way around the hue circle
      midPoint_hue += 0.5;
      if(midPoint_hue >= 1.0)
         midPoint_hue -= 1.0;
   }

   return midPoint_hue;
}