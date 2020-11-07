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
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "colorGradient.h"


ColorGradient::ColorGradient(size_t numPoints)
{
   init(numPoints);
}

ColorGradient::ColorGradient(std::vector<ColorGradient::tGradientPoint>& points)
{
   init(points.size());
   for(size_t i = 0; i < m_gradPoints.size(); ++i)
   {
      m_gradPoints[i].hue = points[i].hue;
      m_gradPoints[i].saturation = points[i].saturation;
   }
}

ColorGradient::~ColorGradient()
{

}

void ColorGradient::init(size_t numPoints)
{
   m_numZones = numPoints * 2 - 1;
   float reach = 0.5f / (float)m_numZones;
   float deltaBetweenPoints = 1.0f / (float)(numPoints-1);

   m_gradPoints.resize(numPoints);
   for(size_t i = 0; i < m_gradPoints.size(); ++i)
   {
      bool first = (i == 0);
      bool last  = (i == (m_gradPoints.size()-1));

      m_gradPoints[i].hue = 0.0f + deltaBetweenPoints * i;
      m_gradPoints[i].saturation = 1.0;
      m_gradPoints[i].lightness = 1.0;
      m_gradPoints[i].position = 0.0f + deltaBetweenPoints * i;
      m_gradPoints[i].reach = (first || last) ? reach * 2.0f : reach;
      assert(m_gradPoints[i].reach >= MIN_INCREMENT);
   }
}

std::vector<ColorGradient::tGradientPoint> ColorGradient::getGradient()
{
   return m_gradPoints;
}

ColorGradient::tGradientPoint ColorGradient::getGradientPoint(int pointIndex)
{
   if(pointIndex >= 0 && pointIndex < (int)m_gradPoints.size())
   {
      return m_gradPoints[pointIndex];
   }
   tGradientPoint retVal;
   return retVal;
}

void ColorGradient::updateGradient(ColorGradient::eGradientOptions option, float value, int pointIndex)
{
   if(pointIndex >= 0 && pointIndex < (int)m_gradPoints.size())
   {
      storePrevSettings(option, pointIndex);

      switch(option)
      {
         case E_GRAD_HUE:
            setHue(value, pointIndex);
         break;
         case E_GRAD_SATURATION:
            setSat(value, pointIndex);
         break;
         case E_GRAD_LIGHTNESS:
            setLight(value, pointIndex);
         break;
         case E_GRAD_POSITION:
            setPos(value, pointIndex);
         break;
         case E_GRAD_REACH:
            setReach(value, pointIndex);
         break;
         default:
            assert(0);
         break;
      }
   }
   else
   {
      assert(0);
   }
}

void ColorGradient::updateGradientDelta(eGradientOptions option, float delta, int pointIndex)
{
   float newValue;
   switch(option)
   {
      case E_GRAD_HUE:
         newValue = fmod(m_gradPoints[pointIndex].hue+delta, 1.0); // Hue wraps around.
         if(newValue < 0)
         {
            newValue = 1.0 + newValue; // fmod mirrors around 0. So need to make negative numbers positive.
         }
      break;
      case E_GRAD_SATURATION:
         newValue = m_gradPoints[pointIndex].saturation + delta;
      break;
      case E_GRAD_LIGHTNESS:
         newValue = m_gradPoints[pointIndex].lightness + delta;
      break;
      case E_GRAD_POSITION:
         newValue = m_gradPoints[pointIndex].position + delta;
      break;
      case E_GRAD_REACH:
         newValue = m_gradPoints[pointIndex].reach + delta;
      break;
      default:
         assert(0);
      break;
   }
   if(newValue < 0.0)
      newValue = 0.0;
   else if(newValue > 1.0)
      newValue = 1.0;
   updateGradient(option, newValue, pointIndex);
}

void ColorGradient::setHue(float value, size_t pointIndex)
{
   // Just update hue.
   m_gradPoints[pointIndex].hue = value;
}

void ColorGradient::setSat(float value, size_t pointIndex)
{
   // Just update saturation.
   m_gradPoints[pointIndex].saturation = value;
}

void ColorGradient::setLight(float value, size_t pointIndex)
{
   // Just update saturation.
   m_gradPoints[pointIndex].lightness = value;
}

