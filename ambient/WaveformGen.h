/* Copyright 2023 Dan Williams. All Rights Reserved.
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

template<class T>
class WaveformGen
{
public:
   WaveformGen(unsigned numPoints);
   virtual ~WaveformGen();

   // Make uncopyable
   WaveformGen();
   WaveformGen(WaveformGen const&);
   void operator=(WaveformGen const&);

   std::vector<T>& getPoints(){return m_points;}

   // Waveforms
   void Sinc(T startPhase, T endPhase);
   void Linear(T startVal, T endVal);

   // Standard Math Manipulations
   void absoluteValue();
   void scale(T scalar);
   void shift(T shiftVal);

   // Quarter Circle Transforms (only really valid for values between 0 and 1)
   void quarterCircle_above();
   void quarterCircle_below();

private:
   std::vector<T> m_points;
};

#include "WaveformGen.cpp"