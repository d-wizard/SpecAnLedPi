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
#include <chrono>
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
   SourceBase()
   {
      std::random_device rd;
      uint64_t ms = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
      m_randGen.seed(rd() ^ ms);
   }
   virtual ~SourceBase(){}
   virtual T getNextValue() = 0;
   SourceBase(SourceBase const&) = delete; void operator=(SourceBase const&) = delete; // delete a bunch of constructors.
protected:
   std::mt19937 m_randGen;
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class TransformBase
{
public:
   TransformBase()
   {
      std::random_device rd;
      uint64_t ms = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
      m_randGen.seed(rd() ^ ms);
   }
   virtual ~TransformBase(){}
   virtual T transform(T input) = 0;
   TransformBase(TransformBase const&) = delete; void operator=(TransformBase const&) = delete; // delete a bunch of constructors.
protected:
   std::mt19937 m_randGen;
};

////////////////////////////////////////////////////////////////////////////////
// Derived Classes - Sources
////////////////////////////////////////////////////////////////////////////////
template<class T>
class LinearSource : public SourceBase<T>
{
public:
   LinearSource(T incr, T firstVal = 0):m_nextValue(firstVal), m_incr_orig(incr), m_incr_cur(incr){}
   virtual ~LinearSource(){}
   virtual T getNextValue() override
   {
      T retVal = m_nextValue;
      m_nextValue += m_incr_cur;
      return retVal;
   }
   void setIncr(T newIncr){m_incr_orig = m_incr_cur = newIncr;}
   void scaleIncr(T scalar){m_incr_cur = m_incr_orig * scalar;}
   void negateIncr(){m_incr_cur = -m_incr_cur;}
   T getIncr(){return m_incr_cur;}
   LinearSource() = delete; LinearSource(LinearSource const&) = delete; void operator=(LinearSource const&) = delete; // delete a bunch of constructors.
private:
   T m_nextValue;
   T m_incr_orig;
   T m_incr_cur;
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
      return m_dist(SourceBase<T>::m_randGen);
   }
   RandUniformSource() = delete; RandUniformSource(RandUniformSource const&) = delete; void operator=(RandUniformSource const&) = delete; // delete a bunch of constructors.
private:
   std::uniform_real_distribution<T> m_dist;
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
      return m_dist(SourceBase<T>::m_randGen);
   }
   RandNormalSource() = delete; RandNormalSource(RandNormalSource const&) = delete; void operator=(RandNormalSource const&) = delete; // delete a bunch of constructors.
private:
   std::normal_distribution<T> m_dist;
};

