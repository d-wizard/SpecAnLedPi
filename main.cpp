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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <signal.h>
#include <math.h>
#include <memory> // unique_ptr
#include <thread>
#include <mutex>
#include <condition_variable>
#include "specAnLedPiTypes.h"
#include "ledStrip.h"
#include "gradientChangeThread.h"
#include "AudioLeds.h"
#include "rotaryEncoder.h"
#include "potentiometerAdc.h"
#include "potentiometerKnob.h"
#include "ThreadPriorities.h"
#include "wiringPi.h"
#include "SaveRestore.h"


// LED Stuff
#define NUM_LEDS (30)
static std::shared_ptr<LedStrip> ledStrip;

// Thread for Updating the Color Gradient
static std::unique_ptr<GradChangeThread> gradChangeThread;

static std::unique_ptr<AudioLeds> audioLed;

// Thread for Polling the Current state of the Rotary Encoders.
static std::atomic<bool> rotaryEncPollThreadActive;
static std::unique_ptr<std::thread> checkRotaryThread;

// The Rotary Encoders.
static std::shared_ptr<RotaryEncoder> hueRotary;
static std::shared_ptr<RotaryEncoder> satRotary;
static std::shared_ptr<RotaryEncoder> ledSelected;
static std::shared_ptr<RotaryEncoder> reachRotary;
static std::shared_ptr<RotaryEncoder> posRotary;
static std::shared_ptr<RotaryEncoder> leftButton;
static std::shared_ptr<RotaryEncoder> rightButton;
static std::vector<std::shared_ptr<RotaryEncoder>> rotaries;

// The Potentiometer Knobs.
static std::shared_ptr<SeeedAdc8Ch12Bit> knobsAdcs;
static std::shared_ptr<PotentiometerKnob> brightKnob;
static std::shared_ptr<PotentiometerKnob> gainKnob;

// The Main Thread (i.e. This App's Thread)
static std::atomic<bool> exitThisApp;
static std::shared_ptr<std::thread> thisAppThread;
static void thisAppForeverFunction();

static std::shared_ptr<SaveRestoreJson> saveRestore;

void cleanUpBeforeExit()
{
   // Let this app's thread know that it needs to exit.
   exitThisApp = true;
  
   // Kill the Rotary Polling Thread.
   rotaryEncPollThreadActive = false;
   if(checkRotaryThread.get() != nullptr)
   {
      checkRotaryThread->join();
      checkRotaryThread.reset();
   }

   // The Gradient Change Thread might be active. If so get it to end.
   if(gradChangeThread.get() != nullptr)
   {
      gradChangeThread->endThread();
   }

   // Cleanup AudioLeds Object
   if(audioLed.get() != nullptr)
   {
      audioLed->endThread();
   }

   // Join this app's thread.
   thisAppThread->join();
   thisAppThread.reset();

   // Turn off all the LEDs in the LED strip.
   ledStrip.reset();
}

void signalHandler(int signum)
{
   cleanUpBeforeExit();
   exit(signum); 
}

void RotaryUpdateFunction()
{
   ThreadPriorities::setThisThreadPriorityPolicy(ThreadPriorities::ROTORY_ENCODER_POLL_THREAD_PRIORITY, SCHED_FIFO);
   ThreadPriorities::setThisThreadName("RotEncPoll");
   while(rotaryEncPollThreadActive)
   {
      for(auto& rotary : rotaries)
      {
         rotary->updateRotation();
      }
      usleep(1*1000);
   }
}

int main (int argc, char *argv[])
{
   wiringPiSetup();

   // This is used to save / restore Color Gradients.
   saveRestore.reset(new SaveRestoreJson());

   // Setup LED strip.
   ledStrip.reset(new LedStrip(NUM_LEDS, LedStrip::GRB));
   ledStrip->clear();

   // Setup Signal Handler for ctrl+c
   signal(SIGINT, signalHandler);

   hueRotary.reset(new RotaryEncoder(RotaryEncoder::E_HIGH, 13, 12, 14));
   satRotary.reset(new RotaryEncoder(RotaryEncoder::E_HIGH,  0,  2,  3));
   ledSelected.reset(new RotaryEncoder(RotaryEncoder::E_HIGH, 21, 22, 23));
   reachRotary.reset(new RotaryEncoder(RotaryEncoder::E_HIGH, 28, 27, 29));
   posRotary.reset(new RotaryEncoder(RotaryEncoder::E_HIGH, 11, 10, 31));
   leftButton.reset(new RotaryEncoder(RotaryEncoder::E_HIGH, 25));
   rightButton.reset(new RotaryEncoder(RotaryEncoder::E_HIGH, 24));

   knobsAdcs.reset(new SeeedAdc8Ch12Bit());
   brightKnob.reset(new PotentiometerKnob(knobsAdcs, 7, 100));
   gainKnob.reset(new PotentiometerKnob(knobsAdcs, 6, 100));

   thisAppThread.reset(new std::thread(thisAppForeverFunction));

   sleep(0x7FFFFFFF);

   return 0;
}

