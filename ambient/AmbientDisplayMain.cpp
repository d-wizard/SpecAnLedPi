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
#define DEFAULT_NUM_LEDS (296)
static std::shared_ptr<LedStrip> g_ledStrip;

// Brightness Constants.
#define BRIGHTNESS_PATTERN_NUM_POINTS (51)
#define BRIGHTNESS_PATTERN_HI_LEVEL (.35)
#define BRIGHTNESS_PATTERN_LO_LEVEL (0.0)

// 
#define GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO (3.0)

// Patterns
static SpecAnLedTypes::tRgbVector g_ledColorPattern_base;
static ColorScale::tBrightnessScale g_brightnessPattern_base;

// Types
typedef float AmbDispFltType; // Use a typedef to easily switch between float and double.

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
   WaveformGen<AmbDispFltType> brightValGen(BRIGHTNESS_PATTERN_NUM_POINTS);
   brightValGen.Sinc(-100, 100);
   brightValGen.absoluteValue();
   brightValGen.scale(BRIGHTNESS_PATTERN_HI_LEVEL - BRIGHTNESS_PATTERN_LO_LEVEL);
   brightValGen.shift(BRIGHTNESS_PATTERN_LO_LEVEL);

   WaveformGen<AmbDispFltType> brightPosGen(BRIGHTNESS_PATTERN_NUM_POINTS);
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

   AmbientDisplay ambDisp(gradPoints, {g_brightnessPattern_base, g_brightnessPattern_base, g_brightnessPattern_base});

   /////////////////////////////////////////////////////////////////////////////
   // Setup the Brightness Movement
   /////////////////////////////////////////////////////////////////////////////
   auto brightMoveSrc0 = std::make_shared<AmbientMovement::LinearSource<AmbDispFltType>>(0.001);
   auto brightMoveSrc1 = std::make_shared<AmbientMovement::LinearSource<AmbDispFltType>>(0.0008381984);
   auto brightMoveSrc2 = std::make_shared<AmbientMovement::LinearSource<AmbDispFltType>>(0.0003984116);
   auto brightTransforms_saw    = std::make_shared<AmbientMovement::SawTransform<AmbDispFltType>>();
   auto brightTransforms_scale  = std::make_shared<AmbientMovement::LinearTransform<AmbDispFltType>>(0.4/GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO);
   std::vector<AmbientMovement::TransformPtr<AmbDispFltType>> brightTransformsSawScale = {brightTransforms_saw, brightTransforms_scale};
   std::vector<AmbientMovement::TransformPtr<AmbDispFltType>> brightTransformsLinScale = {brightTransforms_scale};
   AmbientMovement::Generator<AmbDispFltType> brightMoveGen0(brightMoveSrc0, brightTransformsSawScale);
   AmbientMovement::Generator<AmbDispFltType> brightMoveGen1(brightMoveSrc1, brightTransformsLinScale);
   AmbientMovement::Generator<AmbDispFltType> brightMoveGen2(brightMoveSrc2, brightTransformsLinScale);

   /////////////////////////////////////////////////////////////////////////////
   // Add some randomness to the brightness movement speed
   /////////////////////////////////////////////////////////////////////////////
   AmbientMovement::Generator<AmbDispFltType> brightMoveSpeedModGen(
      std::make_shared<AmbientMovement::RandUniformSource<AmbDispFltType>>(0.5, 1.25), // Generator
      std::make_shared<AmbientMovement::RandNegateTransform<AmbDispFltType>>());

   /////////////////////////////////////////////////////////////////////////////
   // Main Loop
   /////////////////////////////////////////////////////////////////////////////
   uint32_t brightMoveSpeedModGenCount = 0;
   auto numLeds = g_ledStrip->getNumLeds();
   while(1)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      ambDisp.toRgbVect(GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO*numLeds, g_ledColorPattern_base, numLeds);
      g_ledStrip->set(g_ledColorPattern_base);
      ambDisp.gradient_shift(-0.002);
      ambDisp.brightness_shift(brightMoveGen0.getNextDelta(), 0);
      ambDisp.brightness_shift(brightMoveGen1.getNextDelta(), 1);
      ambDisp.brightness_shift(brightMoveGen2.getNextDelta(), 2);

      if(++brightMoveSpeedModGenCount == 33)
      {
         brightMoveSpeedModGenCount = 0;
         brightMoveSrc0->scaleIncr(brightMoveSpeedModGen.getNext());
         brightMoveSrc1->scaleIncr(brightMoveSpeedModGen.getNext());
         brightMoveSrc2->scaleIncr(brightMoveSpeedModGen.getNext());
      }
   }

   return 0;
}