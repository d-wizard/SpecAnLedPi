/* Copyright 2020, 2022 Dan Williams. All Rights Reserved.
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
#include <memory>
#include "AudioDisplayBase.h"
#include "fftRunRate.h"
#include "fftModifier.h"

class AudioDisplayFft : public AudioDisplayBase
{
public:
   typedef enum
   {
      E_GRADIENT_MAG,   // The position on the gradient indications the magnatude.
      E_BRIGHTNESS_MAG  // The brighness indications the magnatude. The color of each LED is constant.
   }eFftColorDisplay;

   AudioDisplayFft(size_t sampleRate, size_t fftSize, size_t numDisplayPoints, eFftColorDisplay colorDisplay, bool mirror = false);

private:
   // Make uncopyable
   AudioDisplayFft();
   AudioDisplayFft(AudioDisplayFft const&);
   void operator=(AudioDisplayFft const&);

   bool processPcm(const SpecAnLedTypes::tPcmSample* samples) override;

   void fillInDisplayPoints(int gain) override;

   // FFT Stuff
   std::unique_ptr<FftRunRate> m_fftRun;
   std::unique_ptr<FftModifier> m_fftModifier;

   SpecAnLedTypes::tFftVector* m_fftResult = nullptr;

   eFftColorDisplay m_brightDisplayType;
};