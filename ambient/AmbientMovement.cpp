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
   m_scalar(scalar)
{

}

template<class T>
AmbientMovement<T>::~AmbientMovement()
{

}

template<class T>
T AmbientMovement<T>::move()
{
   /////////////////////////////////////////////////////////////////////////////
   // Determine the update value.
   /////////////////////////////////////////////////////////////////////////////
   T preTransformUpdateVal = 0.0;
   if(m_moveProps.source == E_AMB_MOVE_SRC__FIXED)
   {
      preTransformUpdateVal = m_moveProps.fixed_incr;
   }
   else if(m_moveProps.source == E_AMB_MOVE_SRC__RANDOM)
   {
      switch(m_moveProps.randType)
      {
         case E_AMB_MOVE_RAND_DIST__UNIFORM:
         {
            std::uniform_real_distribution<T> dist(m_moveProps.rand_paramA, m_moveProps.rand_paramB);
            preTransformUpdateVal = dist(m_randGen);
         }
         break;
         case E_AMB_MOVE_RAND_DIST__NORMAL:
         {
            std::normal_distribution<T> dist(m_moveProps.rand_paramA, m_moveProps.rand_paramB);
            preTransformUpdateVal = dist(m_randGen);
         }
         break;
         default:
            // Do nothing. Keep preTransformUpdateVal at 0.0
         break;
      }
   }
   // else: Do nothing. Keep preTransformUpdateVal at 0.0

   /////////////////////////////////////////////////////////////////////////////
   // Transform the value.
   /////////////////////////////////////////////////////////////////////////////
   T orig_postTransformMovement = m_postTransformMovement; // Store off the before value, so we can return the delta.
   m_preTransformMovement += preTransformUpdateVal; // Update the pre-transform value.

   switch(m_moveProps.transform)
   {
      // Fall through is intended. Make default the same as the linear case.
      default:
      case E_AMB_MOVE_TYPE__LINEAR:
      {
         m_postTransformMovement = m_preTransformMovement;
      }
      break;
      case E_AMB_MOVE_TYPE__SIN:
      {
         m_postTransformMovement = sin(m_preTransformMovement);
      }
      break;
   }

   return (m_postTransformMovement - orig_postTransformMovement)*m_scalar; // Return the change (use the 'get' function to get the full value)
}
