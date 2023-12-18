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
#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <list>
#include "AmbientDisplay.h"
#include "gradientToScale.h"

// #define PLOT_BRIGHTNESS
#ifdef PLOT_BRIGHTNESS
#include "smartPlotMessage.h"
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static bool areTheyClose(float val1, float val2, float minDelta = ColorScale::MIN_RESOLUTION)
{
   bool close = false;
   float ratio = 0.0;
   if(val1 == val2)
   {
      close = true;
   }
   else if(fabs(val1 - val2) < minDelta)
   {
      close = true;
   }
   else if(val1 != 0.0)
   {
      ratio = val2/val1;
   }
   else
   {
      ratio = val1/val2;
   }

   // If the values are close, the ratio should be very near +1.0
   if(close == false && fabs(1.0 - ratio) < 0.00001)
   {
      close = true;
   }

   return close;
}

static bool setIfTheyAreClose(float& val, float desired)
{
   if(val != desired && areTheyClose(val, desired))
   {
      val = desired;
      return true;
   }
   return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

float AmbientDisplayBase::getNewShiftValue(float currentShift, float newShiftVal)
{
   currentShift += newShiftVal;
   if(currentShift > 1.0)
      currentShift = fmod(currentShift, 1.0); // Modulo to keep between 0.0 and 1.0
   else if(currentShift < -1.0)
      currentShift = -fmod(-currentShift, 1.0); // Don't remember how mod works on negative numbers to convert to positive before mod then convert back.
   return currentShift;
}

float AmbientDisplayBase::getMidPoint(float minPoint_pos, float minPoint_val, float maxPoint_pos, float maxPoint_val, float midPoint_pos)
{
   if(maxPoint_pos == minPoint_pos)
      return (minPoint_val + maxPoint_val) / 2.0 ; // Return early to prevent divide by zero. Just return the average between the two points.

   float midPointScaled = (midPoint_pos - minPoint_pos) / (maxPoint_pos - minPoint_pos);
   return midPointScaled * (maxPoint_val - minPoint_val) + minPoint_val;
}

float AmbientDisplayBase::getHuePoint(float minPoint_pos, float minPoint_hue, float maxPoint_pos, float maxPoint_hue, float midPoint_pos)
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

float AmbientDisplayBase::avgHuePoints(float point1, float point2)
{
   float avgHue = (point1 + point2) / 2.0;
   float hueDelta = abs(point1 - point2);
   if(hueDelta > 0.5)
   {
      // The real mid point is half way around the hue circle
      avgHue += 0.5;
      if(avgHue >= 1.0)
         avgHue -= 1.0;
   }
   return avgHue;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

AmbientDisplayGradient::AmbientDisplayGradient(const ColorGradient::tGradient& grad):
   m_grad_orig(grad.begin(), grad.end()),
   m_grad_current(grad.begin(), grad.end())
{
}

AmbientDisplayGradient::~AmbientDisplayGradient()
{

}

void AmbientDisplayGradient::shift(float shiftValue)
{
   if(shiftValue == 0.0)
      return; // Early return since there is nothing to do.

   // Update the current shift value.
   m_gradShiftVal = getNewShiftValue(m_gradShiftVal, shiftValue);
   size_t origGradSize = m_grad_orig.size();

   // Determine where the new start point will be after the shift is applied.
   size_t newBeginIndex = 0;
   bool shiftIsPositive = (m_gradShiftVal > 0);
   for(size_t i = 0; i < origGradSize; ++i)
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
   assert(m_grad_current.size() == origGradSize);

   // Fix the Position Values that have be Shifted.
   for(size_t i = 0; i < origGradSize; ++i)
   {
      m_grad_current[i].position += m_gradShiftVal;
      if(m_grad_current[i].position >= 1.0)
      {
         // A value of 1 is allowed, but only if it is the last point in the gradient.
         bool exactlyOne = (m_grad_current[i].position == 1.0);
         bool lastPoint = (i == (origGradSize-1));
         if(!exactlyOne || !lastPoint)
            m_grad_current[i].position -= 1.0;
      }
      else if(m_grad_current[i].position < 0.0)
         m_grad_current[i].position += 1.0;
   }

   // Check for duplicates
   if(m_grad_current.size() > 1)
   {
      for(auto iter = m_grad_current.begin(); iter != m_grad_current.end();)
      {
         auto nextIter = iter+1;
         if( (nextIter != m_grad_current.end()) && 
             (areTheyClose(iter->position, nextIter->position)) )
         {
            // The point should pretty much be the same, but even so lets average the 2 points.
            nextIter->lightness  = (iter->lightness  + nextIter->lightness)  / 2.0;
            nextIter->saturation = (iter->saturation + nextIter->saturation) / 2.0;
            nextIter->reach      = (iter->reach      + nextIter->reach)      / 2.0;
            nextIter->hue        = avgHuePoints(iter->hue, nextIter->hue);

            iter = m_grad_current.erase(iter);
         }
         else
         {
            ++iter;
         }
      }
   }

   // Fix first and last.
   setIfTheyAreClose(m_grad_current[0].position, 0.0);
   setIfTheyAreClose(m_grad_current[m_grad_current.size()-1].position, 1.0);

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

AmbientDisplayBrightness::AmbientDisplayBrightness(const ColorScale::tBrightnessScale& brightness):
   m_bright_orig(brightness.begin(), brightness.end()),
   m_bright_current(brightness.begin(), brightness.end())
{
}

AmbientDisplayBrightness::~AmbientDisplayBrightness()
{

}

void AmbientDisplayBrightness::shift(float shiftValue)
{
   if(shiftValue == 0.0)
      return; // Early return since there is nothing to do.

   // Update the current shift value.
   m_brightShiftVal = getNewShiftValue(m_brightShiftVal, shiftValue);
   size_t origBrightSize = m_bright_orig.size();

   // Determine where the new start point will be after the shift is applied.
   size_t newBeginIndex = 0;
   bool shiftIsPositive = (m_brightShiftVal > 0);
   for(size_t i = 0; i < origBrightSize; ++i)
   {
      float shiftedValue = m_bright_orig[i].startPoint + m_brightShiftVal;
      if( ( shiftIsPositive && shiftedValue >  1.0) || 
         (!shiftIsPositive && shiftedValue >= 0.0) )
      {
         newBeginIndex = i;
         break;
      }
   }

   // Re-order
   m_bright_current.assign(m_bright_orig.begin()+newBeginIndex, m_bright_orig.end());
   m_bright_current.insert(m_bright_current.end(), m_bright_orig.begin(), m_bright_orig.begin()+newBeginIndex);
   assert(m_bright_current.size() == origBrightSize);

   // Fix the Position Values that have be Shifted.
   for(size_t i = 0; i < origBrightSize; ++i)
   {
      m_bright_current[i].startPoint += m_brightShiftVal;
      if(m_bright_current[i].startPoint >= 1.0)
      {
         // A value of 1 is allowed, but only if it is the last point in the brightness scale.
         bool exactlyOne = (m_bright_current[i].startPoint == 1.0);
         bool lastPoint = (i == (origBrightSize-1));
         if(!exactlyOne || !lastPoint)
            m_bright_current[i].startPoint -= 1.0;
      }
      else if(m_bright_current[i].startPoint < 0.0)
         m_bright_current[i].startPoint += 1.0;
   }

   // Check for duplicates
   if(m_bright_current.size() > 1)
   {
      for(auto iter = m_bright_current.begin(); iter != m_bright_current.end();)
      {
         auto nextIter = iter+1;
         if( (nextIter != m_bright_current.end()) && 
             (areTheyClose(iter->startPoint, nextIter->startPoint)) )
         {
            // The point should pretty much be the same, but even so lets average the 2 points.
            nextIter->brightness = (iter->brightness + nextIter->brightness) / 2.0;
            iter = m_bright_current.erase(iter);
         }
         else
         {
            ++iter;
         }
      }
   }

   // Fix first and last.
   setIfTheyAreClose(m_bright_current[0].startPoint, 0.0);
   setIfTheyAreClose(m_bright_current[m_bright_current.size()-1].startPoint, 1.0);

   // See if new first and last values have to added (first must be at zero and last must be at one)
   auto beginVal = m_bright_current[0];
   auto endVal = m_bright_current[m_bright_current.size()-1];

   assert(beginVal.startPoint >= 0.0);
   assert(endVal.startPoint <= 1.0);

   if(beginVal.startPoint > 0.0)
   {
      // Need to define a new beginning point that is actually at zero.
      float minPos = endVal.startPoint - 1.0;
      float maxPos = beginVal.startPoint;

      ColorScale::tBrightnessPoint brightPoint;
      brightPoint.startPoint = 0.0;
      brightPoint.brightness = getMidPoint(minPos, endVal.brightness, maxPos, beginVal.brightness, brightPoint.startPoint);
      m_bright_current.insert(m_bright_current.begin(), brightPoint);
   }
   if(endVal.startPoint < 1.0)
   {
      // Need to define a new ending point that is actually one.
      float minPos = endVal.startPoint;
      float maxPos = beginVal.startPoint + 1.0;

      ColorScale::tBrightnessPoint brightPoint;
      brightPoint.startPoint = 1.0;
      brightPoint.brightness  = getMidPoint(minPos, endVal.brightness, maxPos, beginVal.brightness, brightPoint.startPoint);
      m_bright_current.push_back(brightPoint);
   }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

AmbientDisplay::AmbientDisplay(size_t numGenPoints, size_t numLeds, const ColorGradient::tGradient& grad, const ColorScale::tBrightnessScale& brightness):
   m_numGenPoints(numGenPoints),
   m_numLeds(numLeds),
   m_gradient(grad)
{
   m_brightness_separate.push_back(std::make_unique<AmbientDisplayBrightness>(brightness));
}

AmbientDisplay::AmbientDisplay(size_t numGenPoints, size_t numLeds, const ColorGradient::tGradient& grad, const std::vector<ColorScale::tBrightnessScale>& brightness):
   m_numGenPoints(numGenPoints),
   m_numLeds(numLeds),
   m_gradient(grad)
{
   for(const auto& b : brightness)
   {
      m_brightness_separate.push_back(std::make_unique<AmbientDisplayBrightness>(b));
   }
}

AmbientDisplay::~AmbientDisplay()
{

}

void AmbientDisplay::gradient_shift(float shiftValue)
{
   m_gradient.shift(shiftValue);
}

void AmbientDisplay::brightness_shift(float shiftValue, size_t index)
{
   auto curSize = m_brightness_separate.size();
   if(curSize > 0 && index >= 0 && index < curSize)
   { 
      m_brightness_separate[index]->shift(shiftValue);
   }
}


void AmbientDisplay::toRgbVect(SpecAnLedTypes::tRgbVector& ledColors)
{
   ColorScale::tColorScale colors;

   Convert::convertGradientToScale(m_gradient.get(), colors);

   bool singleBrightScale = (m_brightness_separate.size() == 1);
   ColorScale colorScale(colors, singleBrightScale ? m_brightness_separate[0]->get() : combineBrightnessValues(1.0 / float(m_numLeds-1)));

   float deltaBetweenPoints = (float)65535/(float)(m_numGenPoints-1);
   ledColors.resize(m_numLeds);
   for(size_t i = 0 ; i < m_numLeds; ++i)
   {
      ledColors[i] = colorScale.getColor((float)i * deltaBetweenPoints, 1.0);
   }

#ifdef PLOT_BRIGHTNESS
   std::vector<float> brightnessPlot(m_numLeds);
   std::vector<float> positionPlot(m_numLeds);
   float divisor = m_numLeds-1;
   for(size_t i = 0 ; i < m_numLeds; ++i)
   {
      brightnessPlot[i] = sqrt(ledColors[i].rgb.r * ledColors[i].rgb.r + ledColors[i].rgb.g * ledColors[i].rgb.g + ledColors[i].rgb.b * ledColors[i].rgb.b);
      positionPlot[i] = (float)i / divisor;
   }
   smartPlot_2D(positionPlot.data(), E_FLOAT_32, brightnessPlot.data(), E_FLOAT_32, m_numLeds, m_numLeds, 0, "Brightness", "Val");
#endif
}

ColorScale::tBrightnessScale& AmbientDisplay::combineBrightnessValues(float minBetweenPoints)
{
   // Combine. Combine values into a sortable list.
   std::list<ColorScale::tBrightnessPoint> combined;
   for(auto& brightness : m_brightness_separate)
   {
      auto& brightPoints = brightness->get();
      for(auto& brightPoint : brightPoints)
      {
         combined.push_back(brightPoint);
      }
   }

   // Sort.
   combined.sort();

   combineBrightnessValues_compute(combined, m_brightness_combined);

   return m_brightness_combined;
}

void AmbientDisplay::combineBrightnessValues_compute(const std::list<ColorScale::tBrightnessPoint>& allTheBrightPoints, ColorScale::tBrightnessScale& computedBrightness)
{
   const size_t numBright = m_brightness_separate.size();
   if(numBright < 1)
      return;

   computedBrightness.resize(allTheBrightPoints.size());

   // First One (just set brightness to the first brightness scale values).
   size_t brightIndex = 0;
   auto brightScale = m_brightness_separate[brightIndex]->get();
   size_t i = 0;
   for(auto& bp : allTheBrightPoints)
   {
      computedBrightness[i].startPoint = bp.startPoint;
      computedBrightness[i].brightness = combineBrightnessValues_getBrightVal(brightScale, bp.startPoint);
      ++i;
   }

   // The Rest (set brightness to the max brightness scale values).
   for(brightIndex = 1; brightIndex < numBright; ++brightIndex)
   {
      brightScale = m_brightness_separate[brightIndex]->get();
      i = 0;
      for(auto& bp : allTheBrightPoints)
      {
         computedBrightness[i].brightness = std::max(computedBrightness[i].brightness, combineBrightnessValues_getBrightVal(brightScale, bp.startPoint));
         ++i;
      }
   }

}

float AmbientDisplay::combineBrightnessValues_getBrightVal(const ColorScale::tBrightnessScale& singleBrightScale, float position_zeroToOne)
{
   if(position_zeroToOne < 0.0)
      position_zeroToOne = 0.0;
   else if(position_zeroToOne > 1.0)
      position_zeroToOne = 1.0;
   
   auto numBrightPoints = singleBrightScale.size();
   if(numBrightPoints < 1)
      return 0;
   else if(numBrightPoints < 2)
      return singleBrightScale[0].brightness;
      
   // TODO make this binary search.
   size_t firstBiggerIndex = numBrightPoints; // Set to invalid index (this will be checked for later in this function).
   for(size_t i = 0; i < numBrightPoints; ++i)
   {
      auto position = singleBrightScale[i].startPoint;
      if(position == position_zeroToOne)
      {
         return singleBrightScale[i].brightness; // Exact match, just return the correct value.
      }
      else if(position > position_zeroToOne)
      {
         firstBiggerIndex = i;
         break;
      }
   }

   // Linear Interpolate Between.
   if(firstBiggerIndex == 0)
   {
      return singleBrightScale[0].brightness; // TODO I think this is right.
   }
   else if(firstBiggerIndex >= numBrightPoints)
   {
      return singleBrightScale[numBrightPoints-1].brightness; // TODO I think this is right.
   }
   // else index is valid.

   float brightLo = singleBrightScale[firstBiggerIndex-1].brightness;
   float brightHi = singleBrightScale[firstBiggerIndex].brightness;
   float posLo = singleBrightScale[firstBiggerIndex-1].startPoint;
   float posHi = singleBrightScale[firstBiggerIndex].startPoint;
   float midPoint = (position_zeroToOne - posLo) / (posHi - posLo);

   return (brightHi - brightLo) * midPoint + brightLo;
}