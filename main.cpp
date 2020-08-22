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

#ifdef SEEED_ADC_DEV_ADDR
#include "seeed_adc_8chan_12bit.h"
#endif

#include <thread>
#include <mutex>
#include <condition_variable>


#include "smartPlotMessage.h"

#define SAMPLE_RATE (44100)
#define NUM_LEDS (20)

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
static std::unique_ptr<LedStrip> ledStrip;
static SpecAnLedTypes::tRgbVector ledColors;
static std::unique_ptr<ColorScale> colorScale;

#ifdef SEEED_ADC_DEV_ADDR
// Gain Knob
static std::unique_ptr<SeeedAdc8Ch12Bit> adc8Ch;
#endif

void processPcmSamples()
{
   int16_t samples[FFT_SIZE];
   size_t numSamp = FFT_SIZE;
   int gain = 16;
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
            smartPlot_1D(samples, E_INT_16, numSamp, SAMPLE_RATE, SAMPLE_RATE/4, "Mic", "Samp");
         #elif 0
            fft->runFft(samples, fftSamp.data());
            int numBins = fftModifier->modify(fftSamp.data());//numSamp/2;//
            smartPlot_1D(fftSamp.data(), E_UINT_16, numBins, numBins, 0, "FFT", "re");
         #else
            SpecAnLedTypes::tFftVector* fftResult = fftRun->run(samples, numSamp);
            if(fftResult != nullptr)
            {
               int numBins = fftModifier->modify(fftResult->data());
               for(int i = 0 ; i < NUM_LEDS; ++i)
               {
                  ledColors[i] = colorScale->getColor(fftResult->data()[i]*gain);
               }
               ledStrip->set(ledColors);
               //smartPlot_1D(fftResult->data(), E_UINT_16, numBins, numBins, 0, "FFT", "re");
            }
            
#ifdef SEEED_ADC_DEV_ADDR
            gain = adc8Ch->isActive() ? adc8Ch->getAdcValue(SEEED_ADC_GAIN_NUM) >> 5 : 16;
#endif
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

void defineColorScale()
{
   std::vector<ColorScale::tColorPoint> colors;
   std::vector<ColorScale::tBrightnessPoint> bright;

   int idx = 0;
   colors.resize(1);
   bright.resize(1);

   // Patriotic colors: Red, White, Blue.
   colors[idx].color.u32 = SpecAnLedTypes::COLOR_RED;
   colors[idx].startPoint  = 0.00;
   idx++; colors.resize(idx+1);

   colors[idx].color.u32 = colors[idx-1].color.u32; // Repeat previous color.
   colors[idx].startPoint  = 0.08;
   idx++; colors.resize(idx+1);

   colors[idx].color.u32 = SpecAnLedTypes::COLOR_WHITE;
   colors[idx].startPoint  = 0.09;
   idx++; colors.resize(idx+1);

   colors[idx].color.u32 = colors[idx-1].color.u32; // Repeat previous color.
   colors[idx].startPoint  = 0.20;
   idx++; colors.resize(idx+1);

   colors[idx].color.u32 = SpecAnLedTypes::COLOR_BLUE;
   colors[idx].startPoint  = 0.25;
   idx++; colors.resize(idx+1);

   colors[idx].color.u32 = colors[idx-1].color.u32; // Repeat previous color.

   idx = 0;
   bright[idx].brightness = 0.0;
   bright[idx].startPoint = 0;
   idx++; bright.resize(idx+1);

   bright[idx].brightness = 0.25;

   colorScale.reset(new ColorScale(colors, bright));
}

void cleanUpBeforeExit()
{
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
}

void signalHandler(int signum)
{
   cleanUpBeforeExit();
}

int main (int argc, char *argv[])
{
   fftRun.reset(new FftRunRate(SAMPLE_RATE, FFT_SIZE, 150.0));

   tFftModifiers mod;
   mod.startFreq = 300;
   mod.stopFreq = 12000;
   mod.clipMin = 0;
   mod.clipMax = 5000;
   mod.logScale = false;
   fftModifier.reset(new FftModifier(SAMPLE_RATE, FFT_SIZE, NUM_LEDS, mod));

   pcmSampBuff.reserve(5000);

   // Create the processing thread.
   processingThread.reset(new std::thread(processPcmSamples));

   // Setup LED strip.
   ledColors.resize(NUM_LEDS);
   ledStrip.reset(new LedStrip(NUM_LEDS, LedStrip::GRB));

   // Define the Color Scale Gradient.
   defineColorScale();

   // Setup Signal Handler for ctrl+c
   signal(SIGINT, signalHandler);

#ifdef SEEED_ADC_DEV_ADDR
   adc8Ch.reset(new SeeedAdc8Ch12Bit(SEEED_ADC_DEV_ADDR));
#endif
   
   // Start capturing from the microphone.
   mic.reset(new AlsaMic("hw:1", SAMPLE_RATE, FFT_SIZE, 1, alsaMicSamples));

   sleep(0x7FFFFFFF);

   return 0;
}