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
#include "alsaMic.h"
#include "specAnFft.h"

#include <thread>
#include <mutex>
#include <condition_variable>


#include "smartPlotMessage.h"


// FFT Stuff
#define FFT_SIZE (64) // Base 2 number
static std::unique_ptr<SpecAnFft> fft;
static std::vector<uint16_t> fftSamp;

// Processing Thread Stuff.
static std::mutex bufferMutex;
static std::condition_variable bufferReadyCondVar;
static std::vector<int16_t> pcmSampBuff;
static bool procThreadLives = true;

void processPcmSamples()
{
   int16_t samples[FFT_SIZE];
   while(procThreadLives)
   {
      bool copySamples = false;
      {
         std::unique_lock<std::mutex> lock(bufferMutex);

         // Check if we have samples right now or if we need to wait.
         copySamples = (pcmSampBuff.size() >= FFT_SIZE);
         if(!copySamples)
         {
            bufferReadyCondVar.wait(lock);
            copySamples = (pcmSampBuff.size() >= FFT_SIZE);
         }

         copySamples = copySamples && procThreadLives;
         if(copySamples)
         {
            memcpy(samples, pcmSampBuff.data(), sizeof(samples));
            pcmSampBuff.erase(pcmSampBuff.begin(),pcmSampBuff.begin()+FFT_SIZE);
         }
      }

      if(copySamples)
      {
         // Now we can process the samples outside of the mutex lock.
         //smartPlot_1D(samples, E_INT_16, FFT_SIZE, 44100, -1, "Mic", "Samp");
         fft->runFft(samples, fftSamp.data());
         smartPlot_1D(fftSamp.data(), E_UINT_16, FFT_SIZE/2, FFT_SIZE/2, 0, "FFT", "re");
      }
   }
}


void alsaMicSamples(int16_t* samples, size_t numSamp)
{
   //fft->runFft(samples, fftSamp.data());
   //smartPlot_1D(fftSamp.data(), E_UINT_16, numSamp/2, 2*numSamp, -1, "FFT", "re");

   //smartPlot_1D(samples, E_INT_16, numSamp, 44100, -1, "Mic", "Samp");

   bufferMutex.lock();
   auto origSize = pcmSampBuff.size();
   pcmSampBuff.resize(origSize+numSamp);
   memcpy(&pcmSampBuff[origSize], samples, numSamp*sizeof(samples[0]));
   bufferReadyCondVar.notify_all();
   bufferMutex.unlock();
}


int main (int argc, char *argv[])
{
   //smartPlot_createFlushThread(10);

   fft.reset(new SpecAnFft(FFT_SIZE));
   fftSamp.resize(2*FFT_SIZE);

   pcmSampBuff.reserve(5000);

   sleep(1);

   // Create the processing thread.
   std::thread processingThread(processPcmSamples);

   std::unique_ptr<AlsaMic> mic;

   mic.reset(new AlsaMic("hw:1", 16000, FFT_SIZE, 1, alsaMicSamples));

   sleep(50);

   mic.reset();

   procThreadLives = false;
   bufferMutex.lock();
   bufferReadyCondVar.notify_all();
   bufferMutex.unlock();
   processingThread.join();

   sleep(2);


  return 0;
}