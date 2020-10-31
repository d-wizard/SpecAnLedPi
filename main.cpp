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
#include "specAnLedPiTypes.h"
#include "alsaMic.h"
#include "specAnFft.h"
#include "fftRunRate.h"
#include "fftModifier.h"
#include "ledStrip.h"
#include "colorScale.h"
#include "colorGradient.h"
#include "gradientToScale.h"
#include "gradientChangeThread.h"
#include "rotaryEncoder.h"
#include "potentiometerAdc.h"
#include "potentiometerKnob.h"
#include "ThreadPriorities.h"

#include <thread>
#include <mutex>
#include <condition_variable>


#include "smartPlotMessage.h"
#include "wiringPi.h"

#define SAMPLE_RATE (44100)
#define NUM_LEDS (40)

// Microphone Capture
static std::unique_ptr<AlsaMic> mic;

// FFT Stuff
#define FFT_SIZE (256) // Base 2 number

static std::unique_ptr<FftRunRate> fftRun;
static std::unique_ptr<FftModifier> fftModifier;

// Processing Thread Stuff.
static std::unique_ptr<std::thread> processingThread;
static std::mutex bufferMutex;
static std::condition_variable bufferReadyCondVar;
static SpecAnLedTypes::tPcmBuffer pcmSampBuff;
static bool procThreadLives = true;

// LED Stuff
static std::shared_ptr<LedStrip> ledStrip;
static SpecAnLedTypes::tRgbVector ledColors;
static std::unique_ptr<ColorScale> colorScale;

// Thread for Updating the Color Gradient
static std::unique_ptr<GradChangeThread> gradChangeThread;

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



// Processes PCM samples whenever the Microphone Capture object (mic) is instantiated.
void processPcmSamples()
{
   int16_t samples[FFT_SIZE];
   size_t numSamp = FFT_SIZE;
   int32_t gain = 64;
   while(procThreadLives)
   {
      bool copySamples = false;
      {
         std::unique_lock<std::mutex> lock(bufferMutex);

         // Check if we have samples right now or if we need to wait.
         copySamples = (pcmSampBuff.size() >= numSamp);
         if(!copySamples)
         {
            bufferReadyCondVar.wait(lock);
            copySamples = (pcmSampBuff.size() >= numSamp);
         }

         copySamples = copySamples && procThreadLives;
         if(copySamples)
         {
            memcpy(samples, pcmSampBuff.data(), sizeof(samples));
            pcmSampBuff.erase(pcmSampBuff.begin(),pcmSampBuff.begin()+numSamp);
         }
      }

      if(copySamples)
      {
         #if 0
            // Now we can process the samples outside of the mutex lock.
            smartPlot_1D(samples, E_INT_16, numSamp, SAMPLE_RATE, SAMPLE_RATE/49, "Mic", "Samp");
         #elif 0
            fft->runFft(samples, fftSamp.data());
            int numBins = fftModifier->modify(fftSamp.data());//numSamp/2;//
            smartPlot_1D(fftSamp.data(), E_UINT_16, numBins, numBins, 0, "FFT", "re");
         #else
            SpecAnLedTypes::tFftVector* fftResult = fftRun->run(samples, numSamp);
            if(fftResult != nullptr)
            {
               fftModifier->modify(fftResult->data());

               float brightness = brightKnob->getFlt();
               gain = gainKnob->getInt()*10;
               for(int i = 0 ; i < NUM_LEDS; ++i)
               {
                  int32_t ledVal = (int32_t)fftResult->data()[i]*gain;
                  if(ledVal > 65535)
                  {
                     ledVal = 65535;
                  }
                  ledColors[i] = colorScale->getColor(ledVal, brightness);
               }
               ledStrip->set(ledColors);
               smartPlot_1D(&gain, E_UINT_16, 1, 1000, 100, "gain", "adc");
            }
         #endif
      }
   }
}


void alsaMicSamples(int16_t* samples, size_t numSamp)
{
   // Move to buffer and return ASAP.
   std::unique_lock<std::mutex> lock(bufferMutex);
   auto origSize = pcmSampBuff.size();
   pcmSampBuff.resize(origSize+numSamp);
   memcpy(&pcmSampBuff[origSize], samples, numSamp*sizeof(samples[0]));
   bufferReadyCondVar.notify_all();
}

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

   // Stop getting samples from the microphone.
   mic.reset();

   // Kill the proc thread and join.
   procThreadLives = false;
   bufferMutex.lock();
   bufferReadyCondVar.notify_all();
   bufferMutex.unlock();
   processingThread->join();

   // Turn off all the LEDs in the LED strip.
   ledStrip.reset();

   // Join this app's thread.
   thisAppThread->join();
   thisAppThread.reset();
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
   fftRun.reset(new FftRunRate(SAMPLE_RATE, FFT_SIZE, 150.0));

   tFftModifiers mod;
   mod.startFreq = 300;
   mod.stopFreq = 12000;
   mod.clipMin = 0;
   mod.clipMax = 5000;
   mod.logScale = false;
   mod.attenLowFreqs = true;
   mod.attenLowStartLevel = 0.2;
   mod.attenLowStopFreq = 6000;
   fftModifier.reset(new FftModifier(SAMPLE_RATE, FFT_SIZE, NUM_LEDS, mod));

   pcmSampBuff.reserve(5000);

   // Create the processing thread.
   processingThread.reset(new std::thread(processPcmSamples));

   // Setup LED strip.
   ledColors.resize(NUM_LEDS);
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

   rotaries.push_back(hueRotary);
   rotaries.push_back(satRotary);
   rotaries.push_back(ledSelected);
   rotaries.push_back(reachRotary);
   rotaries.push_back(posRotary);

   thisAppThread.reset(new std::thread(thisAppForeverFunction));

   sleep(0x7FFFFFFF);

   return 0;
}


void thisAppForeverFunction()
{
   constexpr int numColorPoints = 3;
   std::vector<ColorGradient::tGradientPoint> gradColors(numColorPoints);
   gradColors[0].hue = 0;
   gradColors[0].saturation = 1.0;
   gradColors[1].hue = 0.5;
   gradColors[1].saturation = 0.0;
   gradColors[2].hue = 0.65;
   gradColors[2].saturation = 1.0;

   std::shared_ptr<ColorGradient> grad(new ColorGradient(gradColors));

   exitThisApp = false;
   while(!exitThisApp)
   {
      // Gradient Edit Mode
      if(!exitThisApp)
      {
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

      // Configure for FFT Audio Mode.
      if(!exitThisApp)
      {
         std::vector<ColorScale::tColorPoint> colors;
         std::vector<ColorGradient::tGradientPoint> gradVect = grad->getGradient();
         Convert::convertGradientToScale(gradVect, colors);

         std::vector<ColorScale::tBrightnessPoint> brightPoints{{0,0},{1,1}}; // Scale brightness.
         colorScale.reset(new ColorScale(colors, brightPoints));

         // Start capturing from the microphone.
         mic.reset(new AlsaMic("hw:1", SAMPLE_RATE, FFT_SIZE, 1, alsaMicSamples));

         // Perioidically check if the user wants to enter Gradient Edit Mode.
         bool toggleBackToGradientDefine = false;
         while(toggleBackToGradientDefine == false && !exitThisApp)
         {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if(leftButton->checkButton(false) && rightButton->checkButton(false))
            {
               while(leftButton->checkButton(false) && rightButton->checkButton(false)){std::this_thread::sleep_for(std::chrono::milliseconds(1));} // Wait for both to be released.
               toggleBackToGradientDefine = true;
            }
         }
         mic.reset();
      }

   }
}