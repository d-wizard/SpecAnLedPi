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

#include <chrono>

class RotaryEncoder
{
public:
   typedef enum
   {
      E_NO_CHANGE,
      E_FORWARD,
      E_BACKWARD
   }eRotation;

   typedef enum
   {
      E_NO_CLICK,
      E_SINGLE_CLICK,
      E_DOUBLE_CLICK
   }eButtonClick;


   typedef enum
   {
      E_HIGH,
      E_LOW
   }ePinDefault;

   RotaryEncoder(ePinDefault pinDefault, int forwardFirstGpio, int backwardFirstGpio);
   RotaryEncoder(ePinDefault pinDefault, int buttonGpio);
   RotaryEncoder(ePinDefault pinDefault, int forwardFirstGpio, int backwardFirstGpio, int buttonGpio);
   virtual ~RotaryEncoder();

   bool checkButton(bool only1TruePerPress);
   eButtonClick checkButton();

   void updateRotation();

   eRotation checkRotation();


private:
   // Make uncopyable
   RotaryEncoder();
   RotaryEncoder(RotaryEncoder const&);
   void operator=(RotaryEncoder const&);

   int toWiringPiPullUpDn(ePinDefault val);
   int toWiringPiPullHiLo(ePinDefault val);

   template <class Rep1, class Period1, class Rep2, class Period2>
   bool waitForButtonState(bool state, const std::chrono::duration<Rep1,Period1>& timeBetweenChecksNs, const std::chrono::duration<Rep2,Period2>& timeoutMs);

   // Types, variables, and funtions for detecting a Forward or Backward change in the Rotary Encoder.
   typedef enum
   {
      E_WAIT_EITHER,
      E_FORWARD_WAIT_BOTH,
      E_FORWARD_WAIT_BACK,
      E_FORWARD_WAIT_OFF,
      E_BACK_WAIT_BOTH,
      E_BACK_WAIT_FORWARD,
      E_BACK_WAIT_OFF,
   }eWaitState;

   typedef struct tWaitState
   {
      eWaitState waitEnum;
      int desiredForwardVal;
      int desiredBackwardVal;
      tWaitState():waitEnum(E_WAIT_EITHER), desiredForwardVal(0), desiredBackwardVal(0){}
   }tWaitState;
   
   tWaitState getNextState(eWaitState changingState, eRotation rotationFromOff = E_NO_CHANGE);
   bool waitForStateChange(eRotation& finishedRotation);

   tWaitState m_curWaitState;

   int m_forwardFirstGpio = -1;
   int m_backwardFirstGpio = -1;
   int m_buttonGpio = -1;

   int m_defaultButtonVal = -1;

   static constexpr int CIRC_BUFF_SIZE = 1024; // must be base 2 number
   static constexpr int CIRC_BUFF_MASK = CIRC_BUFF_SIZE-1;
   int m_forwardFirstBuff[CIRC_BUFF_SIZE];
   int m_backwardFirstBuff[CIRC_BUFF_SIZE];
   int m_rotaryReadIndex = 0;
   int m_rotaryWriteIndex = 0;

   bool m_buttonPrevState = false;

};