void ColorGradient::setPos(float value, size_t pointIndex)
{
   if(pointIndex > 0 && pointIndex < (m_gradPoints.size()-1))
   {
      float valueToUse = value;
      float reach = m_gradPoints[pointIndex].reach;

      float loLimit = getLoLimit(pointIndex);
      float hiLimit = getHiLimit(pointIndex);

      bool loValid = false;
      bool hiValid = false;

      while(!loValid || !hiValid)
      {
         loValid = hiValid = true;
         float edgeLoNew = valueToUse - reach;
         if(edgeLoNew < loLimit)
         {
            valueToUse = loLimit + reach;
            
            // Double check new low edge (floating point rounding errors can screw things up).
            float edgeLoDoubleCheck = valueToUse - reach;
            if(edgeLoDoubleCheck < loLimit)
            {
               // Yep, edgeLoDoubleCheck should equal loLimit, but it doesn't. Keep adding a little bit to the position until the low edge is good.
               while(edgeLoDoubleCheck < loLimit)
               {
                  valueToUse += (MIN_INCREMENT / 128.0);
                  edgeLoDoubleCheck = valueToUse - reach;
               }
            }
            loValid = false;
         }

         float edgeHiNew = valueToUse + reach;
         if(edgeHiNew > hiLimit)
         {
            valueToUse = hiLimit - reach;
            hiValid = false;
         }
      }

      // Actually update the position of this point.
      m_gradPoints[pointIndex].position = valueToUse;
      locationChanged(pointIndex);
   }
}

void ColorGradient::setReach(float value, size_t pointIndex)
{
   bool first = (pointIndex == 0);
   bool last  = (pointIndex == (m_gradPoints.size()-1));

   auto maxReach = (getHiLimit(pointIndex) - getLoLimit(pointIndex));
   if(!first && !last) 
      maxReach /= 2.0;

   if(value < MIN_INCREMENT)
      value = MIN_INCREMENT;
   else if(value > maxReach)
      value = maxReach;

   if(pointIndex >= 0 && pointIndex <= (m_gradPoints.size()-1))
   {
      float valueToUse = value;
      float position = m_gradPoints[pointIndex].position;

      float loLimit = getLoLimit(pointIndex);
      float hiLimit = getHiLimit(pointIndex);

      if( (position < hiLimit || last) && (position > loLimit || first) )
      {
         bool loValid = false;
         bool hiValid = false;

         while(!loValid || !hiValid)
         {
            loValid = hiValid = true;
            float edgeLoNew = position - valueToUse;
            if(edgeLoNew < loLimit && !first)
            {
               valueToUse = position - loLimit;
               assert(valueToUse > 0);
               loValid = false;
            }

            float edgeHiNew = position + valueToUse;
            if(edgeHiNew > hiLimit && !last)
            {
               valueToUse = hiLimit - position;
               assert(valueToUse > 0);
               hiValid = false;
            }
         }

         // Actually update the position of this point.
         m_gradPoints[pointIndex].reach = valueToUse;
         locationChanged(pointIndex);
      }

   }
}

bool ColorGradient::canAddPoint()
{
   return getLoLimit(m_gradPoints.size()*3) < 1.0; // If 1.0 or greater, there is no room for another point.
}

void ColorGradient::addPoint(int pointIndexToDuplicate)
{
   bool validIndex = (pointIndexToDuplicate >= 0 && pointIndexToDuplicate < (int)m_gradPoints.size());
   if(validIndex && canAddPoint())
   {
      auto duplicateIter = m_gradPoints.begin();
      for(int i = 0; i < pointIndexToDuplicate; ++i)
         duplicateIter++;
      auto newPoint = m_gradPoints[pointIndexToDuplicate];
      newPoint.reach = 0.0; // Set to zero and let fixSpacing() deal with it.

      m_gradPoints.insert(duplicateIter, newPoint);
      fixSpacing();
      m_previousIndex = -1; // Just made a major change to the vector, make sure the previous version that is store off is not used again.
   }
}

bool ColorGradient::canRemovePoint()
{
   return m_gradPoints.size() > 2;
}

bool ColorGradient::removePoint(int pointIndexToRemove)
{
   bool pointWasActualyRemoved = false;
   bool validIndex = (pointIndexToRemove >= 0 && pointIndexToRemove < (signed)m_gradPoints.size());
   if(validIndex && canRemovePoint())
   {
      bool first = (pointIndexToRemove == 0);
      bool last  = (pointIndexToRemove == ((signed)m_gradPoints.size()-1));
      
      auto toRemoveIter = m_gradPoints.begin();
      for(int i = 0; i < pointIndexToRemove; ++i)
         toRemoveIter++;

      m_gradPoints.erase(toRemoveIter);
      pointWasActualyRemoved = true;

      if(first)
      {
         // Removed the first value. Make sure new first has the correct position.
         m_gradPoints[0].position = 0.0;
      }
      else if(last)
      {
         // Removed the last value. Make sure new last has the correct position.
         m_gradPoints[m_gradPoints.size()-1].position = 1.0;
      }

      fixSpacing();
      m_previousIndex = -1; // Just made a major change to the vector, make sure the previous version that is store off is not used again.
   }
   return pointWasActualyRemoved;
}

