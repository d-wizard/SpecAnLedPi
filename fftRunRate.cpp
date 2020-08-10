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
#include <string.h> // memcpy
#include <algorithm> // std::max
#include "fftRunRate.h"


FftRunRate::FftRunRate(float sampleRate, int fftSize, float fftRate):
   m_sampRate(sampleRate),
   m_fftSize(fftSize),
   m_fft(fftSize)
{
   float newSampPerFft = sampleRate / fftRate;
   assert(newSampPerFft >= fftSize); // Current implementation cannot handle the same sample being used in multiple FFTs.

   m_numSampToRemoveAfterFft = (newSampPerFft + 0.5); // Round to nearest whole number.
   m_numSampNeededToDoFft = std::max(m_numSampToRemoveAfterFft, fftSize);

   m_pcmBuffer.reserve(sampleRate);
   m_fftResult.resize(fftSize>>1);
}

FftRunRate::~FftRunRate()
{

}

tFftVector* FftRunRate::run(tPcmSample* samples, size_t numSamp)
{
   tFftVector* fftResults = nullptr;

   // Copy new samples in.
   auto origSize = m_pcmBuffer.size();
   m_pcmBuffer.resize(origSize+numSamp);
   memcpy(&m_pcmBuffer[origSize], samples, numSamp*sizeof(samples[0]));

   // Run the FFT(s)
   while(m_pcmBuffer.size() >= m_numSampNeededToDoFft)
   {
      m_fft.runFft(m_pcmBuffer.data(), m_fftResult.data());
      fftResults = &m_fftResult;
      m_pcmBuffer.erase(m_pcmBuffer.begin(), m_pcmBuffer.begin()+m_numSampToRemoveAfterFft);
   }

   return fftResults;
}

