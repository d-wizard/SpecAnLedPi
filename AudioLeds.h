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
#include "fftRunRate.h"
#include "fftModifier.h"

class AudioLeds
{
public:
   AudioLeds( std::shared_ptr<ColorGradient> colorGrad, 
              std::shared_ptr<LedStrip> ledStrip, 
              std::shared_ptr<RotaryEncoder> cycleGrads,
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
   std::unique_ptr<AlsaMic> mic;
   static void alsaMicSamples(void* usrPtr, int16_t* samples, size_t numSamp);

   // FFT Stuff
   std::unique_ptr<FftRunRate> fftRun;
   std::unique_ptr<FftModifier> fftModifier;

   // PCM Sample Processing Thread Stuff.
   std::thread processingThread;
   std::mutex bufferMutex;
   std::condition_variable bufferReadyCondVar;
   SpecAnLedTypes::tPcmBuffer pcmSampBuff;
   std::atomic<bool> procThreadLives;
   void processPcmSamples();

   // Button Monitor Thread Stuff.
   std::thread buttonMonitorThread;
   std::atomic<bool> buttonMonitorThreadLives;
   void buttonMonitorThreadFunc();


   // LED Stuff
   std::shared_ptr<LedStrip> m_ledStrip;
   SpecAnLedTypes::tRgbVector ledColors;
   std::unique_ptr<ColorScale> colorScale;

   // Knobs and Buttons
   std::shared_ptr<RotaryEncoder> m_cycleGrads;
   std::shared_ptr<RotaryEncoder> m_deleteButton;
   std::shared_ptr<RotaryEncoder> m_leftButton;
   std::shared_ptr<RotaryEncoder> m_rightButton;
   std::shared_ptr<PotentiometerKnob> m_brightKnob;
   std::shared_ptr<PotentiometerKnob> m_gainKnob;


};

