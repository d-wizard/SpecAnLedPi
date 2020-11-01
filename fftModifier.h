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

typedef struct tFftModifiers
{
   float startFreq;
   float stopFreq;

   uint16_t clipMax;
   uint16_t clipMin;

   bool logScale;

   bool attenLowFreqs;
   float attenLowStartLevel;
   float attenLowStopFreq;

   int fadeAwayAmount;

   // Constructor. Initialize to a Do Nothing state (i.e. do not modify the FFT)
   tFftModifiers():startFreq(0.0), stopFreq(0.0), clipMax(0xFFFF), clipMin(0), logScale(false), attenLowFreqs(false), fadeAwayAmount(0){}
}tFftModifiers;


class FftModifier
{
public:
   FftModifier(float samplesRate, int fftSize, int numOutputValues, tFftModifiers& modifiers);
   virtual ~FftModifier();

   int modify(uint16_t* inOut);

private:
   // Make uncopyable
   FftModifier();
   FftModifier(FftModifier const&);
   void operator=(FftModifier const&);

   float m_freqRange;
   float m_hzPerBin;

   std::vector<int> m_indexMap;

   // Used to shift result back to 0 to 0xFFFF.
   std::vector<int32_t> m_scalar;
   int32_t m_offset;

   bool m_logScale;

   // Fade Away Parameters
   std::vector<int32_t> m_fadeAwayPeak;
   std::vector<int> m_fadeAwayCountDown;
   int m_fadeAwayAmount;
   
   void fadeAway(uint16_t* inOut, int num);

   float spliceToFreq(float splice, float range, bool isStop);
   void initIndexMap(float samplesRate, int fftSize, int numOutputValues, tFftModifiers& modifiers);
   void initScale(tFftModifiers& modifiers, int numOutputValues);

   void logScale(uint16_t* inOut, int num);

   float atten_quarterCircle(float zeroToOne);
   float atten_linear(float zeroToOne);
};

