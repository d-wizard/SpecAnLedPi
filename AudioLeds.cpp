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
#include "AudioLeds.h"
#include "colorGradient.h"
#include "gradientToScale.h"

// Audio Stuff
#define SAMPLE_RATE (44100)

// FFT Stuff
#define FFT_SIZE (256) // Base 2 number


AudioLeds::AudioLeds( std::shared_ptr<ColorGradient> colorGrad, 
                      std::shared_ptr<LedStrip> ledStrip, 
                      std::shared_ptr<RotaryEncoder> cycleGrads,
                      std::shared_ptr<RotaryEncoder> deleteButton,
                      std::shared_ptr<RotaryEncoder> leftButton,
                      std::shared_ptr<RotaryEncoder> rightButton,
                      std::shared_ptr<PotentiometerKnob> brightKnob,
                      std::shared_ptr<PotentiometerKnob> gainKnob ) :
   m_ledStrip(ledStrip),
   m_cycleGrads(cycleGrads),
   m_deleteButton(deleteButton),
   m_leftButton(leftButton),
   m_rightButton(rightButton),
   m_brightKnob(brightKnob),
   m_gainKnob(gainKnob)
{
   auto NUM_LEDS = m_ledStrip->getNumLeds();

   // FFT Stuff
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
   mod.fadeAwayAmount = 15;
   fftModifier.reset(new FftModifier(SAMPLE_RATE, FFT_SIZE, NUM_LEDS, mod));
   
   // Set Color Scale that will be used to turn Audio into Colors.
   auto gradVect = colorGrad->getGradient();
   std::vector<ColorScale::tColorPoint> colors;
   Convert::convertGradientToScale(gradVect, colors);
   std::vector<ColorScale::tBrightnessPoint> brightPoints{{0,0},{1,1}}; // Scale brightness.
   colorScale.reset(new ColorScale(colors, brightPoints));

   ledColors.resize(NUM_LEDS);

   // Create the Button / Rotary Enocoder monitoring thread.
   buttonMonitorThreadLives = true;
   buttonMonitorThread = std::thread(&AudioLeds::buttonMonitorThreadFunc, this);

   // Create the processing thread.
   pcmSampBuff.reserve(5000);
   procThreadLives = true;
   processingThread = std::thread(&AudioLeds::processPcmSamples, this);

   // Start capturing from the microphone.
   mic.reset(new AlsaMic("hw:1", SAMPLE_RATE, FFT_SIZE, 1, alsaMicSamples, this));
}

AudioLeds::~AudioLeds()
{
   // Stop getting samples from the microphone.
   mic.reset();

   // Kill the proc thread and join.
   procThreadLives = false;
   bufferMutex.lock();
   bufferReadyCondVar.notify_all();
   bufferMutex.unlock();
   processingThread.join();
}

void AudioLeds::waitForThreadDone()
{
   buttonMonitorThread.join();
}

void AudioLeds::endThread()
{
   buttonMonitorThreadLives = false;
}


void AudioLeds::buttonMonitorThreadFunc()
{
   while(buttonMonitorThreadLives)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if(m_leftButton->checkButton(false) && m_rightButton->checkButton(false))
      {
         buttonMonitorThreadLives = false;
      }
   }
}

// Processes PCM samples from the Microphone Capture object.
void AudioLeds::processPcmSamples()
{
   auto NUM_LEDS = m_ledStrip->getNumLeds();
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
            smartPlot_1D(samples, E_INT_16, numSamp, SAMPLE_RATE, SAMPLE_RATE/49, "Mic", "Samp");
         #elif 0
            SpecAnLedTypes::tFftVector* fftResult = fftRun->run(samples, numSamp);
            if(fftResult != nullptr)
            {
               fftModifier->modify(fftResult->data());
               smartPlot_1D(fftResult->data(), E_UINT_16, NUM_LEDS, NUM_LEDS, 0, "FFT", "re");
            }
         #else
            SpecAnLedTypes::tFftVector* fftResult = fftRun->run(samples, numSamp);
            if(fftResult != nullptr)
            {
               fftModifier->modify(fftResult->data());

               float brightness = m_brightKnob->getFlt();
               auto gain = m_gainKnob->getInt()*10;
               for(size_t i = 0 ; i < NUM_LEDS; ++i)
               {
                  int32_t ledVal = (int32_t)fftResult->data()[i]*gain;
                  if(ledVal > 65535)
                  {
                     ledVal = 65535;
                  }
                  ledColors[i] = colorScale->getColor(ledVal, brightness);
               }
               m_ledStrip->set(ledColors);
            }
         #endif
      }
   }
}


void AudioLeds::alsaMicSamples(void* usrPtr, int16_t* samples, size_t numSamp)
{
   // Move to buffer and return ASAP.
   auto _this = (AudioLeds*)usrPtr;
   std::unique_lock<std::mutex> lock(_this->bufferMutex);
   auto origSize = _this->pcmSampBuff.size();
   _this->pcmSampBuff.resize(origSize+numSamp);
   memcpy(&_this->pcmSampBuff[origSize], samples, numSamp*sizeof(samples[0]));
   _this->bufferReadyCondVar.notify_all();
}
