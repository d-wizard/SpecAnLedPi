/* Copyright 2020, 2023 Dan Williams. All Rights Reserved.
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
#include "gradientToScale.h"
#include "hsvrgb.h"

namespace Convert
{

static HsvColor gradientToHsv(ColorGradient::tGradientPoint grad)
{
   HsvColor hsv;
   hsv.h = (int)(grad.hue*255.0) & 0xFF;
   hsv.s = (int)(grad.saturation*255.0) & 0xFF;
   hsv.v = (int)(grad.lightness*255.0) & 0xFF;
   return hsv;
}

void convertGradientToScale( const ColorGradient::tGradient& gradPoints, 
                             ColorScale::tColorScale& colorPoints )
{
   // Color Points.
   colorPoints.resize(gradPoints.size()*2);
   size_t outIndex = 0;
   for(size_t i = 0; i < gradPoints.size(); ++i)
   {
      bool first = (i == 0);
      bool last  = (i == (gradPoints.size()-1));

      HsvColor hsv = gradientToHsv(gradPoints[i]);
      RgbColor rgb = HsvToRgb(hsv);
      SpecAnLedTypes::tRgbColor color;
      color.rgb.r = rgb.r;
      color.rgb.g = rgb.g;
      color.rgb.b = rgb.b;
      
      if(first)
      {
         colorPoints[outIndex].color = color;
         colorPoints[outIndex].startPoint = 0.0;
         outIndex++;

         colorPoints[outIndex].color = color;
         colorPoints[outIndex].startPoint = colorPoints[outIndex-1].startPoint + gradPoints[i].reach;
         outIndex++;
      }
      else if(last)
      {
         colorPoints[outIndex].color = color;
         colorPoints[outIndex].startPoint = gradPoints[i].position - gradPoints[i].reach;
         outIndex++;

         colorPoints[outIndex].color = color;
         colorPoints[outIndex].startPoint = gradPoints[i].position;
         outIndex++;
      }
      else
      {
         colorPoints[outIndex].color = color;
         colorPoints[outIndex].startPoint = gradPoints[i].position - gradPoints[i].reach;
         outIndex++;

         colorPoints[outIndex].color = color;
         colorPoints[outIndex].startPoint = gradPoints[i].position + gradPoints[i].reach;
         outIndex++;
      }
   }

}

ColorScale::tColorScale convertGradientToScale(const ColorGradient::tGradient& gradPoints)
{
   ColorScale::tColorScale retVal;
   convertGradientToScale(gradPoints, retVal);
   return retVal;
}

SpecAnLedTypes::tRgbColor convertGradientPointToRGB(ColorGradient::tGradientPoint in)
{
   auto rgb = HsvToRgb(gradientToHsv(in));
   SpecAnLedTypes::tRgbColor retVal;
   retVal.u32 = 0; // Set to 0, then fill in RGB values.
   retVal.rgb.r = rgb.r;
   retVal.rgb.g = rgb.g;
   retVal.rgb.b = rgb.b;
   return retVal;
}

ColorGradient::tGradient reverseGradient(ColorGradient::tGradient& in)
{
   ColorGradient::tGradient retVal(in.size());
   int maxIndex = in.size()-1;
   for(int i = 0; i <= maxIndex; ++i)
   {
      auto& pointOut = retVal[maxIndex-i];
      pointOut = in[i];
      pointOut.position = 1.0 - pointOut.position;
   }

   return retVal;
}

}