static void thisAppForeverFunction()
{
   bool skipGradFirst = true;
   exitThisApp = false;
   while(!exitThisApp)
   {
      // Set initial gradient. Try to restore, set to default if restore fails.
      auto gradColors = saveRestore->restore_gradient();
      bool failedRestore = (gradColors.size() <= 0);
      if(failedRestore)
      {
         // Restore didn't have anything. Set to Red, White and Blue
         constexpr int numColorPoints = 3;
         gradColors.resize(numColorPoints);
         gradColors[0].hue = 0;
         gradColors[0].saturation = 1.0;
         gradColors[1].hue = 0.5;
         gradColors[1].saturation = 0.0;
         gradColors[2].hue = 0.65;
         gradColors[2].saturation = 1.0;
      }
      std::shared_ptr<ColorGradient> grad(new ColorGradient(gradColors, failedRestore));

      if(!skipGradFirst)
      {
         // Gradient Edit Mode
         if(!exitThisApp)
         {
            // Start up the thread that will check the periodically query the state fo the rotary encoders.
            rotaries.clear();
            rotaries.push_back(hueRotary);
            rotaries.push_back(satRotary);
            rotaries.push_back(ledSelected);
            rotaries.push_back(reachRotary);
            rotaries.push_back(posRotary);
            rotaryEncPollThreadActive = true;
            checkRotaryThread.reset(new std::thread(RotaryUpdateFunction));

            gradChangeThread.reset(new GradChangeThread(
               grad, 
               ledStrip, 
               hueRotary,
               satRotary,
               ledSelected,
               reachRotary,
               posRotary,
               leftButton,
               rightButton,
               brightKnob));

            // Wait for User to Exit Gradient Edit Mode.
            gradChangeThread->waitForThreadDone();
            gradChangeThread.reset();

            // Kill the Rotary Polling Thread.
            rotaryEncPollThreadActive = false;
            if(checkRotaryThread.get() != nullptr)
            {
               checkRotaryThread->join();
               checkRotaryThread.reset();
            }
         }

         // Set the LEDs to Black.
         ledStrip->clear();

         // Wait for both to be unpressed.
         while(leftButton->checkButton(false) && rightButton->checkButton(false) && !exitThisApp){std::this_thread::sleep_for(std::chrono::milliseconds(1));}
      }
      skipGradFirst = false;

      std::vector<ColorGradient::tGradientPoint> gradVect = grad->getGradient();
      saveRestore->save_gradient(gradVect);

      // Configure for FFT Audio Mode.
      if(!exitThisApp)
      {
         // Start up the thread that will check the periodically query the state fo the rotary encoders.
         rotaries.clear();
         rotaries.push_back(hueRotary);
         rotaries.push_back(ledSelected);
         rotaries.push_back(posRotary);
         rotaryEncPollThreadActive = true;
         checkRotaryThread.reset(new std::thread(RotaryUpdateFunction));

         audioLed.reset(new AudioLeds(
            grad,
            saveRestore,
            ledStrip,
            hueRotary,
            ledSelected,
            posRotary,
            rightButton,
            leftButton,
            rightButton,
            brightKnob,
            gainKnob ));
         
         // Wait for User to Exit Gradient Edit Mode.
         audioLed->waitForThreadDone();
         audioLed.reset();
         
         // Kill the Rotary Polling Thread.
         rotaryEncPollThreadActive = false;
         if(checkRotaryThread.get() != nullptr)
         {
            checkRotaryThread->join();
            checkRotaryThread.reset();
         }
      }
      
      // Set the LEDs to Black.
      ledStrip->clear();

      // Wait for both to be unpressed.
      while(leftButton->checkButton(false) && rightButton->checkButton(false) && !exitThisApp){std::this_thread::sleep_for(std::chrono::milliseconds(1));}
   }
   
}
