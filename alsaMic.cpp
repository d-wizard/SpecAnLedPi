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
#include "alsaMic.h"
#include "ThreadPriorities.h"

#define ALSA_ERR(printStr, err) if(err < 0) {printf("%s - %s\n", printStr, snd_strerror(err)); return err;}

AlsaMic::AlsaMic(const char* micName, unsigned int sampleRate, size_t sampPer, size_t numChannels, alsaMicFunctr callbackFunc, void* callbackUsrPtr):
   m_micName(micName),
   m_sampleRate(sampleRate),
   m_sampPer(sampPer),
   m_numChannels(numChannels),
   m_callbackFunc(callbackFunc),
   m_callbackUsrPtr(callbackUsrPtr)
{
   if(callbackFunc != nullptr)
   {
      int err = init();
      if(err >= 0)
      {
         m_running = true;
         pthread_create(&m_readThread, nullptr, micReadThreadFunction, this);
      }
   }
}

AlsaMic::~AlsaMic()
{
   deinit();
}

int AlsaMic::init()
{
   // Initialize all the Alsa stuff.
   int err = 0;
   snd_pcm_hw_params_t* alsaParams = nullptr;

   // Get a handle to the Microphone.
   err = snd_pcm_open(&m_alsaHandle, m_micName.c_str(), SND_PCM_STREAM_CAPTURE, 0);
   ALSA_ERR("snd_pcm_open", err); // This will early return on error.
   
   // Allocate HW Paramters Settings and fill the settings in.
   {
      err = snd_pcm_hw_params_malloc(&alsaParams);
      ALSA_ERR("snd_pcm_hw_params_malloc", err); // This will early return on error.
      
      err = snd_pcm_hw_params_any(m_alsaHandle, alsaParams);
      ALSA_ERR("snd_pcm_hw_params_any", err); // This will early return on error.
      
      err = snd_pcm_hw_params_set_format(m_alsaHandle, alsaParams, m_alsaFormat);
      ALSA_ERR("snd_pcm_hw_params_set_format", err); // This will early return on error.

      err = snd_pcm_hw_params_set_rate_near(m_alsaHandle, alsaParams, &m_sampleRate, 0);
      ALSA_ERR("snd_pcm_hw_params_set_rate_near", err); // This will early return on error.

      if(m_numChannels >= 1) // TODO, if only one channel, can I optimize better? (shouldn't need to interleave)
      {
         err = snd_pcm_hw_params_set_access(m_alsaHandle, alsaParams, SND_PCM_ACCESS_RW_INTERLEAVED);
         ALSA_ERR("snd_pcm_hw_params_set_access", err); // This will early return on error.

         err = snd_pcm_hw_params_set_channels(m_alsaHandle, alsaParams, m_numChannels);
         ALSA_ERR("snd_pcm_hw_params_set_channels", err); // This will early return on error.
      }

      // Following example in https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2latency_8c-example.html#a18 , setparams_bufsize
      snd_pcm_uframes_t periodsize = 2*m_sampPer;
      err = snd_pcm_hw_params_set_buffer_size_near(m_alsaHandle, alsaParams, &periodsize);
      ALSA_ERR("snd_pcm_hw_params_set_buffer_size_near", err); // This will early return on error.

      periodsize >>= 1;
      err = snd_pcm_hw_params_set_period_size_near(m_alsaHandle, alsaParams, &periodsize, 0);
      ALSA_ERR("snd_pcm_hw_params_set_period_size_near", err); // This will early return on error.

      err = snd_pcm_hw_params(m_alsaHandle, alsaParams);
      ALSA_ERR("snd_pcm_hw_params", err); // This will early return on error.

      snd_pcm_hw_params_free(alsaParams);
   }

   err = snd_pcm_prepare(m_alsaHandle);
   ALSA_ERR("snd_pcm_prepare", err); // This will early return on error.

   return err;
}

int AlsaMic::deinit()
{
   if(m_running)
   {
      m_running = false;
      pthread_join(m_readThread, nullptr);
      snd_pcm_close(m_alsaHandle);
   }

   return 0;
}

void* AlsaMic::micReadThreadFunction(void* inPtr)
{
   ThreadPriorities::setThisThreadPriorityPolicy(ThreadPriorities::ALSA_MIC_THREAD_PRIORITY, SCHED_FIFO);

   AlsaMic* _this = (AlsaMic*)inPtr;

   _this->m_buffer.resize(_this->m_sampPer*_this->m_numChannels);

   snd_pcm_t* handle = _this->m_alsaHandle;
   void* usrPtr = _this->m_callbackUsrPtr;
   int16_t* buffer = &_this->m_buffer[0];
   size_t numSamp = _this->m_sampPer;

   while(_this->m_running)
   {
      auto err = snd_pcm_readi(handle, buffer, numSamp); // TODO i at the end means interleaved, for 1 channel can I use other function??
      if(err != (signed)numSamp)
      {
         _this->m_running = false;
      }
      else
      {
         _this->m_callbackFunc(usrPtr, buffer, numSamp); // Send to callback.
      }
      
   }
   return NULL;
}


