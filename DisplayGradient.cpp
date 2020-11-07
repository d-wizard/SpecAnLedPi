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
#include <chrono>
#include <thread>
#include "DisplayGradient.h"
#include "gradientToScale.h"



DisplayGradient::DisplayGradient(std::shared_ptr<ColorGradient> grad, std::shared_ptr<LedStrip> ledStrip, std::shared_ptr<PotentiometerKnob> brightKnob):
   m_grad(grad),
   m_ledStrip(ledStrip),
   m_brightKnob(brightKnob),
   m_cues(new GradientUserCues(ledStrip, brightKnob))
{
}

void DisplayGradient::fillInLedStrip(float constBrightnessLevel)
{
   std::vector<ColorScale::tColorPoint> colors;
   auto numLeds = m_ledStrip->getNumLeds();
   m_ledColors.resize(numLeds);
   auto gradVect = m_grad->getGradient();
   float brightnessPot = m_brightKnob->getFlt();

   if(constBrightnessLevel >= 0.0 && constBrightnessLevel <= 1.0)
   {
      for(auto& gradVal : gradVect)
      {
         gradVal.lightness = constBrightnessLevel;
      }
      brightnessPot = 1.0;
   }

   Convert::convertGradientToScale(gradVect, colors);
   
   std::vector<ColorScale::tBrightnessPoint> brightPoints{{1,0},{1,1}}; // Full brightness.
   ColorScale colorScale(colors, brightPoints);

   float deltaBetweenPoints = (float)65535/(float)(numLeds-1);
   for(size_t i = 0 ; i < numLeds; ++i)
   {
      m_ledColors[i] = colorScale.getColor((float)i * deltaBetweenPoints, brightnessPot);
   }
}

int DisplayGradient::colorIndexToLedIndex(int colorIndex)
{
   int retVal = 0;

   auto numLeds = m_ledStrip->getNumLeds();
   auto gradVect = m_grad->getGradient();

   if(colorIndex >= 0 && colorIndex < (signed)gradVect.size())
   {
      double colorPos = gradVect[colorIndex].position;

      retVal = (colorPos * (double)(numLeds-1));
      if(retVal < 0)
         retVal = 0;
      else if(retVal >= (signed)numLeds)
         retVal = numLeds - 1;
   }

   return retVal;
}

SpecAnLedTypes::tRgbVector DisplayGradient::getBlankLedColors()
{
   SpecAnLedTypes::tRgbVector retVal(m_ledStrip->getNumLeds());
   for(auto& led : retVal)
   {
      led.u32 = SpecAnLedTypes::COLOR_BLACK;
   }
   return retVal;
}

SpecAnLedTypes::tRgbColor DisplayGradient::getColorFromGrad(int index)
{
   return Convert::convertGradientPointToRGB(m_grad->getGradientPoint(index));
}

void DisplayGradient::showGradient()
{
   fillInLedStrip();
   m_ledStrip->set(m_ledColors);
}

void DisplayGradient::blinkAll()
{
   auto blink = getBlankLedColors();

   auto numPoints = m_grad->getNumPoints();
   for(size_t i = 0; i < numPoints; ++i)
   {
      int ledIndex = colorIndexToLedIndex(i);
      blink[ledIndex] = getColorFromGrad(i);
   }

   m_cues->startBlink(blink, (size_t)-1);
}

void DisplayGradient::blinkOne(int colorIndex)
{
   auto blink = getBlankLedColors();

   int ledIndex = colorIndexToLedIndex(colorIndex);
   blink[ledIndex] = getColorFromGrad(colorIndex);

   m_cues->startBlink(blink, ledIndex);
}

void DisplayGradient::fadeIn(int colorIndex)
{
   auto fade = getBlankLedColors();

   int ledIndex = colorIndexToLedIndex(colorIndex);
   fade[ledIndex] = getColorFromGrad(colorIndex);

   m_cues->startFade(fade, ledIndex, true);
}

void DisplayGradient::fadeOut(int colorIndex)
{
   auto fade = getBlankLedColors();

   int ledIndex = colorIndexToLedIndex(colorIndex);
   fade[ledIndex] = getColorFromGrad(colorIndex);

   m_cues->startFade(fade, ledIndex, false);
}


bool DisplayGradient::userCueDone()
{
   return m_cues->userCueJustFinished();
}


