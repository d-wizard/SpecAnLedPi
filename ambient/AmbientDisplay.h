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
#pragma once
#include <vector>
#include <memory>
#include "specAnLedPiTypes.h"
#include "colorGradient.h"
#include "colorScale.h"

////////////////////////////////////////////////////////////////////////////////
class AmbientDisplayBase
{
public:
   AmbientDisplayBase(){};
   virtual ~AmbientDisplayBase(){};
   AmbientDisplayBase(AmbientDisplayBase const&) = delete; void operator=(AmbientDisplayBase const&) = delete; // delete a bunch of constructors.
protected:
   float getNewShiftValue(float currentShift, float newShiftVal);
   float getMidPoint(float minPoint_pos, float minPoint_val, float maxPoint_pos, float maxPoint_val, float midPoint_pos);
   float getHuePoint(float minPoint_pos, float minPoint_hue, float maxPoint_pos, float maxPoint_hue, float midPoint_pos);
   float avgHuePoints(float point1, float point2);
};
////////////////////////////////////////////////////////////////////////////////
class AmbientDisplayGradient: public AmbientDisplayBase
{
public:
   AmbientDisplayGradient(const ColorGradient::tGradient& grad);
   virtual ~AmbientDisplayGradient();
   AmbientDisplayGradient() = delete; AmbientDisplayGradient(AmbientDisplayGradient const&) = delete; void operator=(AmbientDisplayGradient const&) = delete; // delete a bunch of constructors.

   void shift(float shiftValue); // Shift values should be between -1 and 1
   ColorGradient::tGradient& get(){return m_grad_current;}
private:
   ColorGradient::tGradient m_grad_orig;
   ColorGradient::tGradient m_grad_current;
   float m_gradShiftVal = 0.0;
};
////////////////////////////////////////////////////////////////////////////////
class AmbientDisplayBrightness: public AmbientDisplayBase
{
public:
   AmbientDisplayBrightness(const ColorScale::tBrightnessScale& brightness);
   virtual ~AmbientDisplayBrightness();
   AmbientDisplayBrightness() = delete; AmbientDisplayBrightness(AmbientDisplayBrightness const&) = delete; void operator=(AmbientDisplayBrightness const&) = delete; // delete a bunch of constructors.

   void shift(float shiftValue); // Shift values should be between -1 and 1
   ColorScale::tBrightnessScale& get(){return m_bright_current;}
private:
   ColorScale::tBrightnessScale m_bright_orig;
   ColorScale::tBrightnessScale m_bright_current;
   float m_brightShiftVal = 0.0;
};
////////////////////////////////////////////////////////////////////////////////
class AmbientDisplay
{
public:
   AmbientDisplay(const ColorGradient::tGradient& grad, const ColorScale::tBrightnessScale& brightness);
   AmbientDisplay(const ColorGradient::tGradient& grad, const std::vector<ColorScale::tBrightnessScale>& brightness);
   virtual ~AmbientDisplay();
   AmbientDisplay() = delete; AmbientDisplay(AmbientDisplay const&) = delete; void operator=(AmbientDisplay const&) = delete; // delete a bunch of constructors.

   void gradient_shift(float shiftValue); // Shift values should be between -1 and 1
   void brightness_shift(float shiftValue, size_t index = 0); // Shift values should be between -1 and 1

   void toRgbVect(SpecAnLedTypes::tRgbVector& ledColors, size_t numLeds);
private:
   AmbientDisplayGradient m_gradient;
   std::vector<std::unique_ptr<AmbientDisplayBrightness>> m_brightness_separate;
   ColorScale::tBrightnessScale m_brightness_combined;

   ColorScale::tBrightnessScale& combineBrightnessValues();
};
