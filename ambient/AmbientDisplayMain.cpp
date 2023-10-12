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
#include "WaveformGen.h"
#include "AmbientDisplay.h"
#include "smartPlotMessage.h" // Debug Plotting


// 
#define GRADIENT_NUM_LEDS (45)
#define BRIGHTNESS_PATTERN_NUM_LEDS (45)
#define BRIGHTNESS_PATTERN_HI_LEVEL (1.0)
#define BRIGHTNESS_PATTERN_LO_LEVEL (0.0)

// LED Stuff
#define DEFAULT_NUM_LEDS (45)
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
   static const int numGradPoints = 10;
   for(int i = 0; i < numGradPoints; ++i)
   {
      gradPoint.hue = double(i) / double(numGradPoints-1);
      gradPoint.position = gradPoint.hue;
      gradPoints.push_back(gradPoint);
   }

   // Sinc func for brightness
   WaveformGen<float> brightValGen(BRIGHTNESS_PATTERN_NUM_LEDS);
   brightValGen.Sinc(-1000, 1000);
   brightValGen.absoluteValue();
   brightValGen.scale(BRIGHTNESS_PATTERN_HI_LEVEL - BRIGHTNESS_PATTERN_LO_LEVEL);
   brightValGen.shift(BRIGHTNESS_PATTERN_LO_LEVEL);
   brightValGen.quarterCircle_above();

   WaveformGen<float> brightPosGen(BRIGHTNESS_PATTERN_NUM_LEDS);
   brightPosGen.Linear(0, 1);

   g_brightnessPattern_base.resize(BRIGHTNESS_PATTERN_NUM_LEDS);
   for(int i = 0; i < BRIGHTNESS_PATTERN_NUM_LEDS; ++i)
   {
      g_brightnessPattern_base[i].brightness = brightValGen.getPoints()[i];
      g_brightnessPattern_base[i].startPoint = brightPosGen.getPoints()[i];
      smartPlot_2D(&g_brightnessPattern_base[i].startPoint, E_FLOAT_32, &g_brightnessPattern_base[i].brightness, E_FLOAT_32, 1, 100, -1, "2D", "val");
   }

   ColorGradient::DuplicateGradient(gradPoints, 2, true);
   ColorScale::DuplicateBrightness(g_brightnessPattern_base, 1, false);

   ColorGradient grad(gradPoints);
   gradToRgbVect(grad, g_ledColorPattern_base, g_brightnessPattern_base, g_ledStrip->getNumLeds());
   g_ledStrip->set(g_ledColorPattern_base);

   AmbientDisplay ambDisp(gradPoints, g_brightnessPattern_base);

   while(1)
   {
      std::this_thread::sleep_for(std::chrono::microseconds(10000));
      ambDisp.toRgbVect(g_ledColorPattern_base, g_ledStrip->getNumLeds());
      g_ledStrip->set(g_ledColorPattern_base);
      ambDisp.gradient_shift(-0.002);
      ambDisp.brightness_shift(0.008);
   }

   return 0;
}