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
#include "ThreadPriorities.h"


class RotEncGradObj
{
public:
   RotEncGradObj(std::shared_ptr<RotaryEncoder> rotEnc, ColorGradient::eGradientOptions gradOption, float coarse, float fine):
      m_rotEnc(rotEnc),
      m_gradOption(gradOption),
      m_coarse(coarse),
      m_fine(fine)
   {}

   bool updateCoarseFine()
   {
      bool buttonJustUnpressed = false;
      bool buttonPressed = m_rotEnc->checkButton(false);
      if(buttonPressed != m_buttonPressed)
      {
         // State changed.
         if(!m_rotationActive && m_buttonPressed)
         {
            // Button unpressed while not rotating. Toggle Coarse / Fine.
            m_isCoarse = !m_isCoarse;
         }

         m_rotationActive = false; // Reset on state change.
         m_buttonPressed = buttonPressed;
         buttonJustUnpressed = !m_buttonPressed;
      }
      return buttonJustUnpressed;
   } 

   bool update(std::shared_ptr<ColorGradient> colorGrad, int gradPointIndex)
   {
      bool changed = false;
      auto dialValue = m_rotEnc->checkRotation();
      if(dialValue != RotaryEncoder::E_NO_CHANGE)
      {
         float change = m_isCoarse ? m_coarse : m_fine;
         float delta = (dialValue == RotaryEncoder::E_FORWARD) ? change : -change;

         // Make Reach change more intuitive for the end user.
         if(m_gradOption == ColorGradient::E_GRAD_REACH && gradPointIndex == 0)
         {
            // Make the color's reach direction match the orientation of the dial movement. 
            delta = -delta;
         }

         colorGrad->updateGradientDelta(m_gradOption, delta, gradPointIndex);
         changed = true;
         m_rotationActive = true;
      }
      return changed;
   }

   bool isButtonPressed()
   {
      return m_buttonPressed;
   }

   // Delete constructors / operations that should not be allowed.
   RotEncGradObj() = delete;
   RotEncGradObj(RotEncGradObj const&) = delete;
   void operator=(RotEncGradObj const&) = delete;

private:
   std::shared_ptr<RotaryEncoder> m_rotEnc;
   ColorGradient::eGradientOptions m_gradOption;
   float m_coarse;
   float m_fine;

   bool m_isCoarse = true;

   bool m_buttonPressed = false;
   bool m_rotationActive = false;
};


GradChangeThread::GradChangeThread(std::shared_ptr<ColorGradient> colorGrad, std::shared_ptr<LedStrip> ledStrip, spre hue, spre sat, spre ledSelect, spre reach, spre pos, spre leftBut, spre rightBut, std::shared_ptr<PotentiometerKnob> brightKnob):
   m_colorGrad(colorGrad),
   m_ledStrip(ledStrip),
   m_hueRotary(   new RotEncGradObj(hue,    ColorGradient::E_GRAD_HUE,        0.05, 0.003)),
   m_satRotary(   new RotEncGradObj(sat,    ColorGradient::E_GRAD_SATURATION, 0.1, 0.01)),
   m_reachRotary( new RotEncGradObj(reach,  ColorGradient::E_GRAD_REACH,      0.1, 0.01)),
   m_posRotary(   new RotEncGradObj(pos,    ColorGradient::E_GRAD_POSITION,   0.1, 0.01)),
   m_ledSelector(ledSelect),
   m_addButton(leftBut),
   m_removeButton(rightBut),
   m_brightKnob(brightKnob),
   m_gradPointIndex(0),
   m_threadLives(true)
{
   m_allGradRotaries.push_back(m_hueRotary);
   m_allGradRotaries.push_back(m_satRotary);
   m_allGradRotaries.push_back(m_reachRotary);
   m_allGradRotaries.push_back(m_posRotary);

   m_thread = std::thread(&GradChangeThread::threadFunction, this);
}

GradChangeThread::~GradChangeThread()
{
   m_threadLives = false;
   if(m_thread.joinable())
      m_thread.join();
}

void GradChangeThread::waitForThreadDone()
{
   m_thread.join();
}

void GradChangeThread::endThread()
{
   m_threadLives = false;
}

void GradChangeThread::setGradientPointIndex(int newPointIndex)
{
   if(newPointIndex >= 0 && newPointIndex < (int)m_colorGrad->getNumPoints())
   {
      m_gradPointIndex = newPointIndex;
   }
}

