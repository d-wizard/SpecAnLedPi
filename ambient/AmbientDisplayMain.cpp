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
#include "AmbientMovement.h"
#include "smartPlotMessage.h" // Debug Plotting

// LED Stuff
#define DEFAULT_NUM_LEDS (198)
static std::shared_ptr<LedStrip> g_ledStrip;

// Brightness Constants.
#define BRIGHTNESS_PATTERN_NUM_POINTS (51)
#define BRIGHTNESS_PATTERN_HI_LEVEL (.75)
#define BRIGHTNESS_PATTERN_LO_LEVEL (0.0)

// 
#define GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO (3.0)

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

int main(void)
{
   smartPlot_createFlushThread_withPriorityPolicy(200, 30, SCHED_FIFO);

   // Setup Signal Handler for ctrl+c
   signal(SIGINT, signalHandler);

   /////////////////////////////////////////////////////////////////////////////
   // Setup LED strip.
   /////////////////////////////////////////////////////////////////////////////
   g_ledStrip.reset(new LedStrip(DEFAULT_NUM_LEDS, LedStrip::GRB));
   g_ledStrip->clear();

   /////////////////////////////////////////////////////////////////////////////
   // Define the Gradient.
   /////////////////////////////////////////////////////////////////////////////
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

   /////////////////////////////////////////////////////////////////////////////
   // Define Brightness Scale
   /////////////////////////////////////////////////////////////////////////////
   WaveformGen<float> brightValGen(BRIGHTNESS_PATTERN_NUM_POINTS);
   brightValGen.Sinc(-100, 100);
   brightValGen.absoluteValue();
   brightValGen.scale(BRIGHTNESS_PATTERN_HI_LEVEL - BRIGHTNESS_PATTERN_LO_LEVEL);
   brightValGen.shift(BRIGHTNESS_PATTERN_LO_LEVEL);

   WaveformGen<float> brightPosGen(BRIGHTNESS_PATTERN_NUM_POINTS);
   brightPosGen.Linear(0, 1);

   g_brightnessPattern_base.resize(BRIGHTNESS_PATTERN_NUM_POINTS);
   for(int i = 0; i < BRIGHTNESS_PATTERN_NUM_POINTS; ++i)
   {
      g_brightnessPattern_base[i].brightness = brightValGen.getPoints()[i];
      g_brightnessPattern_base[i].startPoint = brightPosGen.getPoints()[i];
      // smartPlot_2D(&g_brightnessPattern_base[i].startPoint, E_FLOAT_32, &g_brightnessPattern_base[i].brightness, E_FLOAT_32, 1, 100, -1, "2D", "val");
   }

   /////////////////////////////////////////////////////////////////////////////
   // Setup the Display
   /////////////////////////////////////////////////////////////////////////////
   ColorGradient::DuplicateGradient(gradPoints, 2, true);
   ColorScale::DuplicateBrightness(g_brightnessPattern_base, int(GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO), false);

   AmbientDisplay ambDisp(gradPoints, g_brightnessPattern_base);

   /////////////////////////////////////////////////////////////////////////////
   // Setup the Display Movement
   /////////////////////////////////////////////////////////////////////////////
   AmbientMovementF::tAmbientMoveProps ambMovePropsSine;
   ambMovePropsSine.source = AmbientMovementF::E_AMB_MOVE_SRC__FIXED;
   ambMovePropsSine.transform = AmbientMovementF::E_AMB_MOVE_TYPE__SIN;
   ambMovePropsSine.fixed_incr = 0.01;
   AmbientMovementF ambMoveSin(ambMovePropsSine, 0.4/GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO);

   /////////////////////////////////////////////////////////////////////////////
   // Random Mod to the Brightness Speed
   /////////////////////////////////////////////////////////////////////////////
   AmbientMovementF::tAmbientMoveProps modBrightMoveProps;
   modBrightMoveProps.source = AmbientMovementF::E_AMB_MOVE_SRC__RANDOM;
   modBrightMoveProps.transform = AmbientMovementF::E_AMB_MOVE_TYPE__LINEAR;
   modBrightMoveProps.randType = AmbientMovementF::E_AMB_MOVE_RAND_DIST__NORMAL;
   modBrightMoveProps.rand_paramA = 1.0;
   modBrightMoveProps.rand_paramB = 0.5;


   /////////////////////////////////////////////////////////////////////////////
   // Main Loop
   /////////////////////////////////////////////////////////////////////////////
   uint32_t changeBrightSpeedCount = 0;
   while(1)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      ambDisp.toRgbVect(g_ledColorPattern_base, GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO*g_ledStrip->getNumLeds());
      g_ledColorPattern_base.resize(g_ledStrip->getNumLeds());
      g_ledStrip->set(g_ledColorPattern_base);
      ambDisp.gradient_shift(-0.0002);
      ambDisp.brightness_shift(ambMoveSin.move());

      if(++changeBrightSpeedCount == 33)
      {
         changeBrightSpeedCount = 0;
         ambMoveSin.scaleMovementScalar(modBrightMoveProps);
      }
   }

   return 0;
}