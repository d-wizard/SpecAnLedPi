/* Copyright 2020, 2022 - 2023 Dan Williams. All Rights Reserved.
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

// Frame Sizes
#define MICROPHONE_FRAME_SIZE (SAMPLE_RATE / 60) // 60 Hz
#define AMP_DISP_FRAME_SIZE (MICROPHONE_FRAME_SIZE << 0) // Only run every 1 Microphone frames.


AudioLeds::AudioLeds( std::string microphoneName,
                      std::shared_ptr<ColorGradient> colorGrad, 
                      std::shared_ptr<SaveRestoreJson> saveRestore,
                      std::shared_ptr<LedStrip> ledStrip, 
                      std::shared_ptr<RotaryEncoder> cycleGrads,
                      std::shared_ptr<RotaryEncoder> cycleDisplays,
                      std::shared_ptr<RotaryEncoder> reverseGrad,
                      std::shared_ptr<RotaryEncoder> deleteButton,
                      std::shared_ptr<RotaryEncoder> leftButton,
                      std::shared_ptr<RotaryEncoder> rightButton,
                      std::shared_ptr<PotentiometerKnob> brightKnob,
                      std::shared_ptr<PotentiometerKnob> gainKnob,
                      std::shared_ptr<RemoteControl> remoteCtrl,
                      bool mirrorLedMode ) :
   m_activeAudioDisplayIndex(0),
   m_saveRestore(saveRestore),
   m_ledStrip(ledStrip),
   m_currentGradient(colorGrad->getGradient()),
   m_cycleGrads(cycleGrads),
   m_cycleDisplays(cycleDisplays),
   m_reverseGradToggle(reverseGrad),
   m_deleteButton(deleteButton),
   m_leftButton(leftButton),
   m_rightButton(rightButton),
   m_brightKnob(brightKnob),
   m_gainKnob(gainKnob),
   m_remoteCtrl(remoteCtrl)
{
   // Create the Button / Rotary Enocoder monitoring thread.
   m_remoteCtrl->clear(); // Clear out any previously stored commands.
   m_buttonMonitorThread_active = true;
   m_buttonMonitor_thread = std::thread(&AudioLeds::buttonMonitorFunc, this);

   // Set the Audio Displays (do this before creating the thread)
   auto numLeds = ledStrip->getNumLeds();
   // Amplitude based displays
   m_audioDisplayAmp.emplace_back(new AudioDisplayAmp(SAMPLE_RATE, AMP_DISP_FRAME_SIZE, numLeds, AudioDisplayAmp::E_SCALE,    0.125, AudioDisplayAmp::E_PEAK_GRAD_MID_CHANGE, mirrorLedMode));
   m_audioDisplayAmp.emplace_back(new AudioDisplayAmp(SAMPLE_RATE, AMP_DISP_FRAME_SIZE, numLeds, AudioDisplayAmp::E_MIN_SAME, 0.125, AudioDisplayAmp::E_PEAK_GRAD_MID_CONST, mirrorLedMode));
   m_audioDisplayAmp.emplace_back(new AudioDisplayAmp(SAMPLE_RATE, AMP_DISP_FRAME_SIZE, numLeds, AudioDisplayAmp::E_MAX_SAME, 0.125, AudioDisplayAmp::E_PEAK_GRAD_MIN, mirrorLedMode));
   for(auto& disp : m_audioDisplayAmp)
      m_audioDisplays.push_back(disp.get());

#ifndef NO_FFTS
   // Frequency based displays
   m_audioDisplayFft.emplace_back(new AudioDisplayFft(SAMPLE_RATE, FFT_SIZE, numLeds, AudioDisplayFft::E_GRADIENT_MAG, mirrorLedMode));
   m_audioDisplayFft.emplace_back(new AudioDisplayFft(SAMPLE_RATE, FFT_SIZE, numLeds, AudioDisplayFft::E_BRIGHTNESS_MAG, mirrorLedMode));
   for(auto& disp : m_audioDisplayFft)
     m_audioDisplays.push_back(disp.get());
#endif

   // Attempt to Restore settings.
   int restoredDisplayIndex = m_saveRestore->restore_displayIndex();
   if(restoredDisplayIndex >= 0 && restoredDisplayIndex < int(m_audioDisplays.size()))
      m_activeAudioDisplayIndex = restoredDisplayIndex;
   m_reverseGrad = m_saveRestore->restore_gradientReverse();

   // Make sure the first display gets set for the current gradient.
   m_audioDisplays[m_activeAudioDisplayIndex]->setGradient(m_currentGradient, m_reverseGrad);

   // Create the PCM Sample processing thread.
   m_pcmProc_buff.reserve(5000);
   m_pcmProc_active = true;
   m_pcmProc_thread = std::thread(&AudioLeds::pcmProcFunc, this);

   // Create the LED Update processing thread.
   m_ledUpdate_active = true;
   m_ledUpdate_thread = std::thread(&AudioLeds::ledUpdateFunc, this);

   // Start capturing from the microphone.
   m_mic.reset(new AlsaMic(microphoneName.c_str(), SAMPLE_RATE, MICROPHONE_FRAME_SIZE, 1, alsaMicSamples, this));
}

AudioLeds::~AudioLeds()
{
   // Save off current settings.
   m_saveRestore->save_displayIndex(m_activeAudioDisplayIndex);
   m_saveRestore->save_gradientReverse(m_reverseGrad);

   // Stop getting samples from the microphone.
   m_mic.reset();

   // Kill the PCM Sample processing thread and join.
   m_pcmProc_active = false;
   m_pcmProc_mutex.lock();
   m_pcmProc_bufferReadyCondVar.notify_all();
   m_pcmProc_mutex.unlock();
   m_pcmProc_thread.join();

   // Kill the LED Update processing thread and join.
   m_ledUpdate_active = false;
   m_ledUpdate_mutex.lock();
   m_ledUpdate_bufferReadyCondVar.notify_all();
   m_ledUpdate_mutex.unlock();
   m_ledUpdate_thread.join();
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
      auto changeGrad = checkForChange(m_cycleGrads->checkRotation(), m_remoteCtrl->checkGradientChange()); // Check both the rotary and remote control.
      if(changeGrad != SpecAnLedTypes::eDirection::E_DIRECTION_NO_CHANGE)
      {
         newGrad = (changeGrad == SpecAnLedTypes::eDirection::E_DIRECTION_POS ? m_saveRestore->restore_gradientNext() : m_saveRestore->restore_gradientPrev());
         loadNewGrad = true;
      }

      // Check if the user wants to change the Audio Display.
      auto changeDisplay = checkForChange(m_cycleDisplays->checkRotation(), m_remoteCtrl->checkDisplayChange()); // Check both the rotary and remote control.
      if(changeDisplay != SpecAnLedTypes::eDirection::E_DIRECTION_NO_CHANGE)
      {
         int delta = (changeDisplay == SpecAnLedTypes::eDirection::E_DIRECTION_POS ? 1 : -1);
         int max = m_audioDisplays.size();
         int newIndex = m_activeAudioDisplayIndex + delta;
         if(newIndex < 0) newIndex = max-1;
         else if(newIndex >= max) newIndex = 0;

         m_activeAudioDisplayIndex = newIndex;

         // Make sure the gradient gets updates in the new display (in case it changed since the last time the display was used).
         newGrad = m_currentGradient; 
         loadNewGrad = true;

         // Save off the new display index.
         m_saveRestore->save_displayIndex(m_activeAudioDisplayIndex);
      }

      // Check if the user want to reverse the gradient (use rotary and button).
      bool rotaryToggleGrad = m_reverseGradToggle->checkRotation() != RotaryEncoder::E_NO_CHANGE || m_reverseGradToggle->checkButton(true);
      bool remoteToggleGrad = m_remoteCtrl->checkReverseGradientToggle();
      if(rotaryToggleGrad || remoteToggleGrad)
      {
         newGrad = m_currentGradient;
         m_reverseGrad = !m_reverseGrad;
         loadNewGrad = true;
      }
      if(remoteToggleGrad)
         m_saveRestore->save_gradientReverse(m_reverseGrad); // Save the change.

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
   ThreadPriorities::setThisThreadName("PcmProcFunc"); // TODO this should have a thread priority
   auto& audioDisplay = m_audioDisplays[m_activeAudioDisplayIndex];
   size_t numSamp = audioDisplay->getFrameSize();
   SpecAnLedTypes::tRgbVector ledColors;
   SpecAnLedTypes::tPcmBuffer samplesForProcessing(numSamp);
   samplesForProcessing.reserve(numSamp);

   while(m_pcmProc_active)
   {
      auto& audioDisplay = m_audioDisplays[m_activeAudioDisplayIndex];
      numSamp = audioDisplay->getFrameSize();

      bool samplesReady = false;
      {
         std::unique_lock<std::mutex> lock(m_pcmProc_mutex);

         // Check if we have samples right now or if we need to wait.
         samplesReady = (m_pcmProc_buff.size() >= numSamp);
         if(!samplesReady)
         {
            auto result = m_pcmProc_bufferReadyCondVar.wait_for(lock, std::chrono::milliseconds(100));
            if(result == std::cv_status::timeout)
            {
               // Sometimes the ALSA driver stuff just stops sending samples. Killing the application
               // is only way I have found to fix this issue. 
               std::thread([](){ raise(SIGINT); }).detach(); // Kill from a different thread.
               
               m_pcmProc_active = false; // Exit out of this thread.
            }
            samplesReady = (m_pcmProc_buff.size() >= numSamp);
         }

         samplesReady = samplesReady && m_pcmProc_active;
         if(samplesReady)
         {
            if(m_pcmProc_buff.size() == numSamp)
            {
               // Exactly the correct number of samples are in the buffer. No need to copy, just swap.
               samplesForProcessing.resize(0);
               m_pcmProc_buff.swap(samplesForProcessing);
            }
            else
            {
               samplesForProcessing.resize(numSamp);
               memcpy(samplesForProcessing.data(), m_pcmProc_buff.data(), sizeof(samplesForProcessing[0])*numSamp);
               m_pcmProc_buff.erase(m_pcmProc_buff.begin(),m_pcmProc_buff.begin()+numSamp);
            }
         }
      }

      if(samplesReady)
      {
         // Send the samples to the Audio Display to generate the LED Colors.
         if(audioDisplay->parsePcm(samplesForProcessing.data(), numSamp))
         {
            float gain, brightness;
            updateGainBrightness(gain, brightness); // Get the current gain / brightness values.

            ledColors.resize(m_ledStrip->getNumLeds()); // Make sure this is big enough.
            audioDisplay->fillInLeds(ledColors, brightness, gain);

            // Move the LED color values to the buffer and handle them on another thread.
            {
               std::lock_guard<std::mutex> lock(m_ledUpdate_mutex);
               auto newIndex = m_ledUpdate_buff.size();
               m_ledUpdate_buff.resize(newIndex+1);
               ledColors.swap(m_ledUpdate_buff[newIndex]);
               m_ledUpdate_bufferReadyCondVar.notify_all();
            }
         }
      }
   }
}

void AudioLeds::ledUpdateFunc()
{
   ThreadPriorities::setThisThreadName("LedUpdateFunc"); // TODO this should have a thread priority
   std::unique_lock<std::mutex> lock(m_ledUpdate_mutex);
   while(m_ledUpdate_active)
   {
      // If buffer is empty, wait for something to do.
      while(m_ledUpdate_buff.size() == 0 && m_ledUpdate_active)
      {
         m_ledUpdate_bufferReadyCondVar.wait(lock);
      }

      // Update LED Strip.
      while(m_ledUpdate_buff.size() > 0 && m_ledUpdate_active)
      {
         // Move LED values locally so we can unlock the mutex.
         SpecAnLedTypes::tRgbVector ledColors;
         ledColors.swap(m_ledUpdate_buff[0]);
         m_ledUpdate_buff.erase(m_ledUpdate_buff.begin());

         // Update the LED strip (keep the mutex unlocked while doing this so more LED updates can be added to the buffer while this update is happing).
         lock.unlock();
         m_ledStrip->set(ledColors);
         lock.lock();
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

SpecAnLedTypes::eDirection AudioLeds::checkForChange(RotaryEncoder::eRotation rotary, RemoteControl::eDirection remote)
{
   switch(rotary) // Convert rotary movement to SpecAnLedTypes::eDirection
   {
      case RotaryEncoder::eRotation::E_FORWARD:
         return SpecAnLedTypes::eDirection::E_DIRECTION_POS;
      case RotaryEncoder::eRotation::E_BACKWARD:
         return SpecAnLedTypes::eDirection::E_DIRECTION_NEG;
      default:
         break; // Do nothing.
   }
   switch(remote) // Convert remote control type to SpecAnLedTypes::eDirection
   {
      case RemoteControl::eDirection::E_DIRECTION_POS:
         return SpecAnLedTypes::eDirection::E_DIRECTION_POS;
      case RemoteControl::eDirection::E_DIRECTION_NEG:
         return SpecAnLedTypes::eDirection::E_DIRECTION_NEG;
      default:
         break; // Do nothing.
   }
   return SpecAnLedTypes::eDirection::E_DIRECTION_NO_CHANGE;
}

void AudioLeds::updateGainBrightness(float& gain, float& brightness)
{
   // Grab the gain / brightness values (either from remote control of local potentiometer knobs)
   bool remoteBrightGain = m_remoteCtrl->useRemoteGainBrightness();
   brightness = remoteBrightGain ? m_remoteCtrl->getBrightness() : m_brightKnob->getFlt();
   gain = remoteBrightGain ? m_remoteCtrl->getGain() : m_gainKnob->getInt();

   // If remote values haven't been set, try to restore from JSON.
   if(brightness < 0)
      brightness = m_saveRestore->restore_brightness();
   else
      brightness = Transform1D::Unit::quarterCircle_below(brightness);  // Use the quarterCircle_below transform to provide more resolution at lower brightness levels.

   if(gain < 0)
      gain = m_saveRestore->restore_gain();

   if(remoteBrightGain)
   {
      // If in remote mode, save off the values.
      m_saveRestore->save_gain(gain);
      m_saveRestore->save_brightness(brightness);
   }
}
