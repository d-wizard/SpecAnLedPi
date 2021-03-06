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
#include <signal.h>
#include "AudioLeds.h"
#include "colorGradient.h"
#include "ThreadPriorities.h"
#include "Transform1D.h"

// Audio Stuff
#define SAMPLE_RATE (44100)

// FFT Stuff
#define FFT_SIZE (256) // Base 2 number


AudioLeds::AudioLeds( std::shared_ptr<ColorGradient> colorGrad, 
                      std::shared_ptr<SaveRestoreJson> saveRestore,
                      std::shared_ptr<LedStrip> ledStrip, 
                      std::shared_ptr<RotaryEncoder> cycleGrads,
                      std::shared_ptr<RotaryEncoder> cycleDisplays,
                      std::shared_ptr<RotaryEncoder> reverseGrad,
                      std::shared_ptr<RotaryEncoder> deleteButton,
                      std::shared_ptr<RotaryEncoder> leftButton,
                      std::shared_ptr<RotaryEncoder> rightButton,
                      std::shared_ptr<PotentiometerKnob> brightKnob,
                      std::shared_ptr<PotentiometerKnob> gainKnob ) :
   m_activeAudioDisplayIndex(0),
   m_saveRestore(saveRestore),
   m_ledStrip(ledStrip),
   m_ledColors(ledStrip->getNumLeds()),
   m_currentGradient(colorGrad->getGradient()),
   m_cycleGrads(cycleGrads),
   m_cycleDisplays(cycleDisplays),
   m_reverseGradToggle(reverseGrad),
   m_deleteButton(deleteButton),
   m_leftButton(leftButton),
   m_rightButton(rightButton),
   m_brightKnob(brightKnob),
   m_gainKnob(gainKnob)
{
   // Create the Button / Rotary Enocoder monitoring thread.
   m_buttonMonitorThread_active = true;
   m_buttonMonitor_thread = std::thread(&AudioLeds::buttonMonitorFunc, this);

   // Set the Audio Displays (do this before creating the thread)
   auto numLeds = ledStrip->getNumLeds();
   m_audioDisplayAmp.emplace_back(new AudioDisplayAmp(FFT_SIZE>>1, numLeds, AudioDisplayAmp::E_SCALE,    0.7, AudioDisplayAmp::E_PEAK_GRAD_MID_CHANGE));
   m_audioDisplayAmp.emplace_back(new AudioDisplayAmp(FFT_SIZE>>1, numLeds, AudioDisplayAmp::E_MIN_SAME, 0.7, AudioDisplayAmp::E_PEAK_GRAD_MID_CONST));
   m_audioDisplayAmp.emplace_back(new AudioDisplayAmp(FFT_SIZE>>1, numLeds, AudioDisplayAmp::E_MAX_SAME, 0.7, AudioDisplayAmp::E_PEAK_GRAD_MIN));
   m_audioDisplayFft.emplace_back(new AudioDisplayFft(SAMPLE_RATE, FFT_SIZE, numLeds, AudioDisplayFft::E_GRADIENT_MAG));
   m_audioDisplayFft.emplace_back(new AudioDisplayFft(SAMPLE_RATE, FFT_SIZE, numLeds, AudioDisplayFft::E_BRIGHTNESS_MAG));

   for(auto& disp : m_audioDisplayAmp)
      m_audioDisplays.push_back(disp.get());
   for(auto& disp : m_audioDisplayFft)
      m_audioDisplays.push_back(disp.get());

   // Attempt to Restore the Audio Display Index.
   int restoredDisplayIndex = m_saveRestore->restore_displayIndex();
   if(restoredDisplayIndex >= 0 && restoredDisplayIndex < int(m_audioDisplays.size()))
      m_activeAudioDisplayIndex = restoredDisplayIndex;

   // Make sure the first display gets set for the current gradient.
   m_audioDisplays[m_activeAudioDisplayIndex]->setGradient(m_currentGradient, m_reverseGrad);

   // Create the processing thread.
   m_pcmProc_buff.reserve(5000);
   m_pcmProc_active = true;
   m_pcmProc_thread = std::thread(&AudioLeds::pcmProcFunc, this);

   // Start capturing from the microphone.
   m_mic.reset(new AlsaMic("hw:1", SAMPLE_RATE, FFT_SIZE>>1, 1, alsaMicSamples, this));
}

