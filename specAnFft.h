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

#include <stdint.h>
#include <vector>
#include "NE10.h"

class SpecAnFft
{
public:
   SpecAnFft(int numTaps, bool window = true);
   virtual ~SpecAnFft();

   void runFft(int16_t* inSamp, uint16_t* outSamp);
private:
   // Make uncopyable
   SpecAnFft();
   SpecAnFft(SpecAnFft const&);
   void operator=(SpecAnFft const&);

   ne10_fft_r2c_cfg_int16_t m_ne10Config;
   std::vector<ne10_fft_cpx_int16_t> m_tempComplex;

   int m_numTaps;
   bool m_window;
   std::vector<int16_t> m_windowCoefs;
   std::vector<int16_t> m_windowedInput;
   void genWindowCoef(unsigned int numSamp, bool scale);
};

