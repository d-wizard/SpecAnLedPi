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
#include <memory>
#include <thread>
#include <atomic>
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
      m_ledStrip(ledStrip),
      m_numLeds(ledStrip->getNumLeds())
   {
      // Rainbow Gradient
      ColorGradient::tGradientPoint gradPoint;
      gradPoint.saturation = 1.0;
      gradPoint.lightness  = 1.0;

      // Rainbow Gradient
      static const int numGradPoints = 10;
      for(int i = 0; i < numGradPoints; ++i)
      {
         gradPoint.hue = double(i) / double(numGradPoints-1);
         gradPoint.position = gradPoint.hue;
         m_gradient.push_back(gradPoint);
      }
   }
   AmbientLedStripBase(std::shared_ptr<LedStrip> ledStrip, const ColorGradient::tGradient& gradient):
      m_ledStrip(ledStrip),
      m_gradient(gradient),
      m_numLeds(ledStrip->getNumLeds())
   {
   }
   AmbientLedStripBase() = delete; AmbientLedStripBase(AmbientLedStripBase const&) = delete; void operator=(AmbientLedStripBase const&) = delete; // delete a bunch of constructors.
   virtual ~AmbientLedStripBase()
   {
      m_threadActive = false;
      if(m_thread.joinable())
         m_thread.join();
   }

   virtual void setGradient(const ColorGradient::tGradient& gradient)
   {
      m_gradient = gradient;
   }
protected:
   std::shared_ptr<LedStrip> m_ledStrip;
   ColorGradient::tGradient m_gradient;
   size_t m_numLeds;

   virtual void updateLedStrip() = 0;
   
   void startThread()
   {
      m_threadActive = true;
      m_thread = std::thread(&AmbientLedStripBase::threadFunc, this);
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
