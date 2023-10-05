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
#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "ledStrip.h"
#include "rotaryEncoder.h"
#include "potentiometerKnob.h"
#include "alsaMic.h"
#include "specAnFft.h"
#include "AudioDisplayBase.h"
#include "AudioDisplayAmplitude.h"
#include "AudioDisplayFft.h"
#include "SaveRestore.h"
#include "RemoteControl.h"

class AudioLeds
{
public:
   AudioLeds( std::string microphoneName,
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
              bool mirrorLedMode );

   virtual ~AudioLeds();

   void waitForThreadDone();
   void endThread();

private:
   // Make uncopyable
   AudioLeds();
   AudioLeds(AudioLeds const&);
   void operator=(AudioLeds const&);

   // Check for a change via rotary enocder or remote control.
   SpecAnLedTypes::eDirection checkForChange(RotaryEncoder::eRotation rotary, RemoteControl::eDirection remote);

   // Update Gain and Brightness
   void updateGainBrightness(float& gain, float& brightness);

   // Microphone Capture
   std::unique_ptr<AlsaMic> m_mic;
   static void alsaMicSamples(void* usrPtr, int16_t* samples, size_t numSamp);

   // Audio Displays
   std::vector<std::unique_ptr<AudioDisplayAmp>> m_audioDisplayAmp;
   std::vector<std::unique_ptr<AudioDisplayFft>> m_audioDisplayFft;
   std::vector<AudioDisplayBase*> m_audioDisplays;
   std::atomic<int> m_activeAudioDisplayIndex;

   // PCM Sample Processing Thread Stuff.
   std::thread m_pcmProc_thread;
   std::mutex m_pcmProc_mutex;
   std::condition_variable m_pcmProc_bufferReadyCondVar;
   SpecAnLedTypes::tPcmBuffer m_pcmProc_buff;
   std::atomic<bool> m_pcmProc_active;
   void pcmProcFunc();

   // LED Update Thread Stuff.
   std::thread m_ledUpdate_thread;
   std::mutex m_ledUpdate_mutex;
   std::condition_variable m_ledUpdate_bufferReadyCondVar;
   std::vector<SpecAnLedTypes::tRgbVector> m_ledUpdate_buff;
   std::atomic<bool> m_ledUpdate_active;
   void ledUpdateFunc();

   // Button Monitor Thread Stuff.
   std::thread m_buttonMonitor_thread;
   std::atomic<bool> m_buttonMonitorThread_active;
   void buttonMonitorFunc();

   // Save Restore Gradient object
   std::shared_ptr<SaveRestoreJson> m_saveRestore;

   // LED Stuff
   std::shared_ptr<LedStrip> m_ledStrip;
   ColorGradient::tGradient m_currentGradient;
   bool m_reverseGrad = false;

   // Knobs and Buttons
   std::shared_ptr<RotaryEncoder> m_cycleGrads;
   std::shared_ptr<RotaryEncoder> m_cycleDisplays;
   std::shared_ptr<RotaryEncoder> m_reverseGradToggle;
   std::shared_ptr<RotaryEncoder> m_deleteButton;
   std::shared_ptr<RotaryEncoder> m_leftButton;
   std::shared_ptr<RotaryEncoder> m_rightButton;
   std::shared_ptr<PotentiometerKnob> m_brightKnob;
   std::shared_ptr<PotentiometerKnob> m_gainKnob;

   // Remote Control Inteface
   std::shared_ptr<RemoteControl> m_remoteCtrl;

};

