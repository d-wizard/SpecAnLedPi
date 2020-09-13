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
#include "wiringPi.h"
#include "rotaryEncoder.h"


RotaryEncoder::RotaryEncoder(int forwardFirstGpio, int backwardFirstGpio):
   m_forwardFirstGpio(forwardFirstGpio),
   m_backwardFirstGpio(backwardFirstGpio)
{
   pinMode(m_forwardFirstGpio, INPUT);
   pinMode(m_backwardFirstGpio, INPUT);
}

RotaryEncoder::RotaryEncoder(int buttonGpio):
   m_buttonGpio(buttonGpio)
{
   pinMode(m_buttonGpio, INPUT);
}

RotaryEncoder::RotaryEncoder(int forwardFirstGpio, int backwardFirstGpio, int buttonGpio):
   m_forwardFirstGpio(forwardFirstGpio),
   m_backwardFirstGpio(backwardFirstGpio),
   m_buttonGpio(buttonGpio)
{
   pinMode(m_forwardFirstGpio, INPUT);
   pinMode(m_backwardFirstGpio, INPUT);
   pinMode(m_buttonGpio, INPUT);
}

RotaryEncoder::~RotaryEncoder()
{

}

bool RotaryEncoder::checkButton(bool only1TruePerPress)
{
   bool retVal = false;
   if(m_buttonGpio > 0)
   {
      if(only1TruePerPress)
      {
         // Only return true if this is a transition from unpressed to pressed.
         bool curVal = digitalRead(m_buttonGpio);
         if(curVal != m_buttonPrevState)
         {
            retVal = curVal;
            m_buttonPrevState = retVal;
         }
      }
      else
      {
         retVal = digitalRead(m_buttonGpio);
         m_buttonPrevState = retVal;
      }
   }
   return retVal;
}

void RotaryEncoder::updateRotation()
{
   // This may be called at a very fast rate to simply record the state of the GPIOs.
   // Just store off the GPIOs states so they can be processed later.
   m_forwardFirstBuff[m_rotaryWriteIndex] = digitalRead(m_forwardFirstGpio);
   m_backwardFirstBuff[m_rotaryWriteIndex] = digitalRead(m_backwardFirstGpio);
   m_rotaryWriteIndex = (m_rotaryWriteIndex + 1) & CIRC_BUFF_MASK;
}


RotaryEncoder::eRotation RotaryEncoder::checkRotation()
{
   eRotation retVal = E_NO_CHANGE;

   while(m_rotaryReadIndex != m_rotaryWriteIndex && retVal == E_NO_CHANGE)
   {
      bool bothOn = (m_forwardFirstBuff[m_rotaryReadIndex] == 1 && m_backwardFirstBuff[m_rotaryReadIndex] == 1);
      if(m_forwardPrevState == 1 && m_backwardPrevState == 0 && bothOn)
      {
         retVal = E_FORWARD;
      }
      else if(m_forwardPrevState == 0 && m_backwardPrevState == 1 && bothOn)
      {
         retVal = E_BACKWARD;
      }
      m_forwardPrevState = m_forwardFirstBuff[m_rotaryReadIndex];
      m_backwardPrevState = m_backwardFirstBuff[m_rotaryReadIndex];
      m_rotaryReadIndex = (m_rotaryReadIndex + 1) & CIRC_BUFF_MASK;
   }

   if(retVal != E_NO_CHANGE)
   {
      // Try to remove as much from the buffer as possible (without removing the a new pulse)
      bool bothOff = (m_forwardFirstBuff[m_rotaryReadIndex] == 0 && m_backwardFirstBuff[m_rotaryReadIndex] == 0);
      while(m_rotaryReadIndex != m_rotaryWriteIndex && bothOff == false)
      {
         m_rotaryReadIndex = (m_rotaryReadIndex + 1) & CIRC_BUFF_MASK;
         bothOff = (m_forwardFirstBuff[m_rotaryReadIndex] == 0 && m_backwardFirstBuff[m_rotaryReadIndex] == 0);
         m_forwardPrevState = m_forwardFirstBuff[m_rotaryReadIndex];
         m_backwardPrevState = m_backwardFirstBuff[m_rotaryReadIndex];
      }

      while(m_rotaryReadIndex != m_rotaryWriteIndex && bothOff == true)
      {
         m_rotaryReadIndex = (m_rotaryReadIndex + 1) & CIRC_BUFF_MASK;
         bothOff = (m_forwardFirstBuff[m_rotaryReadIndex] == 0 && m_backwardFirstBuff[m_rotaryReadIndex] == 0);
         m_forwardPrevState = m_forwardFirstBuff[m_rotaryReadIndex];
         m_backwardPrevState = m_backwardFirstBuff[m_rotaryReadIndex];
      }
   }

   return retVal;
}

