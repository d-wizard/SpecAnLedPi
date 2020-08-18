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
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "ws2811.h"
#include "specAnLedPiTypes.h"

class LedStrip
{
public:
   typedef enum
   {
      RGB,
      RBG,
      GRB,
      GBR,
      BRG,
      BGR
   }eRgbOrder;

   LedStrip(size_t numLeds, eRgbOrder order, unsigned gpio = 18);
   virtual ~LedStrip();

   void set(const tRgbVector& ledColors);
   void clear();

private:
   // Make uncopyable
   LedStrip();
   LedStrip(LedStrip const&);
   void operator=(LedStrip const&);

   const size_t m_numLeds;
   ws2811_t m_ledStrip;
};