float ColorGradient::getLoLimit(size_t pointIndex)
{
   // There are 3 ranges between points. So multiply point index by 3.
   return MIN_INCREMENT * pointIndex * 3;
}

float ColorGradient::getHiLimit(size_t pointIndex)
{
   return FULL_SCALE - getLoLimit(m_gradPoints.size()-1-pointIndex);
}

void ColorGradient::storePrevSettings(eGradientOptions option, int pointIndex)
{
   bool changed = (option != m_previousOption || pointIndex != m_previousIndex);
   bool newOptionIsLocation = (option == E_GRAD_POSITION || option == E_GRAD_REACH);
   if(changed && newOptionIsLocation)
   {
      m_previousOption = option;
      m_previousIndex = pointIndex;
      m_previousGradPointsLo = m_previousGradPointsHi = m_gradPoints;
   }
}

void ColorGradient::locationChanged(size_t pointIndex)
{
   bool first = (pointIndex == 0);
   bool last  = (pointIndex == (m_gradPoints.size()-1));

   bool doLoSide = !first;
   bool doHiSide = !last;

   if(doLoSide)
   {
      // Low
      float newScalePosLo = m_gradPoints[pointIndex].position - m_gradPoints[pointIndex].reach;
      float oldScalePosLo = m_previousGradPointsLo[pointIndex].position - m_previousGradPointsLo[pointIndex].reach;
      float lowerPointHi  = m_previousGradPointsLo[pointIndex-1].position + m_previousGradPointsLo[pointIndex-1].reach;
      float ratioLo = newScalePosLo / oldScalePosLo;

      bool newOverlap = newScalePosLo < lowerPointHi;

      if(!newOverlap)
      {
         // No overlap, just restore the original values.
         ratioLo = 1.0;
         m_previousGradPointsLo[pointIndex] = m_gradPoints[pointIndex]; // Make sure this value is used if scaling needs to be applied.
      }

      for(size_t i = 0; i < pointIndex; ++i)
      {
         m_gradPoints[i].position = m_previousGradPointsLo[i].position * ratioLo;
         m_gradPoints[i].reach    = m_previousGradPointsLo[i].reach    * ratioLo;
      }

   }

   if(doHiSide)
   {
      // High (similar to low side but reflect from +1.0)
      float newScalePosHi = m_gradPoints[pointIndex].position + m_gradPoints[pointIndex].reach;
      float oldScalePosHi = m_previousGradPointsHi[pointIndex].position + m_previousGradPointsHi[pointIndex].reach;
      float upperPointLo  = m_previousGradPointsHi[pointIndex+1].position - m_previousGradPointsHi[pointIndex+1].reach;
      float ratioHi = (FULL_SCALE - newScalePosHi) / (FULL_SCALE - oldScalePosHi);

      bool newOverlap = upperPointLo < newScalePosHi;

      if(!newOverlap)
      {
         // No overlap, just restore the original values.
         ratioHi = 1.0;
         m_previousGradPointsHi[pointIndex] = m_gradPoints[pointIndex]; // Make sure this value is used if scaling needs to be applied.
      }

      for(size_t i = pointIndex+1; i < m_gradPoints.size(); ++i)
      {
         float reflectPos = FULL_SCALE - m_previousGradPointsHi[i].position;
         m_gradPoints[i].position = FULL_SCALE - (reflectPos * ratioHi);
         m_gradPoints[i].reach    = m_previousGradPointsHi[i].reach * ratioHi;
      }
   }

}

void ColorGradient::fixSpacing()
{
   bool done = false;
   int loopCount = 0;
   while(done == false && loopCount < 10)
   {
      bool goodUp = fixSpacing(true);
      bool goodDn = fixSpacing(false);
      done = goodUp && goodDn;
      ++loopCount;
   }
   assert(fixSpacing(true));
   assert(fixSpacing(false));
}

