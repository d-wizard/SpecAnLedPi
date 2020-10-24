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
#include <mutex>
#include <atomic>
#include <memory>
#include <condition_variable>
#include "specAnLedPiTypes.h"
#include "ledStrip.h"
#include "potentiometerAdc.h"


class GradientUserCues
{
private:
   typedef enum
   {
      E_BLINK,
      E_FADE_IN,
      E_FADE_OUT
   }eUserCueType;

   typedef struct
   {
      std::atomic<bool> earlyEnd;
      size_t ledIndex;
      eUserCueType cueType;
      SpecAnLedTypes::tRgbVector fullScale;
   }tCueThreads;

public:
   GradientUserCues(std::shared_ptr<LedStrip> ledStrip, std::shared_ptr<PotentiometerAdc> brightPot);
   virtual ~GradientUserCues();

   void startBlink(SpecAnLedTypes::tRgbVector& fullScale, size_t ledIndex);
   void startFade(SpecAnLedTypes::tRgbVector& fullScale, size_t ledIndex, bool fadeIn);

   void cancel();

   // Delete constructors / operations that should not be allowed.
   GradientUserCues() = delete;
   GradientUserCues(GradientUserCues const&) = delete;
   void operator=(GradientUserCues const&) = delete;

   bool userCueJustFinished();

private:
   SpecAnLedTypes::tRgbVector getBlankLedColors();
   SpecAnLedTypes::tRgbVector updateBrightness(SpecAnLedTypes::tRgbVector& fullScale);

   void doBlink(std::shared_ptr<tCueThreads> thisCueThread, std::unique_lock<std::mutex>& lock);
   void doFade(std::shared_ptr<tCueThreads> thisCueThread, std::unique_lock<std::mutex>& lock, bool fadeIn);

   void cueThread(std::shared_ptr<tCueThreads> thisCueThread);
   std::mutex m_mutex;

   std::shared_ptr<LedStrip> m_ledStrip;
   std::shared_ptr<PotentiometerAdc> m_brightPot;


   std::shared_ptr<tCueThreads> m_activeCueThread;

   std::atomic<bool> m_userCueJustFinished;

}; 
