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


SpecAnFft::SpecAnFft(int numTaps, bool window):
   m_numTaps(numTaps),
   m_window(window)
{
   m_ne10Config = ne10_fft_alloc_r2c_int16(numTaps);
   m_tempComplex.resize(numTaps>>1);
   if(m_window)
   {
      genWindowCoef(numTaps, false);
      m_windowedInput.resize(numTaps);
   }
}

SpecAnFft::~SpecAnFft()
{
   ne10_fft_destroy_r2c_int16(m_ne10Config);
}

void SpecAnFft::runFft(int16_t* inSamp, uint16_t* outSamp)
{
   int16_t* sampToFft = inSamp;
   if(m_window)
   {
      for(size_t i = 0; i < m_numTaps; ++i)
      {
         m_windowedInput[i] = ((int32_t)inSamp[i] * (int32_t)m_windowCoefs[i]) >> 15;
      }
      sampToFft = &m_windowedInput[0];
   }

   ne10_fft_r2c_1d_int16_neon(m_tempComplex.data(), sampToFft, m_ne10Config, 1);

   // Convert complex to real (i.e. do Pythagoras)
   size_t numComplexSamples = m_tempComplex.size();
   for(size_t i = 0; i < numComplexSamples; ++i)
   {
      outSamp[i] = magnatude((int16_t*)&m_tempComplex[i]);
   }
}


void SpecAnFft::genWindowCoef(unsigned int numSamp, bool scale)
{
   std::vector<double> outSamp(numSamp);
   double linearScaleFactor = 1.0;
#if 1
   // Blackman-Harris Window.
   double denom = numSamp-1;
   double twoPi  =  6.2831853071795864769252867665590057683943387987502116419;
   double fourPi = 12.5663706143591729538505735331180115367886775975004232839;
   double sixPi  = 18.8495559215387594307758602996770173051830163962506349258;

   double cos1 = twoPi  / denom;
   double cos2 = fourPi / denom;
   double cos3 = sixPi  / denom;

   for(unsigned int i = 0; i < numSamp; ++i)
   {
      double sampIndex = i;
      outSamp[i] =   0.35875
                   - 0.48829 * cos(cos1*sampIndex)
                   + 0.14128 * cos(cos2*sampIndex)
                   - 0.01168 * cos(cos3*sampIndex);
   }
   linearScaleFactor = 1.969124795; // Measured value.
#else
   // Standard Blackman Window.
   for(unsigned int i = 0; i < numSamp; ++i)
   {
      outSamp[i] = ( 0.42 - 0.5 * cos (2.0*M_PI*(double)i/(double)(numSamp-1))
         + 0.08 * cos (4.0*M_PI*(double)i/(double)(numSamp-1)) );
   }
   linearScaleFactor = 1.812030468; // Measured value.
#endif

   if(scale)
   {
      for(unsigned int i = 0; i < numSamp; ++i)
      {
         outSamp[i] *= linearScaleFactor;
      }
   }

   // Finally, convert to Q15 values.
   m_windowCoefs.resize(numSamp);
   for(unsigned int i = 0; i < numSamp; ++i)
   {
      int32_t q15Val = outSamp[i] * 32768.0;
      if(q15Val < 0)
         q15Val = 0;
      else if(q15Val > 0x7FFF)
         q15Val = 0x7FFF;
      m_windowCoefs[i] = q15Val;
   }
}