bool ColorGradient::fixSpacing(bool upDirection)
{
   bool goodPass = true;

   if(upDirection)
   {
      // Moving up the Gradient Points (Only move the starts / ends of point forward)
      for(size_t i = 0; i < (m_gradPoints.size()-1); ++i) // Handle the last point outside of this loop.
      {
         bool thisIsFirst = (i == 0);
         bool nextIsLast  = (i == (m_gradPoints.size()-2));

         if(m_gradPoints[i].reach < MIN_INCREMENT)
         {
            goodPass = false;

            auto oldReach = m_gradPoints[i].reach;
            m_gradPoints[i].reach = MIN_INCREMENT;
            if(!thisIsFirst)
            {
               m_gradPoints[i].position += (m_gradPoints[i].reach - oldReach); // Move position by the change in reach. This way the start point remains the same (i.e. don't move anything backward).
            }
         }

         if(!thisIsFirst)
         {
            auto minStart = getLoLimit(i);
            auto thisStart = m_gradPoints[i].position - m_gradPoints[i].reach;
            if(thisStart < minStart)
            {
               // Need to move the start forward.
               auto moveStartAmount = minStart - thisStart;
               m_gradPoints[i].position += moveStartAmount;
               goodPass = false;

               auto thisStart2 = m_gradPoints[i].position - m_gradPoints[i].reach;
               if(thisStart2 < minStart)
               {
                  m_gradPoints[i].position += (MIN_INCREMENT / 128.0);
               }
            }
         }

         auto thisEnd   = m_gradPoints[i  ].position + m_gradPoints[i  ].reach;
         auto nextStart = m_gradPoints[i+1].position - m_gradPoints[i+1].reach;
         auto nextEnd   = m_gradPoints[i+1].position + m_gradPoints[i+1].reach;

         if( (nextStart - thisEnd) < MIN_INCREMENT)
         {
            goodPass = false;
            // Not enough gap between this point and the next point. Move the next point.
            auto moveStartAmount = MIN_INCREMENT - (nextStart - thisEnd);
            if(nextIsLast)
            {
               // Just change the reach. The position cannot be moved.
               m_gradPoints[i+1].reach -= moveStartAmount;
            }
            else
            {
               // Move the reach and the position such that only the start position moves.
               m_gradPoints[i+1].reach -= (moveStartAmount/2.0);
               m_gradPoints[i+1].position = nextEnd - m_gradPoints[i+1].reach;
            }
         }
      } // End for

      // The last point's reach wasn't checked in the for loop.
      if(m_gradPoints[m_gradPoints.size()-1].reach < MIN_INCREMENT)
      {
         goodPass = false; // Do not change here (only change values start/end point forward here). Will be fixed next time through the loop.
      }
   }
   else
   {
      // Moving down the Gradient Points (Only move the starts / ends of point backward)
      for(size_t i = m_gradPoints.size()-1; i > 0; --i) // Handle the first point outside of this loop.
      {
         bool thisIsLast  = (i == (m_gradPoints.size()-1));
         bool prevIsFirst = (i == 1);

         if(m_gradPoints[i].reach < MIN_INCREMENT)
         {
            goodPass = false;

            auto oldReach = m_gradPoints[i].reach;
            m_gradPoints[i].reach = MIN_INCREMENT;
            if(!thisIsLast)
            {
               m_gradPoints[i].position -= (m_gradPoints[i].reach - oldReach); // Move position by the change in reach. This way the end point remains the same (i.e. don't move anything forward).
            }
         }

         if(!thisIsLast)
         {
            auto maxEnd = getHiLimit(i);
            auto thisEnd = m_gradPoints[i].position + m_gradPoints[i].reach;
            if(maxEnd < thisEnd)
            {
               // Need to move the end backward.
               auto moveStartAmount = thisEnd - maxEnd;
               m_gradPoints[i].position -= moveStartAmount;
               goodPass = false;
            }
         }

         auto thisStart = m_gradPoints[i  ].position - m_gradPoints[i  ].reach;
         auto prevStart = m_gradPoints[i-1].position - m_gradPoints[i-1].reach;
         auto prevEnd   = m_gradPoints[i-1].position + m_gradPoints[i-1].reach;

         if( (thisStart - prevEnd) < MIN_INCREMENT)
         {
            goodPass = false;
            // Not enough gap between this point and the next point. Move the next point.
            auto moveEndAmount = MIN_INCREMENT - (thisStart - prevEnd);
            if(prevIsFirst)
            {
               // Just change the reach. The position cannot be moved.
               m_gradPoints[i-1].reach -= moveEndAmount;
            }
            else
            {
               // Move the reach and the position such that only the end position moves.
               m_gradPoints[i-1].reach -= (moveEndAmount/2.0);
               m_gradPoints[i-1].position = prevStart + m_gradPoints[i-1].reach;
            }
         }
      } // End for

      // The last point's reach wasn't checked in the for loop.
      if(m_gradPoints[0].reach < MIN_INCREMENT)
      {
         goodPass = false; // Do not change here (only change values start/end point backward here). Will be fixed next time through the loop.
      }
   }
   return goodPass;
}


