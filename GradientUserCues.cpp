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
#include <chrono>
#include "GradientUserCues.h"
#include "ThreadPriorities.h"


GradientUserCues::GradientUserCues(std::shared_ptr<LedStrip> ledStrip, std::shared_ptr<PotentiometerKnob> brightKnob):
   m_ledStrip(ledStrip),
   m_brightKnob(brightKnob)
{

}

GradientUserCues::~GradientUserCues()
{
   cancel();
}


void GradientUserCues::startBlink(SpecAnLedTypes::tRgbVector& fullScale, size_t ledIndex)
{
   cancel();

   {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_activeCueThread.reset(new tCueThreads);
      m_activeCueThread->cueType = E_BLINK;
      m_activeCueThread->ledIndex = ledIndex;
      m_activeCueThread->earlyEnd = false;
      m_activeCueThread->fullScale = fullScale;
   }
   
   std::thread thread(&GradientUserCues::cueThread, this, m_activeCueThread);
   thread.detach();

}


void GradientUserCues::startFade(SpecAnLedTypes::tRgbVector& fullScale, size_t ledIndex, bool fadeIn)
{
   cancel();

   {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_activeCueThread.reset(new tCueThreads);
      m_activeCueThread->cueType = fadeIn ? E_FADE_IN : E_FADE_OUT;
      m_activeCueThread->ledIndex = ledIndex;
      m_activeCueThread->earlyEnd = false;
      m_activeCueThread->fullScale = fullScale;
   }
   
   std::thread thread(&GradientUserCues::cueThread, this, m_activeCueThread);
   thread.detach();
}


void GradientUserCues::cancel()
{
   std::unique_lock<std::mutex> lock(m_mutex);
   if(m_activeCueThread.get() != nullptr)
   {
      m_activeCueThread->earlyEnd = true;
      m_activeCueThread.reset(); // Done with this.
   }

}

SpecAnLedTypes::tRgbVector GradientUserCues::getBlankLedColors()
{
   SpecAnLedTypes::tRgbVector retVal(m_ledStrip->getNumLeds());
   for(auto& led : retVal)
   {
      led.u32 = SpecAnLedTypes::COLOR_BLACK;
   }
   return retVal;
}


SpecAnLedTypes::tRgbVector GradientUserCues::updateBrightness(SpecAnLedTypes::tRgbVector& fullScale)
{
   auto retVal = fullScale;
   float brightness = m_brightKnob->getFlt();
   for(auto& led : retVal)
   {
      led.rgb.r = (float)led.rgb.r * brightness;
      led.rgb.g = (float)led.rgb.g * brightness;
      led.rgb.b = (float)led.rgb.b * brightness;
   }
   return retVal;
}

void GradientUserCues::doBlink(std::shared_ptr<tCueThreads> thisCueThread, std::unique_lock<std::mutex>& lock)
{
   auto blank = getBlankLedColors();

   int numBlinks = 3;
   auto blinkTime = std::chrono::milliseconds(166);
   
   auto timerTime = std::chrono::steady_clock::now();
   for(int i = 0; i < numBlinks; ++i)
   {
      if(i > 0)
      {
         // Blink Off.
         m_ledStrip->set(blank);
         timerTime += blinkTime;

         lock.unlock();
         std::this_thread::sleep_until(timerTime);
         lock.lock();
         if(thisCueThread->earlyEnd)
         {
            break;
         }
      }

      // Blink On.
      auto leds = updateBrightness(thisCueThread->fullScale);
      m_ledStrip->set(leds);
      timerTime += blinkTime;
      
      lock.unlock();
      std::this_thread::sleep_until(timerTime);
      lock.lock();
      if(thisCueThread->earlyEnd)
      {
         break;
      }
   }

}

void GradientUserCues::doFade(std::shared_ptr<tCueThreads> thisCueThread, std::unique_lock<std::mutex>& lock, bool fadeIn)
{
   size_t numLeds = thisCueThread->fullScale.size();
   SpecAnLedTypes::tRgbVector ledColors;
   ledColors.resize(numLeds);

   int numIterations = 40;
   auto fadeLen = std::chrono::nanoseconds(2*1000*1000*1000);

   auto timeBetween = fadeLen / numIterations;

   for(int i = 0; i < numIterations; ++i)
   {
      float doneness = (float)i / (float)(numIterations - 1);
      float notDoneness = 1.0 - doneness;
      float scaleFactor = fadeIn ? doneness : notDoneness;
      scaleFactor *= m_brightKnob->getFlt();
      if(scaleFactor < 0.0)
         scaleFactor = 0.0;
      else if(scaleFactor > 1.0)
         scaleFactor = 1.0;
      
      for(size_t ledIndex = 0; ledIndex < numLeds; ++ledIndex)
      {
         ledColors[ledIndex].rgb.r = (float)thisCueThread->fullScale[ledIndex].rgb.r * scaleFactor;
         ledColors[ledIndex].rgb.g = (float)thisCueThread->fullScale[ledIndex].rgb.g * scaleFactor;
         ledColors[ledIndex].rgb.b = (float)thisCueThread->fullScale[ledIndex].rgb.b * scaleFactor;
      }
      m_ledStrip->set(ledColors);
      
      lock.unlock();
      std::this_thread::sleep_for(timeBetween);
      lock.lock();
      if(thisCueThread->earlyEnd)
         break;
   }

}

void GradientUserCues::cueThread(std::shared_ptr<tCueThreads> thisCueThread)
{
   ThreadPriorities::setThisThreadPriorityPolicy(ThreadPriorities::USER_CUE_THREAD_PRIORITY, SCHED_FIFO);
   ThreadPriorities::setThisThreadName("UserCue");

   std::unique_lock<std::mutex> lock(m_mutex);
   switch(thisCueThread->cueType)
   {
      case E_BLINK:
         doBlink(thisCueThread, lock);
      break;
      case E_FADE_IN:
      case E_FADE_OUT:
         bool fadeIn = thisCueThread->cueType == E_FADE_IN;
         doFade(thisCueThread, lock, fadeIn);
      break;
   }
   if(!thisCueThread->earlyEnd)
   {
      m_userCueJustFinished = true;
   }
}

bool GradientUserCues::userCueJustFinished()
{
   bool retVal = false;
   {
      std::unique_lock<std::mutex> lock(m_mutex);
      retVal = m_userCueJustFinished;
      m_userCueJustFinished = false;
   }
   return retVal;
}

