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
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include "gradientChangeThread.h"
#include "colorScale.h"
#include "DisplayGradient.h"
#include "specAnLedPiTypes.h"

GradChangeThread::GradChangeThread(std::shared_ptr<ColorGradient> colorGrad, std::shared_ptr<LedStrip> ledStrip, spre hue, spre sat, spre bright, spre reach, spre pos, spre color, spre addRem):
   m_colorGrad(colorGrad),
   m_ledStrip(ledStrip),
   m_hueRotary(hue),
   m_satRotary(sat),
   m_brightRotary(bright),
   m_reachRotary(reach),
   m_posRotary(pos),
   m_colorButton(color),
   m_addRemoveButton(addRem),
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
   bool updatedGradient = false;
   bool fineTune = false;
   
   DisplayGradient display(m_colorGrad, m_ledStrip);
   display.showGradient();

   while(m_threadLives)
   {
      if(!updatedGradient)
      {
         usleep(10*1000);
      }
      updatedGradient = false;

      if(m_threadLives)
      {
         bool updateLeds = false;
         if(m_hueRotary->checkButton(true) || m_satRotary->checkButton(true) || m_brightRotary->checkButton(true) || m_reachRotary->checkButton(true) || m_posRotary->checkButton(true) )
         {
            fineTune = !fineTune;
         }

         switch(m_colorButton->checkButton())
         {
            case RotaryEncoder::E_SINGLE_CLICK:
            {
               auto newColorIndex = (m_gradPointIndex >= (m_colorGrad->getNumPoints()-1)) ? 0 : m_gradPointIndex+1;
               setGradientPointIndex(newColorIndex);
               display.blinkOne(m_gradPointIndex);
               updateLeds = true;
            }
            break;
            case RotaryEncoder::E_DOUBLE_CLICK:
            {
               auto newColorIndex = (m_gradPointIndex <= 0) ? m_colorGrad->getNumPoints()-1 : m_gradPointIndex-1;
               setGradientPointIndex(newColorIndex);
               display.blinkOne(m_gradPointIndex);
               updateLeds = true;
            }
            break;
         }

         switch(m_addRemoveButton->checkButton())
         {
            case RotaryEncoder::E_SINGLE_CLICK:
            {
               bool lastPoint = (m_gradPointIndex == (m_colorGrad->getNumPoints()-1));
               m_colorGrad->addPoint(m_gradPointIndex);
               if(!lastPoint)
                  setGradientPointIndex(m_gradPointIndex+1);
               display.fadeIn(m_gradPointIndex);
               updateLeds = true;
            }
            break;
            case RotaryEncoder::E_DOUBLE_CLICK:
            {
               display.fadeOut(m_gradPointIndex);
               m_colorGrad->removePoint(m_gradPointIndex);
               setGradientPointIndex(m_gradPointIndex-1);
               updateLeds = true;
            }
            break;
         }

         bool foundRotary = false;
         RotaryEncoder::eRotation dialValue;
         float change = fineTune ? 0.01 : 0.1;

         dialValue = m_hueRotary->checkRotation();
         if(!foundRotary && dialValue != RotaryEncoder::E_NO_CHANGE)
         {
            float delta = (dialValue == RotaryEncoder::E_FORWARD) ? change : -change;
            m_colorGrad->updateGradientDelta(ColorGradient::E_GRAD_HUE, delta, m_gradPointIndex);
            updateLeds = true;
            foundRotary = true;
         }
         dialValue = m_satRotary->checkRotation();
         if(!foundRotary && dialValue != RotaryEncoder::E_NO_CHANGE)
         {
            float delta = (dialValue == RotaryEncoder::E_FORWARD) ? change : -change;
            m_colorGrad->updateGradientDelta(ColorGradient::E_GRAD_SATURATION, delta, m_gradPointIndex);
            updateLeds = true;
            foundRotary = true;
         }
         dialValue = m_brightRotary->checkRotation();
         if(!foundRotary && dialValue != RotaryEncoder::E_NO_CHANGE)
         {
            float delta = (dialValue == RotaryEncoder::E_FORWARD) ? change : -change;
            m_colorGrad->updateGradientDelta(ColorGradient::E_GRAD_LIGHTNESS, delta, m_gradPointIndex);
            updateLeds = true;
            foundRotary = true;
         }
         dialValue = m_reachRotary->checkRotation();
         if(!foundRotary && dialValue != RotaryEncoder::E_NO_CHANGE)
         {
            float delta = (dialValue == RotaryEncoder::E_FORWARD) ? change : -change;
            m_colorGrad->updateGradientDelta(ColorGradient::E_GRAD_REACH, delta, m_gradPointIndex);
            updateLeds = true;
            foundRotary = true;
         }
         dialValue = m_posRotary->checkRotation();
         if(!foundRotary && dialValue != RotaryEncoder::E_NO_CHANGE)
         {
            float delta = (dialValue == RotaryEncoder::E_FORWARD) ? change : -change;
            m_colorGrad->updateGradientDelta(ColorGradient::E_GRAD_POSITION, delta, m_gradPointIndex);
            updateLeds = true;
            foundRotary = true;
         }

         if(updateLeds)
         {
            display.showGradient();
            updatedGradient = true;
         }

      }
   }
}


