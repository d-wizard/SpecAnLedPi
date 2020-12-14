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
                      std::shared_ptr<SaveRestoreGrad> saveRestorGrad,
                      std::shared_ptr<LedStrip> ledStrip, 
                      std::shared_ptr<RotaryEncoder> cycleGrads,
                      std::shared_ptr<RotaryEncoder> deleteButton,
                      std::shared_ptr<RotaryEncoder> leftButton,
                      std::shared_ptr<RotaryEncoder> rightButton,
                      std::shared_ptr<PotentiometerKnob> brightKnob,
                      std::shared_ptr<PotentiometerKnob> gainKnob ) :
   m_saveRestorGrad(saveRestorGrad),
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
   m_fftRun.reset(new FftRunRate(SAMPLE_RATE, FFT_SIZE, 150.0));

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
   m_fftModifier.reset(new FftModifier(SAMPLE_RATE, FFT_SIZE, NUM_LEDS, mod));
   
   // Set Color Scale that will be used to turn Audio into Colors.
   auto gradVect = colorGrad->getGradient();
   std::vector<ColorScale::tColorPoint> colors;
   Convert::convertGradientToScale(gradVect, colors);
   std::vector<ColorScale::tBrightnessPoint> brightPoints{{0,0},{1,1}}; // Scale brightness.
   m_colorScale.reset(new ColorScale(colors, brightPoints));

   m_ledColors.resize(NUM_LEDS);

   // Create the Button / Rotary Enocoder monitoring thread.
   m_buttonMonitorThread_active = true;
   m_buttonMonitor_thread = std::thread(&AudioLeds::buttonMonitorFunc, this);

   // Create the processing thread.
   m_pcmProc_buff.reserve(5000);
   m_pcmProc_active = true;
   m_pcmProc_thread = std::thread(&AudioLeds::pcmProcFunc, this);

   // Start capturing from the microphone.
   m_mic.reset(new AlsaMic("hw:1", SAMPLE_RATE, FFT_SIZE, 1, alsaMicSamples, this));
}

AudioLeds::~AudioLeds()
{
   // Stop getting samples from the microphone.
   m_mic.reset();

   // Kill the proc thread and join.
   m_pcmProc_active = false;
   m_pcmProc_mutex.lock();
   m_pcmProc_bufferReadyCondVar.notify_all();
   m_pcmProc_mutex.unlock();
   m_pcmProc_thread.join();
}

void AudioLeds::waitForThreadDone()
{
   m_buttonMonitor_thread.join();
}

void AudioLeds::endThread()
{
   m_buttonMonitorThread_active = false;
}


void AudioLeds::buttonMonitorFunc()
{
   int timerCount = 0;
   std::vector<ColorGradient::tGradientPoint> newGrad;
   bool loadNewGrad = false;

   while(m_buttonMonitorThread_active)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      // Check if the user wants to change the color gradient.
      auto changeGrad = m_cycleGrads->checkRotation();
      if(changeGrad != RotaryEncoder::E_NO_CHANGE)
      {
         newGrad = (changeGrad == RotaryEncoder::E_FORWARD ? m_saveRestorGrad->restoreNext() : m_saveRestorGrad->restorePrev());
         loadNewGrad = true;
      }

      // Check if the user wants to remove a gradient.
      if(m_deleteButton->checkButton() == RotaryEncoder::E_DOUBLE_CLICK)
      {
         newGrad = m_saveRestorGrad->deleteCurrent();
         loadNewGrad = true;
      }

      // Load the New Gradient.
      if(loadNewGrad)
      {
         loadNewGrad = false;

         // Convert
         std::vector<ColorScale::tColorPoint> colors;
         Convert::convertGradientToScale(newGrad, colors);
         std::vector<ColorScale::tBrightnessPoint> brightPoints{{0,0},{1,1}}; // Scale brightness.

         // Store
         std::unique_lock<std::mutex> lock(m_colorScaleMutex);
         m_colorScale.reset(new ColorScale(colors, brightPoints));
      }


      // Slower tasks.
      if(++timerCount == 100)
      {
         timerCount = 0;

         // Check if user want to toggle back to Gradient Edit Mode
         if(m_leftButton->checkButton(false) && m_rightButton->checkButton(false))
         {
            m_buttonMonitorThread_active = false;
         }
      }
   }
}

// Processes PCM samples from the Microphone Capture object.
void AudioLeds::pcmProcFunc()
{
   auto NUM_LEDS = m_ledStrip->getNumLeds();
   int16_t samples[FFT_SIZE];
   size_t numSamp = FFT_SIZE;
   while(m_pcmProc_active)
   {
      bool copySamples = false;
      {
         std::unique_lock<std::mutex> lock(m_pcmProc_mutex);

         // Check if we have samples right now or if we need to wait.
         copySamples = (m_pcmProc_buff.size() >= numSamp);
         if(!copySamples)
         {
            m_pcmProc_bufferReadyCondVar.wait(lock);
            copySamples = (m_pcmProc_buff.size() >= numSamp);
         }

         copySamples = copySamples && m_pcmProc_active;
         if(copySamples)
         {
            memcpy(samples, m_pcmProc_buff.data(), sizeof(samples));
            m_pcmProc_buff.erase(m_pcmProc_buff.begin(),m_pcmProc_buff.begin()+numSamp);
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
            SpecAnLedTypes::tFftVector* fftResult = m_fftRun->run(samples, numSamp);
            if(fftResult != nullptr)
            {
               m_fftModifier->modify(fftResult->data());

               float brightness = m_brightKnob->getFlt();
               auto gain = m_gainKnob->getInt()*10;

               // Generate the actual LED colors
               {
                  std::unique_lock<std::mutex> lock(m_colorScaleMutex);
                  for(size_t i = 0 ; i < NUM_LEDS; ++i)
                  {
                     int32_t ledVal = (int32_t)fftResult->data()[i]*gain;
                     if(ledVal > 65535)
                     {
                        ledVal = 65535;
                     }
                     m_ledColors[i] = m_colorScale->getColor(ledVal, brightness);
                  }
               }

               m_ledStrip->set(m_ledColors);
            }
         #endif
      }
   }
}


void AudioLeds::alsaMicSamples(void* usrPtr, int16_t* samples, size_t numSamp)
{
   // Move to buffer and return ASAP.
   auto _this = (AudioLeds*)usrPtr;
   std::unique_lock<std::mutex> lock(_this->m_pcmProc_mutex);
   auto origSize = _this->m_pcmProc_buff.size();
   _this->m_pcmProc_buff.resize(origSize+numSamp);
   memcpy(&_this->m_pcmProc_buff[origSize], samples, numSamp*sizeof(samples[0]));
   _this->m_pcmProc_bufferReadyCondVar.notify_all();
}
