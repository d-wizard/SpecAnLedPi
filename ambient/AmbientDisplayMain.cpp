/* Copyright 2023 - 2024 Dan Williams. All Rights Reserved.
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
#include <map>
#include <mutex>
#include <condition_variable>
#include "ledStrip.h"
#include "SaveRestore.h"
#include "AmbDisp3SpotLights.h"
#include "AmbRemoteControl.h"
#include "smartPlotMessage.h" // Debug Plotting

// LED Stuff
#define DEFAULT_NUM_LEDS (296)
static std::shared_ptr<LedStrip> g_ledStrip;

static std::unique_ptr<AmbientLedStripBase> g_activeAmbient;
static std::unique_ptr<SaveRestoreJson> g_saveRestoreJson;

static std::string g_settingsJsonPath = "ambient/AmbientDisplaySettings.json";
static std::string g_presetJsonPath = "presets.json";
static int g_presetGradIndex = -1;

// Gradient Display
static bool g_gradDisplay_displayGradient = false;

// Remote Control
static std::unique_ptr<AmbRemoteControl> g_remoteCtrl_worker;
static const uint16_t g_remoteCtrl_socketPort = 2070;
static bool g_remoteCtrl_msgReady = false;
static std::mutex g_remoteCtrl_mutex;
static std::condition_variable g_remoteCtrl_condVar;
static void newRemoteCtrlMsgReady();

// Alive
static bool g_stayAlive = true;

////////////////////////////////////////////////////////////////////////////////
typedef struct
{
   float gradToDisplayAtATime; // 1.0 works best for rainbow. 0.5 works best for Christmas.
   float gradSpeedScalar; // 0.1 is a good default
   bool  gradMirror;
}tAmbGradSettings;
static const tAmbGradSettings DEFAULT_GRAD_SETTINGS = {2.5, 0.3, true};
static const std::map<std::string, tAmbGradSettings> GRAD_SETTINGS = {
   {"xmas",           DEFAULT_GRAD_SETTINGS},
   {"halloween",      DEFAULT_GRAD_SETTINGS},
   {"merica",         DEFAULT_GRAD_SETTINGS},
   {"rainbow",        {1.0, 0.1, false}},
   {"rainbow_pastel", {2.0, 0.3, false}},
   {"fire",           DEFAULT_GRAD_SETTINGS},
   {"valentines",     DEFAULT_GRAD_SETTINGS},
   {"st_paddies",     DEFAULT_GRAD_SETTINGS},
   {"fall",           DEFAULT_GRAD_SETTINGS}
};


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
   g_stayAlive = false;
   newRemoteCtrlMsgReady(); // Fake out remote control message to wait up main loop.
   exit(signum); 
}

////////////////////////////////////////////////////////////////////////////////

static void parseCmdLineArgs(int argc, char *argv[])
{
   if(argc > 1 && (argv[1][0] == 'd' || (argv[1][0] == 'D')))
   {
      g_gradDisplay_displayGradient = true;
   }
   else
   {
      // argv[1] is g_presetGradIndex, argv[2] is g_presetJsonPath, argv[3] is g_settingsJsonPath
      if(argc > 1)
         g_presetGradIndex = std::stoi(argv[1]);
      if(argc > 2)
         g_presetJsonPath = std::string(argv[2]);
      if(argc > 3)
         g_settingsJsonPath = std::string(argv[3]);
   }
}


////////////////////////////////////////////////////////////////////////////////

static void displayGradient(ColorGradient::tGradient& gradient, size_t numDisplayLEDs, size_t totalLEDs)
{
   float brightVal = 0.25;
   ColorScale::tBrightnessScale brightness = {{brightVal,0},{brightVal,1}};
   AmbientDisplay gradToRgb(numDisplayLEDs, numDisplayLEDs, gradient, brightness);

   const size_t leds_total = totalLEDs;
   const size_t leds_grad = numDisplayLEDs;
   const size_t leds_right = (leds_total-leds_grad)/2;
   const size_t leds_left  = (leds_total-leds_grad-leds_right);

   SpecAnLedTypes::tRgbVector rgb;
   gradToRgb.toRgbVect(rgb);
   SpecAnLedTypes::tRgbColor black;
   black.u32 = SpecAnLedTypes::COLOR_BLACK;
   rgb.insert(rgb.begin(), leds_right, black);
   rgb.insert(rgb.end(), leds_left, black);

   g_ledStrip->set(rgb);
}

////////////////////////////////////////////////////////////////////////////////

static void setPresetGradByIndex(int index, ColorGradient::tGradient& gradient, std::string& gradName, tAmbGradSettings& gradSettings)
{
   gradient = g_saveRestoreJson->restore_gradient();
   if(index > 0)
   {
      for(int i = 0; i < index; ++i)
         gradient = g_saveRestoreJson->restore_gradientNext();
   }
   else if(index < 0)
   {
      index = -index; // Negate and call restore_gradientPrev
      for(int i = 0; i < index; ++i)
         gradient = g_saveRestoreJson->restore_gradientPrev();
   }
   gradient = ColorGradient::ConvertToZeroReach(gradient); // The Ambient Display wants gradients with the reach value set to zero.
   gradName = g_saveRestoreJson->getGradName();

   // Get the gradient specific settings (using the gradient name as the key).
   auto match = GRAD_SETTINGS.find(gradName);
   if(match != GRAD_SETTINGS.end())
      gradSettings = match->second;
   else
      gradSettings = DEFAULT_GRAD_SETTINGS;
}

////////////////////////////////////////////////////////////////////////////////

static bool setPresetGradByName(const std::string& desiredGradName, ColorGradient::tGradient& gradient, std::string& gradName, tAmbGradSettings& gradSettings)
{
   ColorGradient::tGradient tempGradient;
   std::string tempGradName;
   tAmbGradSettings tempGradSettings;

   std::string origGradName;
   setPresetGradByIndex(0, tempGradient, origGradName, tempGradSettings); // Get the current gradient name (need this to make sure not to loop forever if the desired name isn't available).

   // Search through the gradients until a match is found or we looped back again.
   bool found = (origGradName == desiredGradName);
   while(!found && tempGradName != origGradName)
   {
      setPresetGradByIndex(1, tempGradient, tempGradName, tempGradSettings);
      found = (tempGradName == desiredGradName);
   }

   // If found, set return values.
   if(found)
   {
      gradient = tempGradient;
      gradName = desiredGradName;
      gradSettings = tempGradSettings;
   }
   return found;
}

////////////////////////////////////////////////////////////////////////////////

static void newRemoteCtrlMsgReady()
{
   std::lock_guard<std::mutex> lock(g_remoteCtrl_mutex);
   g_remoteCtrl_msgReady = true;
   g_remoteCtrl_condVar.notify_all();
}

////////////////////////////////////////////////////////////////////////////////

static void processNewRemoteCtrlMsgs(ColorGradient::tGradient& gradient, std::string& gradName, tAmbGradSettings& gradSettings)
{
   bool lastCmd = false;
   while(!lastCmd)
   {
      AmbRemoteControl::tCmdAndVal cmdVal;
      lastCmd = g_remoteCtrl_worker->getRemoteCmd(cmdVal);
      switch(cmdVal.cmd)
      {
         case AmbRemoteControl::eCommands::E_GRADIENT_POS:
            setPresetGradByIndex(1, gradient, gradName, gradSettings);
         break;
         case AmbRemoteControl::eCommands::E_GRADIENT_NEG:
            setPresetGradByIndex(-1, gradient, gradName, gradSettings);
         break;
         case AmbRemoteControl::eCommands::E_GRADIENT_NAME:
            setPresetGradByName(cmdVal.val_str, gradient, gradName, gradSettings);
         break;
         case AmbRemoteControl::eCommands::E_INVALID_COMMAND:
            printf("[%s] - E_INVALID_COMMAND\n", __func__);
         break;
         default:
            printf("[%s] - Unsupported Cmd (%d)\n", __func__, (int)cmdVal.cmd);
         break;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
   // smartPlot_createFlushThread_withPriorityPolicy(200, 30, SCHED_FIFO);

   // Setup Signal Handler for ctrl+c
   signal(SIGINT, signalHandler);

   /////////////////////////////////////////////////////////////////////////////
   // Setup settings.
   /////////////////////////////////////////////////////////////////////////////
   g_saveRestoreJson = std::make_unique<SaveRestoreJson>(g_settingsJsonPath, g_presetJsonPath);
   auto gradient = ColorGradient::GetRainbowGradient(10, 1.0);
   std::string gradName = "GetRainbowGradient";
   tAmbGradSettings gradSettings = DEFAULT_GRAD_SETTINGS;
   if(argc > 1)
   {
      parseCmdLineArgs(argc, argv);
      setPresetGradByIndex(g_presetGradIndex, gradient, gradName, gradSettings);
   }

   // Start Remote Control
   g_remoteCtrl_worker = std::make_unique<AmbRemoteControl>(g_remoteCtrl_socketPort, newRemoteCtrlMsgReady);

   /////////////////////////////////////////////////////////////////////////////
   // Setup LED strip.
   /////////////////////////////////////////////////////////////////////////////
   g_ledStrip = std::make_shared<LedStrip>(DEFAULT_NUM_LEDS, LedStrip::GRB);
   g_ledStrip->clear();
   
   /////////////////////////////////////////////////////////////////////////////
   // Main Loop
   /////////////////////////////////////////////////////////////////////////////
   while(g_stayAlive)
   {
      g_ledStrip->clear();
      g_activeAmbient.reset();

      if(g_gradDisplay_displayGradient)
      {
         // Special Mode. Just display the gradient.
         displayGradient(gradient, unsigned(float(DEFAULT_NUM_LEDS)/4.0), DEFAULT_NUM_LEDS);
      }
      else
      {
         // Normal Mode.
         g_activeAmbient = std::make_unique<AmbDisp3SpotLights>(g_ledStrip, gradient, gradSettings.gradToDisplayAtATime, gradSettings.gradSpeedScalar, gradSettings.gradMirror, gradName);
      }

      // Wait for remote control message
      {
         std::unique_lock<std::mutex> lock(g_remoteCtrl_mutex);
         g_remoteCtrl_condVar.wait(lock);
         if(g_stayAlive)
         {
            lock.unlock();
            processNewRemoteCtrlMsgs(gradient, gradName, gradSettings);
         }
      }
   }

   return 0;
}