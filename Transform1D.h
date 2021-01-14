/* Copyright 2021 Dan Williams. All Rights Reserved.
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

#include <math.h>



namespace Transform1D
{
   // Unit transforms (i.e. only meant to be used by inputs 0 to 1)
   namespace Unit
   {
      // Output will always be higher than the input (or equal for inputs of 0 or 1).
      // Mapping a quarter circle so input values closer to 0 are highly increased,
      // but values closer to 1 aren't increased that much.
      // See: https://www.wolframalpha.com/input/?i=y%3D%281+-+%281-x%29%5E2%29%5E0.5+from+x%3D0+to+1
      static inline double quarterCircle_above(double input)
      {
         double reflect = double(1.0) - input;
         return sqrt(double(1.0)-reflect*reflect);
      }

      // Output will always be less than the input (or equal for inputs of 0 or 1).
      // Mapping a quarter circle so input values closer to 0 are reduced significantly,
      // but values closer to 1 aren't reduced that much.
      // See: https://www.wolframalpha.com/input/?i=y%3D%281-%281-x%5E2%29%5E0.5%29+from+x%3D0+to+1
      static inline double quarterCircle_below(double input)
      {
         return double(1.0) - sqrt(double(1.0)-input*input);
      }
   }
}

