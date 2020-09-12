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
#include "colorGradient.h"
#include "ledStrip.h"

class GradChangeThread
{
public:
   GradChangeThread(std::shared_ptr<ColorGradient> colorGrad, std::shared_ptr<LedStrip> ledStrip, int dialAdcNum);
   virtual ~GradChangeThread();

   void setGradientOption(ColorGradient::eGradientOptions newOption);
   void setGradientPointIndex(int newPointIndex);

private:
   // Make uncopyable
   GradChangeThread();
   GradChangeThread(GradChangeThread const&);
   void operator=(GradChangeThread const&);

   void threadFunction();

   std::shared_ptr<ColorGradient> m_colorGrad;
   std::shared_ptr<LedStrip> m_ledStrip;
   int m_dialAdcNum;

   std::thread m_thread;
   std::atomic<ColorGradient::eGradientOptions> m_gradOption;
   std::atomic<int> m_gradPointIndex;
   std::atomic<bool> m_threadLives;
};


