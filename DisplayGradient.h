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
#include <memory>
#include "colorGradient.h"
#include "ledStrip.h"
#include "specAnLedPiTypes.h"
#include "potentiometerAdc.h"
#include "GradientUserCues.h"


class DisplayGradient
{
public:
   DisplayGradient(std::shared_ptr<ColorGradient> grad, std::shared_ptr<LedStrip> ledStrip, std::shared_ptr<PotentiometerKnob> brightKnob);

   void showGradient();

   void blinkAll();
   void blinkOne(int colorIndex);

   void fadeIn(int colorIndex);
   void fadeOut(int colorIndex);

   bool userCueDone();

private:
   // Make uncopyable
   DisplayGradient();
   DisplayGradient(DisplayGradient const&);
   void operator=(DisplayGradient const&);

   void fillInLedStrip(float constBrightnessLevel = -1.0);
   int colorIndexToLedIndex(int colorIndex);
   SpecAnLedTypes::tRgbVector getBlankLedColors();
   SpecAnLedTypes::tRgbColor getColorFromGrad(int index);

   std::shared_ptr<ColorGradient> m_grad;
   SpecAnLedTypes::tRgbVector m_ledColors;
   std::shared_ptr<LedStrip> m_ledStrip;
   std::shared_ptr<PotentiometerKnob> m_brightKnob;

   std::unique_ptr<GradientUserCues> m_cues;

};

