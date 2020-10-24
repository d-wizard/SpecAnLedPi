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

#include <stdint.h>
#include <vector>

namespace SpecAnLedTypes
{
   typedef int16_t tPcmSample;
   typedef uint16_t tFftBin;

   typedef std::vector<tPcmSample> tPcmBuffer;

   typedef std::vector<tFftBin> tFftVector;

   typedef struct
   {
      // Typical hex color codes are 0xRRGGBB (i.e. blue is the LSB)
      uint8_t b;
      uint8_t g;
      uint8_t r;
   }tRgbStruct;

   typedef union
   {
      tRgbStruct rgb;
      uint32_t u32;
   }tRgbColor;

   // Define some colors for easy use.
   static constexpr uint32_t COLOR_RED     = 0xFF0000;
   static constexpr uint32_t COLOR_GREEN   = 0x00FF00;
   static constexpr uint32_t COLOR_BLUE    = 0x0000FF;
   static constexpr uint32_t COLOR_YELLOW  = 0xFFFF00;
   static constexpr uint32_t COLOR_CYAN    = 0x00FFFF;
   static constexpr uint32_t COLOR_MAGENTA = 0xFF00FF;
   static constexpr uint32_t COLOR_WHITE   = 0xFFFFFF;
   static constexpr uint32_t COLOR_BLACK   = 0x000000;
   static constexpr uint32_t COLOR_ORANGE  = 0xFF4500; // Orange is not as well defined as previous colors. Here is my best guess.
   static constexpr uint32_t COLOR_PURPLE  = 0x8A2BE2; // Purple is not as well defined as previous colors. Here is my best guess.

   typedef std::vector<tRgbColor> tRgbVector;

}

