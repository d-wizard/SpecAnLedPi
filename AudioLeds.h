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
#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "colorGradient.h"
#include "ledStrip.h"
#include "rotaryEncoder.h"
#include "potentiometerKnob.h"
#include "alsaMic.h"
#include "specAnFft.h"
#include "AudioDisplayBase.h"
#include "SaveRestoreGrad.h"

class AudioLeds
{
public:
   AudioLeds( std::shared_ptr<ColorGradient> colorGrad, 
              std::shared_ptr<SaveRestoreGrad> saveRestorGrad,
              std::shared_ptr<LedStrip> ledStrip, 
              std::shared_ptr<RotaryEncoder> cycleGrads,
              std::shared_ptr<RotaryEncoder> cycleDisplays,
              std::shared_ptr<RotaryEncoder> deleteButton,
              std::shared_ptr<RotaryEncoder> leftButton,
              std::shared_ptr<RotaryEncoder> rightButton,
              std::shared_ptr<PotentiometerKnob> brightKnob,
              std::shared_ptr<PotentiometerKnob> gainKnob );

   virtual ~AudioLeds();

   void waitForThreadDone();
   void endThread();

private:
   // Make uncopyable
   AudioLeds();
   AudioLeds(AudioLeds const&);
   void operator=(AudioLeds const&);

   // Microphone Capture
   std::unique_ptr<AlsaMic> m_mic;
   static void alsaMicSamples(void* usrPtr, int16_t* samples, size_t numSamp);

   // Audio Displays
   std::vector<std::unique_ptr<AudioDisplayBase>> m_audioDisplays;
   std::atomic<int> m_activeAudioDisplayIndex = 0;

   // PCM Sample Processing Thread Stuff.
   std::thread m_pcmProc_thread;
   std::mutex m_pcmProc_mutex;
   std::condition_variable m_pcmProc_bufferReadyCondVar;
   SpecAnLedTypes::tPcmBuffer m_pcmProc_buff;
   std::atomic<bool> m_pcmProc_active;
   void pcmProcFunc();

   // Button Monitor Thread Stuff.
   std::thread m_buttonMonitor_thread;
   std::atomic<bool> m_buttonMonitorThread_active;
   void buttonMonitorFunc();

   // Save Restore Gradient object
   std::shared_ptr<SaveRestoreGrad> m_saveRestorGrad;

   // LED Stuff
   std::shared_ptr<LedStrip> m_ledStrip;
   SpecAnLedTypes::tRgbVector m_ledColors;
   std::unique_ptr<ColorScale> m_colorScale;
   std::mutex m_colorScaleMutex;

   // Knobs and Buttons
   std::shared_ptr<RotaryEncoder> m_cycleGrads;
   std::shared_ptr<RotaryEncoder> m_cycleDisplays;
   std::shared_ptr<RotaryEncoder> m_deleteButton;
   std::shared_ptr<RotaryEncoder> m_leftButton;
   std::shared_ptr<RotaryEncoder> m_rightButton;
   std::shared_ptr<PotentiometerKnob> m_brightKnob;
   std::shared_ptr<PotentiometerKnob> m_gainKnob;


};

