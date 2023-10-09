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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include "ledStrip.h"
#include "colorGradient.h"
#include "colorScale.h"
#include "gradientToScale.h"

// LED Stuff
#define DEFAULT_NUM_LEDS (60)
static std::shared_ptr<LedStrip> g_ledStrip;

////////////////////////////////////////////////////////////////////////////////

static void cleanUpBeforeExit()
{
   // Turn off all the LEDs in the LED strip.
   g_ledStrip.reset();
}

////////////////////////////////////////////////////////////////////////////////

static void signalHandler(int signum)
{
   cleanUpBeforeExit();
   exit(signum); 
}

////////////////////////////////////////////////////////////////////////////////

void fillInLedStrip(std::shared_ptr<LedStrip> ledStrip, ColorGradient& grad)
{
   std::vector<ColorScale::tColorPoint> colors;
   auto numLeds = ledStrip->getNumLeds();
   SpecAnLedTypes::tRgbVector ledColors(numLeds);
   auto gradVect = grad.getGradient();

   Convert::convertGradientToScale(gradVect, colors);
   
   std::vector<ColorScale::tBrightnessPoint> brightPoints{{1,0},{1,1}}; // Full brightness.
   ColorScale colorScale(colors, brightPoints);

   float deltaBetweenPoints = (float)65535/(float)(numLeds-1);
   for(size_t i = 0 ; i < numLeds; ++i)
   {
      ledColors[i] = colorScale.getColor((float)i * deltaBetweenPoints, 1.0);
   }

   ledStrip->set(ledColors);
}

////////////////////////////////////////////////////////////////////////////////

int main(void)
{
   // Setup Signal Handler for ctrl+c
   signal(SIGINT, signalHandler);

   // Setup LED strip.
   g_ledStrip.reset(new LedStrip(DEFAULT_NUM_LEDS, LedStrip::GRB));
   g_ledStrip->clear();

   // Define a simple gradient and display it.
   ColorGradient::tGradient gradPoints;
   ColorGradient::tGradientPoint gradPoint;
   gradPoint.hue        = 0.0;
   gradPoint.saturation = 1.0;
   gradPoint.lightness  = 1.0;
   gradPoint.position   = 0.0;
   gradPoint.reach      = 0.0;

   // Point 1
   gradPoint.hue        = 0.0;
   gradPoint.position   = 0.0;
   gradPoints.push_back(gradPoint);

   // Point 2
   gradPoint.hue        = 0.333333333333333333;
   gradPoint.position   = 0.5;
   gradPoints.push_back(gradPoint);

   // Point 3
   gradPoint.hue        = 0.666666666666666667;
   gradPoint.position   = 1.0;
   gradPoints.push_back(gradPoint);


   ColorGradient grad(gradPoints);
   fillInLedStrip(g_ledStrip, grad);

   std::this_thread::sleep_for(std::chrono::seconds(0x7FFFFFFF));
   return 0;
}