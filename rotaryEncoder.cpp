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
#include <thread>
#include "wiringPi.h"
#include "rotaryEncoder.h"

int updn = PUD_UP;
RotaryEncoder::RotaryEncoder(ePinDefault pinDefault, int forwardFirstGpio, int backwardFirstGpio):
   m_forwardFirstGpio(forwardFirstGpio),
   m_backwardFirstGpio(backwardFirstGpio)
{
   auto pullUpDn = toWiringPiPullUpDn(pinDefault);
   m_defaultButtonVal = toWiringPiPullHiLo(pinDefault);
   pinMode(m_forwardFirstGpio, INPUT);
   pinMode(m_backwardFirstGpio, INPUT);
   pullUpDnControl(m_forwardFirstGpio, pullUpDn);
   pullUpDnControl(m_backwardFirstGpio, pullUpDn);
}

RotaryEncoder::RotaryEncoder(ePinDefault pinDefault, int buttonGpio):
   m_buttonGpio(buttonGpio)
{
   auto pullUpDn = toWiringPiPullUpDn(pinDefault);
   m_defaultButtonVal = toWiringPiPullHiLo(pinDefault);
   pinMode(m_buttonGpio, INPUT);
   pullUpDnControl(m_buttonGpio, pullUpDn);
}

RotaryEncoder::RotaryEncoder(ePinDefault pinDefault, int forwardFirstGpio, int backwardFirstGpio, int buttonGpio):
   m_forwardFirstGpio(forwardFirstGpio),
   m_backwardFirstGpio(backwardFirstGpio),
   m_buttonGpio(buttonGpio)
{
   auto pullUpDn = toWiringPiPullUpDn(pinDefault);
   m_defaultButtonVal = toWiringPiPullHiLo(pinDefault);
   pinMode(m_forwardFirstGpio, INPUT);
   pinMode(m_backwardFirstGpio, INPUT);
   pinMode(m_buttonGpio, INPUT);
   pullUpDnControl(m_forwardFirstGpio, pullUpDn);
   pullUpDnControl(m_backwardFirstGpio, pullUpDn);
   pullUpDnControl(m_buttonGpio, pullUpDn);
}

RotaryEncoder::~RotaryEncoder()
{

}

int RotaryEncoder::toWiringPiPullUpDn(ePinDefault val)
{
   return val == E_HIGH ? PUD_UP : PUD_DOWN;
}

int RotaryEncoder::toWiringPiPullHiLo(ePinDefault val)
{
   return val == E_HIGH ? HIGH : LOW;
}

bool RotaryEncoder::checkButton(bool only1TruePerPress)
{
   bool retVal = false;
   if(m_buttonGpio >= 0)
   {
      if(only1TruePerPress)
      {
         // Only return true if this is a transition from unpressed to pressed.
         bool curVal = digitalRead(m_buttonGpio) != m_defaultButtonVal;
         if(curVal != m_buttonPrevState)
         {
            retVal = curVal;
            m_buttonPrevState = retVal;
         }
      }
      else
      {
         retVal = digitalRead(m_buttonGpio) != m_defaultButtonVal;
         m_buttonPrevState = retVal;
      }
   }
   return retVal;
}

RotaryEncoder::eButtonClick RotaryEncoder::checkButton()
{
   eButtonClick retVal = E_NO_CLICK;

   if(checkButton(true))
   {
      retVal = E_SINGLE_CLICK;

      // Button Pressed. Wait for unpress.
      if(waitForButtonState(false, 10*1000, 750))
      {
         // Button was unpressed. Check if it was repressed quickly.
         if(waitForButtonState(true, 10*1000, 750))
         {
            retVal = E_DOUBLE_CLICK;
         }
      }
   }

   return retVal;
}

bool RotaryEncoder::waitForButtonState(bool state, uint64_t timeBetweenChecksNs, uint32_t timeoutMs)
{
   bool foundStateMatch = (checkButton(false) == state);
   if(!foundStateMatch)
   {
      auto startTime = std::chrono::steady_clock::now();
      auto nextTime = startTime;
      auto timeout = startTime + std::chrono::milliseconds(timeoutMs);
      auto timeBetweenChecks = std::chrono::nanoseconds(timeBetweenChecksNs);

      while( !foundStateMatch && (std::chrono::steady_clock::now() < timeout) )
      {
         foundStateMatch = (checkButton(false) == state);
         if(!foundStateMatch)
         {
            nextTime += timeBetweenChecks;
            std::this_thread::sleep_until(nextTime);
         }
      }

   }
   return foundStateMatch;
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
      if(m_waitForBothOff)
      {
         bool bothOff = (m_forwardFirstBuff[m_rotaryReadIndex] == 0 && m_backwardFirstBuff[m_rotaryReadIndex] == 0);
         if(bothOff)
         {
            m_waitForBothOff = false;
         }
      }
      else
      {
         bool bothOn = (m_forwardFirstBuff[m_rotaryReadIndex] == 1 && m_backwardFirstBuff[m_rotaryReadIndex] == 1);
         if(m_forwardPrevState == 1 && m_backwardPrevState == 0 && bothOn)
         {
            retVal = E_FORWARD;
            m_waitForBothOff = true;
         }
         else if(m_forwardPrevState == 0 && m_backwardPrevState == 1 && bothOn)
         {
            retVal = E_BACKWARD;
            m_waitForBothOff = true;
         }
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

         m_waitForBothOff = m_waitForBothOff && !bothOff;
      }
      m_waitForBothOff = m_waitForBothOff && !bothOff;

      while(m_rotaryReadIndex != m_rotaryWriteIndex && bothOff == true)
      {
         m_rotaryReadIndex = (m_rotaryReadIndex + 1) & CIRC_BUFF_MASK;
         bothOff = (m_forwardFirstBuff[m_rotaryReadIndex] == 0 && m_backwardFirstBuff[m_rotaryReadIndex] == 0);
         m_forwardPrevState = m_forwardFirstBuff[m_rotaryReadIndex];
         m_backwardPrevState = m_backwardFirstBuff[m_rotaryReadIndex];

         m_waitForBothOff = m_waitForBothOff && !bothOff;
      }
      m_waitForBothOff = m_waitForBothOff && !bothOff;
   }

   return retVal;
}

