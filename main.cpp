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
#include <math.h>
#include <memory> // unique_ptr
#include "specAnLedPiTypes.h"
#include "alsaMic.h"
#include "specAnFft.h"
#include "fftRunRate.h"
#include "fftModifier.h"
#include "ledStrip.h"

#include <thread>
#include <mutex>
#include <condition_variable>


#include "smartPlotMessage.h"

#define SAMPLE_RATE (44100)
#define NUM_LEDS (5)

// FFT Stuff
#define FFT_SIZE (256) // Base 2 number
//static std::unique_ptr<SpecAnFft> fft;
//static std::vector<uint16_t> fftSamp;

static std::unique_ptr<FftRunRate> fftRun;
static std::unique_ptr<FftModifier> fftModifier;


// Processing Thread Stuff.
static std::mutex bufferMutex;
static std::condition_variable bufferReadyCondVar;
static tPcmBuffer pcmSampBuff;
static bool procThreadLives = true;

// LED Stuff
static std::unique_ptr<LedStrip> ledStrip;


void processPcmSamples()
{
   int16_t samples[FFT_SIZE];
   size_t numSamp = FFT_SIZE;
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
            //smartPlot_1D(samples, E_INT_16, numSamp, SAMPLE_RATE, SAMPLE_RATE/4, "Mic", "Samp");
            fft->runFft(samples, fftSamp.data());
            int numBins = fftModifier->modify(fftSamp.data());//numSamp/2;//
            smartPlot_1D(fftSamp.data(), E_UINT_16, numBins, numBins, 0, "FFT", "re");
         #else
            tFftVector* fftResult = fftRun->run(samples, numSamp);
            if(fftResult != nullptr)
            {
               int numBins = fftModifier->modify(fftResult->data());
               smartPlot_1D(fftResult->data(), E_UINT_16, numBins, numBins, 0, "FFT", "re");
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


int main (int argc, char *argv[])
{
   //smartPlot_createFlushThread(10);

   //fft.reset(new SpecAnFft(FFT_SIZE));
   //fftSamp.resize(2*FFT_SIZE);
   fftRun.reset(new FftRunRate(SAMPLE_RATE, FFT_SIZE, 35.0));


   tFftModifiers mod;
   mod.startFreq = 300;
   mod.stopFreq = 12000;
   mod.clipMin = 0;
   mod.clipMax = 65535;
   mod.logScale = false;
   fftModifier.reset(new FftModifier(SAMPLE_RATE, FFT_SIZE, 40, mod));

   pcmSampBuff.reserve(5000);

   sleep(1);

   // Create the processing thread.
   std::thread processingThread(processPcmSamples);

   std::unique_ptr<AlsaMic> mic;

   mic.reset(new AlsaMic("hw:1", SAMPLE_RATE, FFT_SIZE, 1, alsaMicSamples));

   ledStrip.reset(new LedStrip(NUM_LEDS, LedStrip::GRB));
   tRgbVector leds(NUM_LEDS);
   leds[0].rgb.r = 50;
   leds[2].rgb.g = 50;
   leds[4].rgb.b = 50;
   ledStrip->set(leds);

   sleep(5);

   mic.reset();

   procThreadLives = false;
   bufferMutex.lock();
   bufferReadyCondVar.notify_all();
   bufferMutex.unlock();
   processingThread.join();

   ledStrip.reset();
   sleep(2);


  return 0;
}