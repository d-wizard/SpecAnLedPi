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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // Needed for pthread_setname_np
#define NEED_TO_UNDEF_GNU_SOURCE
#endif

#include <pthread.h>
#include <sched.h>

namespace ThreadPriorities
{
   static inline void setThisThreadPriorityPolicy(int priority, int policy)
   {
      int dummyPolicyRead;
      struct sched_param param;
      pthread_getschedparam(pthread_self(), &dummyPolicyRead, &param);
      param.sched_priority = priority;
      pthread_setschedparam(pthread_self(), policy, &param);
   }

   static inline void setThisThreadName(const char* threadName)
   {
      pthread_setname_np(pthread_self(), threadName);
   }


   // Thread Priorities for this application.
   static constexpr int ROTORY_ENCODER_POLL_THREAD_PRIORITY = 99;
   static constexpr int GRADIENT_CHANGE_THREAD_PRIORITY = 98;
   static constexpr int USER_CUE_THREAD_PRIORITY = 97;
}

#ifdef NEED_TO_UNDEF_GNU_SOURCE
#undef _GNU_SOURCE
#endif

