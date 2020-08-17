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
#include <math.h>
#include "fftModifier.h"


FftModifier::FftModifier(float samplesRate, int fftSize, int numOutputValues, tFftModifiers& modifiers):
   m_logScale(modifiers.logScale)
{
   initIndexMap(samplesRate, fftSize, numOutputValues, modifiers);
   initScale(modifiers);
}

FftModifier::~FftModifier()
{

}

int FftModifier::modify(uint16_t* inOut)
{
   int numOuts = m_indexMap.size()-1;
   for(int outIndex = 0; outIndex < numOuts; ++outIndex)
   {
      int64_t sum = 0;
      int numInSum = 0;
      for(int inIndex = m_indexMap[outIndex]; inIndex < m_indexMap[outIndex+1]; ++inIndex)
      {
         sum += inOut[inIndex];
         numInSum++;
      }
      sum /= numInSum;

      sum = ((sum - m_offset) * m_scalar) >> 15;
      
      if(sum > 0xFFFF) sum = 0xFFFF;
      else if(sum < 0) sum = 0;
      inOut[outIndex] = sum;
   }
   if(m_logScale)
      logScale(inOut, numOuts);

   return numOuts;
}


void FftModifier::logScale(uint16_t* inOut, int num)
{
   static constexpr float maxU16 = (float)0xFFFF;
   static constexpr float scalar = maxU16 / 4.8165; // log(0xFFFF) ~= 4.8165 (make sure to round up so scaler is slightly smaller)
   for(int i = 0; i < num; ++i)
   {
      if(inOut[i] == 0) inOut[i] = 1; // Log of 0 is invalid, set to 1 to avoid.

      inOut[i] = logf((float)inOut[i]) * scalar;
   }
}

float FftModifier::spliceToFreq(float splice, float range, bool isStop)
{
   float value = isStop ? range + splice : splice;
   while(value < 0) value += range;
   while(value > range) value -= range;
   return value;
}

void FftModifier::initIndexMap(float samplesRate, int fftSize, int numOutputValues, tFftModifiers& modifiers)
{
   float freqRange = samplesRate / 2.0;
   float hzPerBin = samplesRate/((float)fftSize);
   m_indexMap.resize(numOutputValues+1);

   float startFreq = spliceToFreq(modifiers.startFreq, freqRange, false);
   float stopFreq  = spliceToFreq(modifiers.stopFreq,  freqRange, true);

   float hzPerOutput = (stopFreq-startFreq) / numOutputValues;

   float binsPerOutput = hzPerOutput / hzPerBin;
   float startBin = startFreq / hzPerBin;

   for(int i = 0; i < (numOutputValues+1); ++i)
   {
      m_indexMap[i] = startBin + binsPerOutput * i;
   }
}

void FftModifier::initScale(tFftModifiers& modifiers)
{
   // Map modifiers clip range to 0 to 0xFFFF
   float rangeIn = modifiers.clipMax - modifiers.clipMin;
   float rangeOut = (float)(0xFFFF);

   float scalar = rangeOut / rangeIn;
   m_scalar = scalar * 32768.0; // Q15
   m_offset = modifiers.clipMin;
}