AudioLeds::~AudioLeds()
{
   // Save off the current Audio Display.
   m_saveRestore->save_displayIndex(m_activeAudioDisplayIndex);

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
   ThreadPriorities::setThisThreadName("AudioButtonMon");
   int timerCount = 0;
   ColorGradient::tGradient newGrad;
   bool loadNewGrad = false;

   while(m_buttonMonitorThread_active)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      // Check if the user wants to change the color gradient.
      auto changeGrad = m_cycleGrads->checkRotation();
      if(changeGrad != RotaryEncoder::E_NO_CHANGE)
      {
         newGrad = (changeGrad == RotaryEncoder::E_FORWARD ? m_saveRestore->restore_gradientNext() : m_saveRestore->restore_gradientPrev());
         loadNewGrad = true;
      }

      // Check if the user wants to change the Audio Display.
      auto changeDisplay = m_cycleDisplays->checkRotation();
      if(changeDisplay != RotaryEncoder::E_NO_CHANGE)
      {
         int delta = (changeDisplay == RotaryEncoder::E_FORWARD ? 1 : -1);
         int max = m_audioDisplays.size();
         int newIndex = m_activeAudioDisplayIndex + delta;
         if(newIndex < 0) newIndex = max-1;
         else if(newIndex >= max) newIndex = 0;

         m_activeAudioDisplayIndex = newIndex;

         // Make sure the gradient gets updates in the new display (in case it changed since the last time the display was used).
         newGrad = m_currentGradient; 
         loadNewGrad = true;
      }

      // Check if the user want to reverse the gradient (use rotary and button).
      if(m_reverseGradToggle->checkRotation() != RotaryEncoder::E_NO_CHANGE || m_reverseGradToggle->checkButton(true))
      {
         newGrad = m_currentGradient;
         m_reverseGrad = !m_reverseGrad;
         loadNewGrad = true;
      }

      // Check if the user wants to remove a gradient.
      if(m_deleteButton->checkButton() == RotaryEncoder::E_DOUBLE_CLICK)
      {
         newGrad = m_saveRestore->delete_gradient();
         loadNewGrad = true;
      }

      // Load the New Gradient.
      if(loadNewGrad)
      {
         loadNewGrad = false;
         m_currentGradient = newGrad;
         m_audioDisplays[m_activeAudioDisplayIndex]->setGradient(m_currentGradient, m_reverseGrad);
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
   ThreadPriorities::setThisThreadName("PcmProcFunc");
   auto& audioDisplay = m_audioDisplays[m_activeAudioDisplayIndex];
   size_t numSamp = audioDisplay->getFrameSize();
   SpecAnLedTypes::tPcmBuffer samples(numSamp);

   while(m_pcmProc_active)
   {
      auto& audioDisplay = m_audioDisplays[m_activeAudioDisplayIndex];
      numSamp = audioDisplay->getFrameSize();
      samples.resize(numSamp);

      bool copySamples = false;
      {
         std::unique_lock<std::mutex> lock(m_pcmProc_mutex);

         // Check if we have samples right now or if we need to wait.
         copySamples = (m_pcmProc_buff.size() >= numSamp);
         if(!copySamples)
         {
            auto result = m_pcmProc_bufferReadyCondVar.wait_for(lock, std::chrono::milliseconds(100));
            if(result == std::cv_status::timeout)
            {
               // Sometimes the ALSA driver stuff just stops sending samples. Killing the application
               // is only way I have found to fix this issue. 
               std::thread([](){ raise(SIGINT); }).detach(); // Kill from a different thread.
               
               m_pcmProc_active = false; // Exit out of this thread.
            }
            copySamples = (m_pcmProc_buff.size() >= numSamp);
         }

         copySamples = copySamples && m_pcmProc_active;
         if(copySamples)
         {
            memcpy(samples.data(), m_pcmProc_buff.data(), sizeof(samples[0])*numSamp);
            m_pcmProc_buff.erase(m_pcmProc_buff.begin(),m_pcmProc_buff.begin()+numSamp);
         }
      }

      if(copySamples)
      {
         // Send the samples to the Audio Display to generate the LED Colors.
         if(audioDisplay->parsePcm(samples.data(), numSamp))
         {
            float brightness = Transform1D::Unit::quarterCircle_below(m_brightKnob->getFlt()); // Use the quarterCircle_below transform to provide more resolution at lower brightness levels.
            auto gain = m_gainKnob->getInt();

            audioDisplay->fillInLeds(m_ledColors, brightness, gain);
            m_ledStrip->set(m_ledColors);
         }
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
