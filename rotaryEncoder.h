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

class RotaryEncoder
{
public:
   typedef enum
   {
      E_NO_CHANGE,
      E_FORWARD,
      E_BACKWARD
   }eRotation;

   RotaryEncoder(int forwardFirstGpio, int backwardFirstGpio);
   RotaryEncoder(int buttonGpio);
   RotaryEncoder(int forwardFirstGpio, int backwardFirstGpio, int buttonGpio);
   virtual ~RotaryEncoder();

   void updateRotation();

   eRotation checkRotation();
   //bool checkButton(bool only1TruePerPress);

private:
   // Make uncopyable
   RotaryEncoder();
   RotaryEncoder(RotaryEncoder const&);
   void operator=(RotaryEncoder const&);

   int m_forwardFirstGpio = -1;
   int m_backwardFirstGpio = -1;
   int m_buttonGpio = -1;

   static constexpr int CIRC_BUFF_SIZE = 1024; // must be base 2 number
   static constexpr int CIRC_BUFF_MASK = CIRC_BUFF_SIZE-1;
   int m_forwardFirstBuff[CIRC_BUFF_SIZE];
   int m_backwardFirstBuff[CIRC_BUFF_SIZE];
   int m_forwardPrevState = 0;
   int m_backwardPrevState = 0;
   int m_rotaryReadIndex = 0;
   int m_rotaryWriteIndex = 0;
};