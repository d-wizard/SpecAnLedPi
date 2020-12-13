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
#include <pthread.h>
#include <string>
#include <vector>
#include <alsa/asoundlib.h>


class AlsaMic
{
public:
   typedef void (*alsaMicFunctr)(void*, int16_t*, size_t); // variables are (void* usrPtr, int16_t* samples, size_t numSamples)

   AlsaMic(const char* micName, unsigned int sampleRate, size_t sampPer, size_t numChannels, alsaMicFunctr callbackFunc, void* callbackUsrPtr);
   virtual ~AlsaMic();

private:
   // Make uncopyable
   AlsaMic();
   AlsaMic(AlsaMic const&);
   void operator=(AlsaMic const&);

   // Private functions
   static void* micReadThreadFunction(void* inPtr);

   // Private Functions
   int init();
   int deinit();

   // Private Member Variables
   snd_pcm_t* m_alsaHandle = nullptr;
   static constexpr snd_pcm_format_t m_alsaFormat = SND_PCM_FORMAT_S16_LE; // Anything other than this is wrong (sorry audiophiles and big endian fans).

   pthread_t m_readThread;
   std::string m_micName;
   unsigned int m_sampleRate;
   size_t m_sampPer;
   size_t m_numChannels;
   alsaMicFunctr m_callbackFunc;
   void* m_callbackUsrPtr;
   std::vector<int16_t> m_buffer;

   bool m_running = false;

};


