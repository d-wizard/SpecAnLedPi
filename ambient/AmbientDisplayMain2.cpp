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
#include "ledStrip.h"
#include "AmbDisp3SpotLights.h"
#include "smartPlotMessage.h" // Debug Plotting

// LED Stuff
#define DEFAULT_NUM_LEDS (296)
static std::shared_ptr<LedStrip> g_ledStrip;

static std::unique_ptr<AmbientLedStripBase> g_activeAmbient;

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

   g_activeAmbient = std::make_unique<AmbDisp3SpotLights>(g_ledStrip);

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