void GradChangeThread::waitForButtonUnpress(spre& button)
{
   while(button->checkButton(false) && m_threadLives) std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void GradChangeThread::threadFunction()
{
   ThreadPriorities::setThisThreadPriorityPolicy(ThreadPriorities::GRADIENT_CHANGE_THREAD_PRIORITY, SCHED_FIFO);
   ThreadPriorities::setThisThreadName("GradChange");

   bool updatedGradient = false;
   bool needToBlinkAfterFade = false;

   bool onlyShowOneColor = false;
   
   DisplayGradient display(m_colorGrad, m_ledStrip, m_brightKnob);
   display.showGradient();

   while(m_threadLives)
   {
      if(!updatedGradient)
      {
         usleep(10*1000);
      }
      updatedGradient = false;

      // Check for exit condition (i.e. add and remove buttons pressed at the same time).
      if(m_addButton->checkButton(false) && m_removeButton->checkButton(false))
      {
         m_threadLives = false;
      }

      if(m_threadLives)
      {
         bool blinking_fading = false;
         bool updateLeds = false;

         // Change selected LED / Blink selected LED
         auto ledChange = m_ledSelector->checkRotation();
         auto ledShow = m_ledSelector->checkButton(true);
         if(ledChange != RotaryEncoder::E_NO_CHANGE || ledShow)
         {
            int newColorIndex = m_gradPointIndex;
            if(ledChange == RotaryEncoder::E_FORWARD)
               newColorIndex--;
            else if(ledChange == RotaryEncoder::E_BACKWARD)
               newColorIndex++;
            if(newColorIndex < 0)
               newColorIndex = m_colorGrad->getNumPoints()-1;
            else if(newColorIndex >= (signed)m_colorGrad->getNumPoints())
               newColorIndex = 0;
            setGradientPointIndex(newColorIndex);
            display.blinkOne(m_gradPointIndex);
            blinking_fading = true;
            needToBlinkAfterFade = false;
         }

         // Add / Remove LED
         auto addButtonState = m_addButton->checkButton(); // Check Add Button.
         if(addButtonState == RotaryEncoder::E_DOUBLE_CLICK)
         {
            if(m_colorGrad->canAddPoint())
            {
               waitForButtonUnpress(m_addButton); // Don't do anything until the user releases the button.

               bool lastPoint = (m_gradPointIndex == ((signed)m_colorGrad->getNumPoints()-1));
               m_colorGrad->addPoint(m_gradPointIndex);
               if(!lastPoint)
                  setGradientPointIndex(m_gradPointIndex+1);
               display.fadeIn(m_gradPointIndex);
               blinking_fading = true;
               needToBlinkAfterFade = false;
            }
         }
         else if(addButtonState == RotaryEncoder::E_NO_CLICK) // Only check Remove LED button if Add Button was unclicked.
         {
            // Check Remove Button.
            if(m_removeButton->checkButton() == RotaryEncoder::E_DOUBLE_CLICK)
            {
               if(m_colorGrad->canRemovePoint())
               {
                  waitForButtonUnpress(m_removeButton); // Don't do anything until the user releases the button.
            
                  display.fadeOut(m_gradPointIndex);
                  m_colorGrad->removePoint(m_gradPointIndex);
                  setGradientPointIndex(m_gradPointIndex-1);
                  blinking_fading = true;
                  needToBlinkAfterFade = true;
               }
            }
         }

         // Update Based on Rotary Encoder states.
         for(auto& rotary : m_allGradRotaries)
         {
            // See if Coarse / Fine need to change. Keep track of whether a button was just unpressed.
            bool buttonUnpressed = rotary->updateCoarseFine();

            // Check if the rotary encoder change position.
            bool rotaryChanged = rotary->update(m_colorGrad, m_gradPointIndex);

            // Determine whether only one color should be displayed based on the button position.
            if(buttonUnpressed)
               onlyShowOneColor = false; // If only one color was being displayed, make sure the original display is done.
            else if(rotaryChanged)
               onlyShowOneColor = rotary->isButtonPressed(); // If the button is press when rotation is detected, only display the current color on the gradient.

            updateLeds = rotaryChanged || buttonUnpressed || updateLeds;
         }

         // If the special User Cue Display has finished, set the LEDs back to displaying the gradient.
         if(display.userCueDone())
         {
            updateLeds = true;
            if(needToBlinkAfterFade)
            {
               needToBlinkAfterFade = false;
               display.blinkOne(m_gradPointIndex);
               blinking_fading = true;
            }
         }

         // Check if the brightness knob has been changed.
         float dummyBrightness;
         if(m_brightKnob->getFlt(dummyBrightness))
         {
            updateLeds = true;
         }

         // Update the LEDs (if needed)
         if(!blinking_fading && updateLeds)
         {
            display.showGradient(onlyShowOneColor, m_gradPointIndex);
            updatedGradient = true;
         }

      }
   }
}