////////////////////////////////////////////////////////////////////////////////
// Derived Classes - Transforms
////////////////////////////////////////////////////////////////////////////////
template<class T>
class LinearTransform : public TransformBase<T>
{
public:
   LinearTransform(T m, T b = 0):m_m_orig(m), m_m_cur(m), m_b_orig(b), m_b_cur(b) {}
   virtual ~LinearTransform(){}
   virtual T transform(T input) override
   {
      return input * m_m_cur + m_b_cur;
   }
   void setM(T newM){m_m_orig = m_m_cur = newM;}
   void setB(T newB){m_b_orig = m_b_cur = newB;}
   void scaleM(T scalar){m_m_cur = m_m_orig * scalar;}
   void scaleB(T scalar){m_b_cur = m_b_orig * scalar;}
   LinearTransform() = delete; LinearTransform(LinearTransform const&) = delete; void operator=(LinearTransform const&) = delete; // delete a bunch of constructors.
private:
   T m_m_orig;
   T m_m_cur;
   T m_b_orig;
   T m_b_cur;
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
   SineTransform(SineTransform const&) = delete; void operator=(SineTransform const&) = delete; // delete a bunch of constructors.
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
   SumTransform(SumTransform const&) = delete; void operator=(SumTransform const&) = delete; // delete a bunch of constructors.
private:
   T m_sum;
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class SawTransform : public TransformBase<T>
{
// Saw wave. Kinda like sine in terms of shape, but repeats every 1.0 (instead of 2pi). Still goes between +1.0 and -1.0 like sine.
public:
   SawTransform(){}
   virtual ~SawTransform(){}
   virtual T transform(T input) override
   {
      bool negative = (input < 0);
      T modTwo = fmod(negative ? -input : input, 2.0);
      T out = (modTwo <= 0.5) ? modTwo : 
              (modTwo <= 1.5) ? 1.0 - modTwo : 
              /* else */        modTwo - 2.0;
      return negative ? -2.0 * out : 2.0 * out; // Scale to +/- 1.0
   }
   SawTransform(SawTransform const&) = delete; void operator=(SawTransform const&) = delete; // delete a bunch of constructors.
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class RandNegateTransform : public TransformBase<T>
{
   // Randomly negates the input value (i.e. half the time the output will be -input)
public:
   RandNegateTransform(double chanceForNegateChange = 0.5):m_dist(0.0,1.0),m_chanceForNegateChange(chanceForNegateChange){}
   virtual ~RandNegateTransform(){}
   virtual T transform(T input) override
   {
      bool doNegateChange = (m_dist(TransformBase<T>::m_randGen) <= m_chanceForNegateChange);
      if(doNegateChange)
         m_prevNegateValue = !m_prevNegateValue;
      return  m_prevNegateValue ? input : -input;
   }
   RandNegateTransform(RandNegateTransform const&) = delete; void operator=(RandNegateTransform const&) = delete; // delete a bunch of constructors.
private:
   std::uniform_real_distribution<double> m_dist;
   double m_chanceForNegateChange;
   bool m_prevNegateValue = false;
};
////////////////////////////////////////////////////////////////////////////////
template<class T>
class BlockFastSignChanges : public TransformBase<T>
{
   // Prevents the sign of the output value from changing quickly.
public:
   BlockFastSignChanges(double minTimeBetweenSignChanges):m_lastSignChangeTime(std::chrono::steady_clock::now()), m_minTimeBetweenSignChanges(minTimeBetweenSignChanges){}
   virtual ~BlockFastSignChanges(){}
   virtual T transform(T inOut) override
   {
      bool inputIsPositive = (inOut > 0.0);
      auto nowTime = std::chrono::steady_clock::now();
      int64_t timeBetweenNs = std::chrono::duration_cast<std::chrono::nanoseconds>(nowTime - m_lastSignChangeTime).count();
      double timeBetween = double(timeBetweenNs) * double(1e-9);
      if(timeBetween >= m_minTimeBetweenSignChanges)
      {
         m_lastSignChangeTime = nowTime;
         m_lastSignPositive = inputIsPositive;
      }
      else
      {
         // Not enough time has passed. Don't allow a sign change.
         if(m_lastSignPositive != inputIsPositive)
            inOut = -inOut;
      }
      return inOut;
   }
   BlockFastSignChanges(BlockFastSignChanges const&) = delete; void operator=(BlockFastSignChanges const&) = delete; // delete a bunch of constructors.
private:
   std::chrono::steady_clock::time_point m_lastSignChangeTime;
   const double m_minTimeBetweenSignChanges;
   bool m_lastSignPositive = true;
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
   Generator(SourcePtr<T> source, const std::vector<TransformPtr<T>>& transforms):m_source(source), m_transforms(transforms){}; // Multiple transforms.

   T getNext()
   {
      T val = m_source->getNextValue();
      for(auto& transform : m_transforms)
         val = transform->transform(val);
      m_lastVal = val;
      return m_lastVal;
   }
   
   T getNextDelta()
   {
      T last = m_lastVal;
      return getNext() - last;
   }

   T getLast(){return m_lastVal;}

   Generator() = delete; Generator(Generator const&) = delete; void operator=(Generator const&) = delete; // delete a bunch of constructors.
private:
   SourcePtr<T> m_source;
   std::vector<TransformPtr<T>> m_transforms;
   T m_lastVal = 0;
};
}
