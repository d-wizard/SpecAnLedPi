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
#include "specAnFft.h"

#include <math.h>

// Quake III Inverse Squareroot (see wiki for more info: https://en.wikipedia.org/wiki/Fast_inverse_square_root)
union floatAndInt
{
   float f;
   uint32_t i;
};
#define MAGIC_NUM (0x5f375a86)


static float quakeInvSqrt(float num)
{
   float half = num * 0.5;

   floatAndInt both;
   both.f = num;

   both.i = MAGIC_NUM - (both.i >> 1);
   both.f *= (1.5 - half * both.f * both.f);
   both.f *= (1.5 - half * both.f * both.f);
   return both.f;
}

static uint16_t magnatude(int16_t* complexSample)
{
   float squared = (int32_t)complexSample[0] * (int32_t)complexSample[0] + (int32_t)complexSample[1] * (int32_t)complexSample[1];
   uint16_t result = (uint16_t)(squared * quakeInvSqrt(squared) + 0.5);//sqrtf(squared);// TODO is Quake method even better than sqrtf?
   return result;
}


SpecAnFft::SpecAnFft(int numTaps)
{
   m_ne10Config = ne10_fft_alloc_r2c_int16(numTaps);
   m_tempComplex.resize(numTaps>>1);
}

SpecAnFft::~SpecAnFft()
{
   ne10_fft_destroy_r2c_int16(m_ne10Config);
}

void SpecAnFft::runFft(int16_t* inSamp, uint16_t* outSamp)
{
   ne10_fft_r2c_1d_int16_neon(m_tempComplex.data(), inSamp, m_ne10Config, 1);

   // Convert complex to real (i.e. do Pythagoras)
   size_t numComplexSamples = m_tempComplex.size();
   for(size_t i = 0; i < numComplexSamples; ++i)
   {
      outSamp[i] = magnatude((int16_t*)&m_tempComplex[i]);
   }
}

