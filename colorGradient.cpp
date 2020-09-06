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

#include <assert.h>
#include "colorGradient.h"


ColorGradient::ColorGradient(size_t numPoints)
{
   size_t numZones = numPoints * 2 - 1;
   float reach = 0.5f / (float)numZones;
   float deltaBetweenPoints = 1.0f / (float)(numPoints-1);

   m_gradPoints.resize(numPoints);
   for(size_t i = 0; i < m_gradPoints.size(); ++i)
   {
      bool first = (i == 0);
      bool last  = (i == (m_gradPoints.size()-1));

      m_gradPoints[i].hue = 0;
      m_gradPoints[i].saturation = 0;
      m_gradPoints[i].lightness = 0;
      m_gradPoints[i].position = 0.0f + deltaBetweenPoints * i;
      m_gradPoints[i].reach = (first || last) ? reach * 2.0f : reach;
   }
}

ColorGradient::~ColorGradient()
{

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
   if(pointIndex >= 0 && pointIndex <= (m_gradPoints.size()-1))
   {
      float valueToUse = value;
      float position = m_gradPoints[pointIndex].position;

      float loLimit = getLoLimit(pointIndex);
      float hiLimit = getHiLimit(pointIndex);

      bool loValid = false;
      bool hiValid = false;

      while(!loValid || !hiValid)
      {
         loValid = hiValid = true;
         float edgeLoNew = position - valueToUse;
         if(edgeLoNew < loLimit)
         {
            valueToUse = position - loLimit;
            loValid = false;
         }

         float edgeHiNew = position + valueToUse;
         if(edgeHiNew > hiLimit)
         {
            valueToUse = hiLimit - position;
            hiValid = false;
         }
      }

      // Actually update the position of this point.
      m_gradPoints[pointIndex].reach = valueToUse;
      locationChanged(pointIndex);
   }
}

float ColorGradient::getLoLimit(size_t pointIndex)
{
   return MIN_INCREMENT * pointIndex;
}

float ColorGradient::getHiLimit(size_t pointIndex)
{
   return FULL_SCALE - MIN_INCREMENT * (m_gradPoints.size()-1-pointIndex);
}

void ColorGradient::storePrevSettings(eGradientOptions option, int pointIndex)
{
   bool changed = (option != m_previousOption || pointIndex != m_previousIndex);
   bool newOptionIsLocation = (option == E_GRAD_POSITION || option == E_GRAD_REACH);
   if(changed && newOptionIsLocation)
   {
      m_previousOption = option;
      m_previousIndex = pointIndex;
      m_previousGradPoints = m_gradPoints;
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
      float oldScalePosLo = m_previousGradPoints[pointIndex].position - m_previousGradPoints[pointIndex].reach;
      float ratioLo = newScalePosLo / oldScalePosLo;
      for(size_t i = 0; i < pointIndex; ++i)
      {
         m_gradPoints[i].position = m_previousGradPoints[i].position * ratioLo;
         m_gradPoints[i].reach    = m_previousGradPoints[i].reach    * ratioLo;
      }

   }

   if(doHiSide)
   {
      // High (similar to low side but reflect from +1.0)
      float newScalePosHi = m_gradPoints[pointIndex].position + m_gradPoints[pointIndex].reach;
      float oldScalePosHi = m_previousGradPoints[pointIndex].position + m_previousGradPoints[pointIndex].reach;
      float ratioHi = (FULL_SCALE - newScalePosHi) / (FULL_SCALE - oldScalePosHi);
      for(size_t i = pointIndex+1; i < m_gradPoints.size(); ++i)
      {
         float reflectPos = FULL_SCALE - m_previousGradPoints[i].position;
         m_gradPoints[i].position = FULL_SCALE - (reflectPos * ratioHi);
         m_gradPoints[i].reach    = m_previousGradPoints[i].reach * ratioHi;
      }
   }

}
