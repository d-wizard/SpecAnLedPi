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
#include <math.h>
#include "ledStrip.h"
#include "colorGradient.h"
#include "colorScale.h"
#include "gradientToScale.h"
#include "smartPlotMessage.h" // Debug Plotting

// 
#define GRADIENT_NUM_LEDS (60)
#define BRIGHTNESS_PATTERN_NUM_LEDS (60)
#define BRIGHTNESS_PATTERN_HI_LEVEL (1.0)
#define BRIGHTNESS_PATTERN_LO_LEVEL (0.0)

// LED Stuff
#define DEFAULT_NUM_LEDS (60)
static std::shared_ptr<LedStrip> g_ledStrip;

// Patterns
static SpecAnLedTypes::tRgbVector g_ledColorPattern_base;
static ColorScale::tBrightnessScale g_brightnessPattern_base;


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

void gradToRgbVect(ColorGradient& grad, SpecAnLedTypes::tRgbVector& ledColors, ColorScale::tBrightnessScale& brightPoints, size_t numLeds)
{
   std::vector<ColorScale::tColorPoint> colors;
   ledColors.resize(numLeds);
   auto gradVect = grad.getGradient();

   Convert::convertGradientToScale(gradVect, colors);

   ColorScale colorScale(colors, brightPoints);

   float deltaBetweenPoints = (float)65535/(float)(numLeds-1);
   for(size_t i = 0 ; i < numLeds; ++i)
   {
      ledColors[i] = colorScale.getColor((float)i * deltaBetweenPoints, 1.0);
   }
}

////////////////////////////////////////////////////////////////////////////////

int main(void)
{
   smartPlot_createFlushThread_withPriorityPolicy(200, 30, SCHED_FIFO);

   // Setup Signal Handler for ctrl+c
   signal(SIGINT, signalHandler);

   // Setup LED strip.
   g_ledStrip.reset(new LedStrip(DEFAULT_NUM_LEDS, LedStrip::GRB));
   g_ledStrip->clear();

   // Define a simple gradient and display it.
   ColorGradient::tGradient gradPoints;
   ColorGradient::tGradientPoint gradPoint;
   gradPoint.saturation = 1.0;
   gradPoint.lightness  = 1.0;

   // Rainbow Gradient
   static const int numGradPoints = 30;
   for(int i = 0; i < numGradPoints; ++i)
   {
      gradPoint.hue = 0.6;//double(i) / double(numGradPoints-1);
      gradPoint.position = gradPoint.hue;
      gradPoints.push_back(gradPoint);
   }

   // Sinc func for brightness
   g_brightnessPattern_base.resize(BRIGHTNESS_PATTERN_NUM_LEDS);
   int centerOffset = (BRIGHTNESS_PATTERN_NUM_LEDS >> 1);
   float scalar = (BRIGHTNESS_PATTERN_HI_LEVEL - BRIGHTNESS_PATTERN_LO_LEVEL);
   for(int i = 0; i < BRIGHTNESS_PATTERN_NUM_LEDS; ++i)
   {
      float x = i - centerOffset;
      float brightness = (x == 0.0) ? 1.0 : abs(sin(x) / x);

      // Scale
      brightness = ( brightness * scalar + BRIGHTNESS_PATTERN_LO_LEVEL );
      g_brightnessPattern_base[i].brightness = brightness;
      g_brightnessPattern_base[i].startPoint = float(i) / float(BRIGHTNESS_PATTERN_NUM_LEDS-1);
   }

   ColorGradient grad(gradPoints);
   gradToRgbVect(grad, g_ledColorPattern_base, g_brightnessPattern_base, g_ledStrip->getNumLeds());
   g_ledStrip->set(g_ledColorPattern_base);

   while(1)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(60));
      g_ledColorPattern_base.push_back(g_ledColorPattern_base[0]);
      g_ledColorPattern_base.erase(g_ledColorPattern_base.begin());

      auto brightness0 = g_brightnessPattern_base[0].brightness;
      for(size_t i = 0; i < g_brightnessPattern_base.size()-1; ++i)
      {
         g_brightnessPattern_base[i].brightness = g_brightnessPattern_base[i+1].brightness;
      }
      g_brightnessPattern_base[g_brightnessPattern_base.size()-1].brightness = brightness0;

      gradToRgbVect(grad, g_ledColorPattern_base, g_brightnessPattern_base, g_ledStrip->getNumLeds());
      g_ledStrip->set(g_ledColorPattern_base);
   }

   return 0;
}