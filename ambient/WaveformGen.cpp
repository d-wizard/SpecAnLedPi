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
#include <math.h>
#include "WaveformGen.h"
#include "Transform1D.h"


template<class T>
WaveformGen<T>::WaveformGen(unsigned numPoints):
   m_points(numPoints)
{

}

template<class T>
WaveformGen<T>::~WaveformGen()
{

}

template<class T>
void WaveformGen<T>::Sinc(T startPhase, T endPhase)
{
   size_t numPoints = m_points.size();
   for(size_t i = 0; i < numPoints; ++i)
   {
      auto phase = T(i) * (endPhase - startPhase) / T(numPoints-1) + startPhase;
      m_points[i] = (phase == 0.0) ? 1.0 : sin(phase) / phase;
   }
}

template<class T>
void WaveformGen<T>::Linear(T startVal, T endVal)
{
   size_t numPoints = m_points.size();
   for(size_t i = 0; i < numPoints; ++i)
   {
      m_points[i] = T(i) * (endVal - startVal) / T(numPoints-1) + startVal;
   }
}

template<class T>
void WaveformGen<T>::absoluteValue()
{
   size_t numPoints = m_points.size();
   for(size_t i = 0; i < numPoints; ++i)
   {
      m_points[i] = abs(m_points[i]);
   }
}

template<class T>
void WaveformGen<T>::scale(T scalar)
{
   size_t numPoints = m_points.size();
   for(size_t i = 0; i < numPoints; ++i)
   {
      m_points[i] *= scalar;
   }
}

template<class T>
void WaveformGen<T>::shift(T shiftVal)
{
   size_t numPoints = m_points.size();
   for(size_t i = 0; i < numPoints; ++i)
   {
      m_points[i] += shiftVal;
   }
}

template<class T>
void WaveformGen<T>::quarterCircle_above()
{
   size_t numPoints = m_points.size();
   for(size_t i = 0; i < numPoints; ++i)
   {
      m_points[i] = Transform1D::Unit::quarterCircle_above(m_points[i]);
   }
}

template<class T>
void WaveformGen<T>::quarterCircle_below()
{
   size_t numPoints = m_points.size();
   for(size_t i = 0; i < numPoints; ++i)
   {
      m_points[i] = Transform1D::Unit::quarterCircle_below(m_points[i]);
   }
}