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

template<class T>
class AmbientMovement
{
public:
   // Types
   typedef enum
   {
      E_AMB_MOVE_TYPE__SIN,
      E_AMB_MOVE_TYPE__LINEAR
   }eAmbientMoveTransform;

   typedef enum
   {
      E_AMB_MOVE_SRC__FIXED,
      E_AMB_MOVE_SRC__RANDOM
   }eAmbientMoveSource;

   typedef enum
   {
      E_AMB_MOVE_RAND_DIST__UNIFORM,
      E_AMB_MOVE_RAND_DIST__NORMAL
   }eAmbientMoveRandType;

   typedef struct tAmbientMoveProps
   {
      tAmbientMoveProps(): transform(E_AMB_MOVE_TYPE__LINEAR), source(E_AMB_MOVE_SRC__FIXED), randType(E_AMB_MOVE_RAND_DIST__UNIFORM), fixed_incr(0), rand_paramA(0), rand_paramB(0){}

      eAmbientMoveTransform transform;
      eAmbientMoveSource    source;
      eAmbientMoveRandType  randType;

      // Value used by Fixed Movement sources.
      T fixed_incr;

      // Values used to define random distributions.
      T rand_paramA; // example: lower bound for uniform dist, mean for normal dist.
      T rand_paramB; // example: upper bound for uniform dist, standard deviation for normal dist.
   }tAmbientMoveProps;

public:
   AmbientMovement(const tAmbientMoveProps& prop, T outputScalar = 1.0);
   virtual ~AmbientMovement();

   // Make uncopyable
   AmbientMovement();
   AmbientMovement(AmbientMovement const&);
   void operator=(AmbientMovement const&);

   T move(); // returns the movement change. Use the 'get' function for total movement.

   T get(){return m_postTransformMovement*m_outputScalar_current;}

   void scaleOutputScalar(tAmbientMoveProps& props);
   void scaleMovementScalar(tAmbientMoveProps& props);

private:
   tAmbientMoveProps m_moveProps;

   T m_outputScalar_orig = 1.0;
   T m_outputScalar_current = 1.0;

   T m_moveScalar_orig = 1.0;
   T m_moveScalar_current = 1.0;

   T m_preTransformMovement = 0.0;
   T m_postTransformMovement = 0.0;

   std::default_random_engine m_randGen;

   T getDeltaVal(tAmbientMoveProps& prop);
   T transform(tAmbientMoveProps& prop, T valToTransform);
};

typedef AmbientMovement<float> AmbientMovementF;
typedef AmbientMovement<double> AmbientMovementD;

#include "AmbientMovement.cpp"
