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
#include <unistd.h>
#include <vector>
#include "gradientChangeThread.h"
#include "seeed_adc_8chan_12bit.h"
#include "colorScale.h"
#include "gradientToScale.h"
#include "specAnLedPiTypes.h"

#include "smartPlotMessage.h"

GradChangeThread::GradChangeThread(std::shared_ptr<ColorGradient> colorGrad, std::shared_ptr<LedStrip> ledStrip, int dialAdcNum):
   m_colorGrad(colorGrad),
   m_ledStrip(ledStrip),
   m_dialAdcNum(dialAdcNum),
   m_gradOption(ColorGradient::E_GRAD_HUE),
   m_gradPointIndex(0),
   m_threadLives(true)
{
   m_thread = std::thread(&GradChangeThread::threadFunction, this);
}

GradChangeThread::~GradChangeThread()
{
   m_threadLives = false;
   if(m_thread.joinable())
      m_thread.join();
}


void GradChangeThread::setGradientOption(ColorGradient::eGradientOptions newOption)
{
   m_gradOption = newOption;
}

void GradChangeThread::setGradientPointIndex(int newPointIndex)
{
   if(newPointIndex >= 0 && newPointIndex < (int)m_colorGrad->getNumPoints())
   {
      m_gradPointIndex = newPointIndex;
   }
}


void GradChangeThread::threadFunction()
{
   SeeedAdc8Ch12Bit adc;
   std::vector<ColorScale::tColorPoint> colors;
   std::vector<ColorScale::tBrightnessPoint> bright;
   size_t numLeds = m_ledStrip->getNumLeds();
   SpecAnLedTypes::tRgbVector ledColors;
   ledColors.resize(numLeds);

   while(m_threadLives)
   {
      usleep(250*1000);
      if(m_threadLives)
      {
         int adcValue = adc.getAdcValue(m_dialAdcNum);
         float adcValueFloat = (float)adcValue / 4095.0;
         smartPlot_1D(&adcValueFloat, E_FLOAT_32, 1, 100, 1, "ADC", "Val");
         m_colorGrad->updateGradient(m_gradOption, adcValueFloat, m_gradPointIndex);

         std::vector<ColorGradient::tGradientPoint> grad = m_colorGrad->getGradient();
         Convert::convertGradientToScale(grad, colors, bright);

         ColorScale colorScale(colors, bright);

         float deltaBetweenPoints = (float)65535/(float)(numLeds-1);
         for(size_t i = 0 ; i < numLeds; ++i)
         {
            ledColors[i] = colorScale.getColor((float)i * deltaBetweenPoints);
         }
         m_ledStrip->set(ledColors);
      }
   }
}


