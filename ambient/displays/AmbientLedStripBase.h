/* Copyright 2023 Dan Williams. All Rights Reserved.
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
#include <stdio.h>
#include <memory>
#include <thread>
#include <atomic>
#include <math.h>
#include "ledStrip.h"
#include "colorGradient.h"
#include "colorScale.h"

class AmbientLedStripBase
{
public:
// Types
typedef float AmbDispFltType; // Use a typedef to easily switch between float and double.

public:
   AmbientLedStripBase(std::shared_ptr<LedStrip> ledStrip):
      AmbientLedStripBase(ledStrip, ColorGradient::GetRainbowGradient(), 1.0, true)
   {
   }
   AmbientLedStripBase(std::shared_ptr<LedStrip> ledStrip, const ColorGradient::tGradient& gradient, float gradientsToDisplayAtATime, bool forceGradientMirror):
      m_ledStrip(ledStrip),
      m_gradient(gradient),
      m_numLeds(ledStrip->getNumLeds()),
      m_forceGradientMirror(forceGradientMirror)
   {
      gradientsToDisplayAtATime = gradientsToDisplayAtATime <= 0.0 ? 1.0 : gradientsToDisplayAtATime; // Avoid divide by zero and negative numbers.
      m_numGradientCopies = ceil(gradientsToDisplayAtATime);
      m_numGradientCopies = (forceGradientMirror && (m_numGradientCopies & 1)) ? m_numGradientCopies + 1 : m_numGradientCopies; // If m_forceGradientMirror, make sure numGradientCopies is even.

      m_numBrightCopies = unsigned(float(m_numGradientCopies) / gradientsToDisplayAtATime);
      m_numBrightCopies = (m_numBrightCopies < 1) ? 1 : m_numBrightCopies; // Bound.

      printf("gradientsToDisplayAtATime = %f | m_numGradientCopies = %u | m_numBrightCopies = %u\n", gradientsToDisplayAtATime, m_numGradientCopies, m_numBrightCopies); fflush(stdout);
   }
   AmbientLedStripBase() = delete; AmbientLedStripBase(AmbientLedStripBase const&) = delete; void operator=(AmbientLedStripBase const&) = delete; // delete a bunch of constructors.
   virtual ~AmbientLedStripBase()
   {
      stopThread();
   }

   virtual void setGradient(const ColorGradient::tGradient& gradient)
   {
      m_gradient = gradient;
   }
protected:
   std::shared_ptr<LedStrip> m_ledStrip;
   ColorGradient::tGradient m_gradient;
   size_t m_numLeds;
   bool m_forceGradientMirror = true;

   unsigned m_numGradientCopies;
   unsigned m_numBrightCopies;

   virtual void updateLedStrip() = 0;
   
   void startThread()
   {
      m_threadActive = true;
      m_thread = std::thread(&AmbientLedStripBase::threadFunc, this);
   }
   void stopThread()
   {
      if(m_threadActive.load())
      {
         m_threadActive = false;
         if(m_thread.joinable())
            m_thread.join();
      }
   }
private:
   std::thread m_thread;
   std::atomic<bool> m_threadActive {false};

   void threadFunc()
   {
      while(m_threadActive.load())
      {
         updateLedStrip();
      }
   }
};
