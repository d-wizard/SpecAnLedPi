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
#include <random>
#include <math.h>
#include <vector>
#include <memory>

namespace AmbientMovement
{
////////////////////////////////////////////////////////////////////////////////
// Base Classes
////////////////////////////////////////////////////////////////////////////////
template<class T>
class SourceBase
{
public:
   SourceBase(){}
   virtual ~SourceBase(){}
   virtual T getNextValue() = 0;
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class TransformBase
{
public:
   TransformBase(){}
   virtual ~TransformBase(){}
   virtual T transform(T input) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// Derived Classes - Sources
////////////////////////////////////////////////////////////////////////////////
template<class T>
class LinearSource : public SourceBase<T>
{
public:
   LinearSource(T incr, T firstVal = 0):m_nextValue(firstVal), m_incr(incr) {}
   virtual ~LinearSource(){}
   virtual T getNextValue() override
   {
      T retVal = m_nextValue;
      m_nextValue += m_incr;
      return retVal;
   }
private:
   T m_nextValue;
   T m_incr;
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class RandUniformSource : public SourceBase<T>
{
public:
   RandUniformSource(T minVal, T maxVal):m_dist(minVal, maxVal) {}
   virtual ~RandUniformSource(){}
   virtual T getNextValue() override
   {
      return m_dist(m_randGen);
   }
private:
   std::uniform_real_distribution<T> m_dist;
   std::default_random_engine m_randGen;
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class RandNormalSource : public SourceBase<T>
{
public:
   RandNormalSource(T mean, T standardDeviation):m_dist(mean, standardDeviation) {}
   virtual ~RandNormalSource(){}
   virtual T getNextValue() override
   {
      return m_dist(m_randGen);
   }
   RandNormalSource() = 0; RandNormalSource(RandNormalSource const&) = 0; void operator=(RandNormalSource const&) = 0; // delete a bunch of constructors.
private:
   std::normal_distribution<T> m_dist;
   std::default_random_engine m_randGen;
};

////////////////////////////////////////////////////////////////////////////////
// Derived Classes - Transforms
////////////////////////////////////////////////////////////////////////////////
template<class T>
class LinearTransform : public TransformBase<T>
{
public:
   LinearTransform(T m, T b = 0):m_m(m), m_b(b) {}
   virtual ~LinearTransform(){}
   virtual T transform(T input) override
   {
      return input * m_m + m_b;
   }
private:
   T m_m;
   T m_b;
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class SineTransform : public TransformBase<T>
{
public:
   SineTransform(){}
   virtual ~SineTransform(){}
   virtual T transform(T input) override
   {
      return sin(input);
   }
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class SumTransform : public TransformBase<T>
{
public:
   SumTransform(T sumStart = 0):m_sum(sumStart){}
   virtual ~SumTransform(){}
   virtual T transform(T input) override
   {
      m_sum += input;
      return sin(m_sum);
   }
private:
   T m_sum;
};

////////////////////////////////////////////////////////////////////////////////
// Final Class
////////////////////////////////////////////////////////////////////////////////
template<class T>
using SourcePtr = std::shared_ptr<SourceBase<T>>;
template<class T>
using TransformPtr = std::shared_ptr<TransformBase<T>>;

template<class T>
class Generator
{
public:
   Generator(SourcePtr<T> source):m_source(source){} // No transforms.
   Generator(SourcePtr<T> source, TransformPtr<T> transform):m_source(source){m_transforms.push_back(transform);} // Single transforms.
   Generator(SourcePtr<T> source, std::vector<TransformPtr<T>>& transforms):m_source(source), m_transforms(transforms){}; // Multiple transforms.

   T getRaw()
   {
      T val = m_source->getNextValue();
      for(auto& transform : m_transforms)
         val = transform->transform(val);
      m_lastVal = val;
      return m_lastVal;
   }
   
   T getDelta()
   {
      T last = m_lastVal;
      return getRaw() - last;
   }

private:
   SourcePtr<T> m_source;
   std::vector<TransformPtr<T>> m_transforms;
   T m_lastVal = 0;
};
}
