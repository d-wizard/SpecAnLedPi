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



DisplayGradient::DisplayGradient(std::shared_ptr<ColorGradient> grad, std::shared_ptr<LedStrip> ledStrip):
   m_grad(grad),
   m_ledStrip(ledStrip)
{
}

void DisplayGradient::fillInLedStrip(float constBrightnessLevel)
{
   std::vector<ColorScale::tColorPoint> colors;
   std::vector<ColorScale::tBrightnessPoint> bright;
   auto numLeds = m_ledStrip->getNumLeds();

   m_ledColors.resize(numLeds);

   auto gradVect = m_grad->getGradient();
   Convert::convertGradientToScale(gradVect, colors, bright);
   
   if(constBrightnessLevel >= 0.0 && constBrightnessLevel <= 1.0)
   {
      bright[0].brightness = bright[bright.size()-1].brightness * constBrightnessLevel; // TODO, need final brightness solution.
   }

   ColorScale colorScale(colors, bright);

   float deltaBetweenPoints = (float)65535/(float)(numLeds-1);
   for(size_t i = 0 ; i < numLeds; ++i)
   {
      m_ledColors[i] = colorScale.getColor((float)i * deltaBetweenPoints);
   }
}

int DisplayGradient::colorIndexToLedIndex(int colorIndex)
{
   int retVal = 0;

   auto numLeds = m_ledStrip->getNumLeds();
   auto gradVect = m_grad->getGradient();

   if(colorIndex >= 0 && colorIndex < gradVect.size())
   {
      double colorPos = gradVect[colorIndex].position;

      retVal = (colorPos * (double)(numLeds-1)) + 0.5;
      if(retVal < 0)
         retVal = 0;
      else if(retVal >= numLeds)
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

void DisplayGradient::showGradient()
{
   fillInLedStrip();
   m_ledStrip->set(m_ledColors);
}

void DisplayGradient::blinkAll()
{
   auto blink = getBlankLedColors();
   fillInLedStrip(1.0);

   auto numPoints = m_grad->getNumPoints();
   for(size_t i = 0; i < numPoints; ++i)
   {
      auto ledIndex = colorIndexToLedIndex(i);
      blink[ledIndex] = m_ledColors[ledIndex];
   }

   doBlink(blink);
}

void DisplayGradient::blinkOne(int colorIndex)
{
   auto blink = getBlankLedColors();
   fillInLedStrip(1.0);

   auto ledIndex = colorIndexToLedIndex(colorIndex);
   blink[ledIndex] = m_ledColors[ledIndex];

   doBlink(blink);
}

void DisplayGradient::doBlink(SpecAnLedTypes::tRgbVector& ledBlink)
{
   auto blank = getBlankLedColors();

   int numBlinks = 3;
   auto blinkTime = std::chrono::milliseconds(166);
   
   auto timerTime = std::chrono::steady_clock::now();
   for(int i = 0; i < numBlinks; ++i)
   {
      // Blink Off.
      m_ledStrip->set(blank);
      timerTime += blinkTime;
      std::this_thread::sleep_until(timerTime);

      // Blink On.
      m_ledStrip->set(ledBlink);
      timerTime += blinkTime;
      std::this_thread::sleep_until(timerTime);
   }

}

void DisplayGradient::fadeIn(int colorIndex)
{
   auto fade = getBlankLedColors();
   fillInLedStrip(1.0);

   auto ledIndex = colorIndexToLedIndex(colorIndex);
   fade[ledIndex] = m_ledColors[ledIndex];

   doFade(fade, true);
}

void DisplayGradient::fadeOut(int colorIndex)
{
   auto fade = getBlankLedColors();
   fillInLedStrip(1.0);

   auto ledIndex = colorIndexToLedIndex(colorIndex);
   fade[ledIndex] = m_ledColors[ledIndex];

   doFade(fade, false);
}

void DisplayGradient::doFade(SpecAnLedTypes::tRgbVector& ledFadeMax, bool fadeIn)
{
   auto ledColor = ledFadeMax;
   size_t numLeds = ledFadeMax.size();

   int numIterations = 40;
   auto fadeLen = std::chrono::nanoseconds(2*1000*1000*1000);

   auto timeBetween = fadeLen / numIterations;

   for(int i = 0; i < numIterations; ++i)
   {
      float doneness = (float)i / (float)(numIterations - 1);
      float notDoneness = 1.0 - doneness;
      float scaleFactor = fadeIn ? doneness : notDoneness;
      if(scaleFactor < 0.0)
         scaleFactor = 0.0;
      else if(scaleFactor > 1.0)
         scaleFactor = 1.0;
      
      for(size_t ledIndex = 0; ledIndex < numLeds; ++ledIndex)
      {
         ledColor[ledIndex].rgb.r = (float)ledFadeMax[ledIndex].rgb.r * scaleFactor;
         ledColor[ledIndex].rgb.g = (float)ledFadeMax[ledIndex].rgb.g * scaleFactor;
         ledColor[ledIndex].rgb.b = (float)ledFadeMax[ledIndex].rgb.b * scaleFactor;
      }
      m_ledStrip->set(ledColor);
      std::this_thread::sleep_for(timeBetween);
   }

}
