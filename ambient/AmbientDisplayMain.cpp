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
#include <string>
#include "ledStrip.h"
#include "SaveRestore.h"
#include "AmbDisp3SpotLights.h"
#include "smartPlotMessage.h" // Debug Plotting

// LED Stuff
#define DEFAULT_NUM_LEDS (296)
static std::shared_ptr<LedStrip> g_ledStrip;

static std::unique_ptr<AmbientLedStripBase> g_activeAmbient;
static std::unique_ptr<SaveRestoreJson> g_saveRestoreJson;

static std::string g_settingsJsonPath = "ambient/AmbientDisplaySettings.json";
static std::string g_presetJsonPath = "presets.json";
static int g_presetGradIndex = 0;

////////////////////////////////////////////////////////////////////////////////

static void cleanUpBeforeExit()
{
   g_activeAmbient.reset();

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

static void parseCmdLineArgs(int argc, char *argv[])
{
   // argv[1] is g_presetGradIndex, argv[2] is g_presetJsonPath, argv[3] is g_settingsJsonPath
   if(argc > 1)
      g_presetGradIndex = std::stoi(argv[1]);
   if(argc > 2)
      g_presetJsonPath = std::string(argv[2]);
   if(argc > 3)
      g_settingsJsonPath = std::string(argv[3]);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
   smartPlot_createFlushThread_withPriorityPolicy(200, 30, SCHED_FIFO);

   // Setup Signal Handler for ctrl+c
   signal(SIGINT, signalHandler);

   /////////////////////////////////////////////////////////////////////////////
   // Setup settings.
   /////////////////////////////////////////////////////////////////////////////
   auto gradient = ColorGradient::GetRainbowGradient(10, 0.6);
   if(argc > 1)
   {
      parseCmdLineArgs(argc, argv);
      g_saveRestoreJson = std::make_unique<SaveRestoreJson>(g_settingsJsonPath, g_presetJsonPath);
      gradient = g_saveRestoreJson->restore_gradient();
      if(g_presetGradIndex > 0)
      {
         for(int i = 0; i < g_presetGradIndex; ++i)
            gradient = g_saveRestoreJson->restore_gradientNext();
      }
      gradient = ColorGradient::ConvertToZeroReach(gradient); // The Ambient Display wants gradients with the reach value set to zero.
   }

   /////////////////////////////////////////////////////////////////////////////
   // Setup LED strip.
   /////////////////////////////////////////////////////////////////////////////
   g_ledStrip.reset(new LedStrip(DEFAULT_NUM_LEDS, LedStrip::GRB));
   g_ledStrip->clear();

   g_activeAmbient = std::make_unique<AmbDisp3SpotLights>(g_ledStrip, gradient);

   /////////////////////////////////////////////////////////////////////////////
   // Main Loop
   /////////////////////////////////////////////////////////////////////////////
   while(1)
   {
      // Do nothing
      std::this_thread::sleep_for(std::chrono::hours(240));
   }
   return 0;
}