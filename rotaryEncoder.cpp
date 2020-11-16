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

RotaryEncoder::tWaitState RotaryEncoder::getNextState(eWaitState changingState, eRotation rotationFromOff)
{
   RotaryEncoder::tWaitState retVal;
   switch(changingState)
   {
      default:
      case E_WAIT_EITHER:
         retVal.waitEnum = rotationFromOff == E_FORWARD ? E_FORWARD_WAIT_BOTH : E_BACK_WAIT_BOTH;
         retVal.desiredForwardVal = !m_defaultButtonVal;
         retVal.desiredBackwardVal = !m_defaultButtonVal;
      break;
      case E_FORWARD_WAIT_BOTH:
         retVal.waitEnum = E_FORWARD_WAIT_BACK;
         retVal.desiredForwardVal = m_defaultButtonVal;
         retVal.desiredBackwardVal = !m_defaultButtonVal;
      break;
      case E_FORWARD_WAIT_BACK:
         retVal.waitEnum = E_FORWARD_WAIT_OFF;
         retVal.desiredForwardVal = m_defaultButtonVal;
         retVal.desiredBackwardVal = m_defaultButtonVal;
      break;
      case E_FORWARD_WAIT_OFF:
         retVal.waitEnum = E_WAIT_EITHER; // Desired values don't matter.
      break;
      case E_BACK_WAIT_BOTH:
         retVal.waitEnum = E_BACK_WAIT_FORWARD;
         retVal.desiredForwardVal = !m_defaultButtonVal;
         retVal.desiredBackwardVal = m_defaultButtonVal;
      break;
      case E_BACK_WAIT_FORWARD:
         retVal.waitEnum = E_BACK_WAIT_OFF;
         retVal.desiredForwardVal = m_defaultButtonVal;
         retVal.desiredBackwardVal = m_defaultButtonVal;
      break;
      case E_BACK_WAIT_OFF:
         retVal.waitEnum = E_WAIT_EITHER; // Desired values don't matter.
      break;
   }
   return retVal;
}

// Loops through the saved Forward and Backward samples until either there are not more
// sample to process or a state transition has occurred. 
// The return value will be true if there are no more sample to process.
// If a state transition has occurred and it represent the end of the rotation 'finishedRotation'
// will be filled in with the rotation direction.
bool RotaryEncoder::waitForStateChange(eRotation& finishedRotation)
{
   bool empty = m_rotaryReadIndex == m_rotaryWriteIndex;
   finishedRotation = E_NO_CHANGE;

   if(m_curWaitState.waitEnum == E_WAIT_EITHER)
   {
      // Keep reading until 1 bit is set and the other isn't.
      bool newRotationFound = false;
      eRotation rotationForNextState;

      while(!empty && !newRotationFound)
      {
         if(m_forwardFirstBuff[m_rotaryReadIndex] != m_backwardFirstBuff[m_rotaryReadIndex])
         {
            newRotationFound = true;
            rotationForNextState = m_forwardFirstBuff[m_rotaryReadIndex] != m_defaultButtonVal ? E_FORWARD : E_BACKWARD;
         }

         m_rotaryReadIndex = (m_rotaryReadIndex + 1) & CIRC_BUFF_MASK;
         empty = m_rotaryReadIndex == m_rotaryWriteIndex;
      }
      
      if(newRotationFound)
      {
         m_curWaitState = getNextState(m_curWaitState.waitEnum, rotationForNextState);
      }
   }
   else
   {
      // Keep reading until the Next State is detected or a reset is detected.
      bool nextStateFound = false;
      bool resetFound = false;

      while(!empty && !nextStateFound && !resetFound)
      {
         nextStateFound = m_forwardFirstBuff[m_rotaryReadIndex] == m_curWaitState.desiredForwardVal && 
                          m_backwardFirstBuff[m_rotaryReadIndex] == m_curWaitState.desiredBackwardVal;
         resetFound = m_forwardFirstBuff[m_rotaryReadIndex] == m_defaultButtonVal && m_backwardFirstBuff[m_rotaryReadIndex] == m_defaultButtonVal;

         m_rotaryReadIndex = (m_rotaryReadIndex + 1) & CIRC_BUFF_MASK;
         empty = m_rotaryReadIndex == m_rotaryWriteIndex;
      }

      if(nextStateFound)
      {
         // Check for finished rotation.
         if(m_curWaitState.waitEnum == E_FORWARD_WAIT_OFF)
            finishedRotation = E_FORWARD; // Finsihed Forward Rotation.
         else if(m_curWaitState.waitEnum == E_BACK_WAIT_OFF)
            finishedRotation = E_BACKWARD; // Finsihed Backward Rotation.

         // Advance the next state.
         m_curWaitState = getNextState(m_curWaitState.waitEnum);
      }
      else if(resetFound)
      {
         // Reset
         m_curWaitState.waitEnum = E_WAIT_EITHER;
      }
   }

   return empty;
}

RotaryEncoder::eRotation RotaryEncoder::checkRotation()
{
   eRotation retVal = E_NO_CHANGE;
   bool empty = false;

   // Keep checking until either we found a Forward or Backward pulse or the buffers are empty.
   while(retVal == E_NO_CHANGE && !empty)
   {
      empty = waitForStateChange(retVal);
   }

   // If the buffers aren't empty, try to remove as much as possible (without removing a new pulse)
   while(!empty && m_curWaitState.waitEnum == E_WAIT_EITHER)
   {
      eRotation dummy = E_NO_CHANGE;
      empty = waitForStateChange(dummy);
   }
   return retVal;
}
