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
#include "AmbientMovement.h"

template<class T>
AmbientMovement<T>::AmbientMovement(const AmbientMovement::tAmbientMoveProps& prop, T scalar):
   m_moveProps(prop),
   m_outputScalar_orig(scalar),
   m_outputScalar_current(scalar)
{

}

template<class T>
AmbientMovement<T>::~AmbientMovement()
{

}


template<class T>
T AmbientMovement<T>::getDeltaVal(tAmbientMoveProps& prop)
{
   T preTransformUpdateVal = 0.0;
   if(prop.source == E_AMB_MOVE_SRC__FIXED)
   {
      preTransformUpdateVal = prop.fixed_incr;
   }
   else if(prop.source == E_AMB_MOVE_SRC__RANDOM)
   {
      switch(prop.randType)
      {
         case E_AMB_MOVE_RAND_DIST__UNIFORM:
         {
            std::uniform_real_distribution<T> dist(prop.rand_paramA, prop.rand_paramB);
            preTransformUpdateVal = dist(m_randGen);
         }
         break;
         case E_AMB_MOVE_RAND_DIST__NORMAL:
         {
            std::normal_distribution<T> dist(prop.rand_paramA, prop.rand_paramB);
            preTransformUpdateVal = dist(m_randGen);
         }
         break;
         default:
            // Do nothing. Keep preTransformUpdateVal at 0.0
         break;
      }
   }
   // else: Do nothing. Keep preTransformUpdateVal at 0.0
   return preTransformUpdateVal;
}

template<class T>
T AmbientMovement<T>::transform(tAmbientMoveProps& prop, T valToTransform)
{
   T retVal = 0.0;
   switch(prop.transform)
   {
      // Fall through is intended. Make default the same as the linear case.
      default:
      case E_AMB_MOVE_TYPE__LINEAR:
      {
         retVal = valToTransform;
      }
      break;
      case E_AMB_MOVE_TYPE__SIN:
      {
         retVal = sin(valToTransform);
      }
      break;
   }
   return retVal;
}

template<class T>
T AmbientMovement<T>::move()
{
   /////////////////////////////////////////////////////////////////////////////
   // Determine the update value.
   /////////////////////////////////////////////////////////////////////////////
   T preTransformUpdateVal = getDeltaVal(m_moveProps)*m_moveScalar_current;

   /////////////////////////////////////////////////////////////////////////////
   // Transform the value.
   /////////////////////////////////////////////////////////////////////////////
   T orig_postTransformMovement = m_postTransformMovement; // Store off the before value, so we can return the delta.
   m_preTransformMovement += preTransformUpdateVal; // Update the pre-transform value.

   m_postTransformMovement = transform(m_moveProps, m_preTransformMovement);

   return (m_postTransformMovement - orig_postTransformMovement)*m_outputScalar_current; // Return the change (use the 'get' function to get the full value)
}

template<class T>
void AmbientMovement<T>::scaleOutputScalar(tAmbientMoveProps& props)
{
   m_outputScalar_current = m_outputScalar_orig * transform(props, getDeltaVal(props));
}

template<class T>
void AmbientMovement<T>::scaleMovementScalar(tAmbientMoveProps& props)
{
   m_moveScalar_current = m_moveScalar_orig * transform(props, getDeltaVal(props));
}

