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
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <math.h>
#include <memory> // unique_ptr
#include "alsaMic.h"
#include "smartPlotMessage.h"

void alsaMicSamples(int16_t* samples, size_t numSamp)
{
   smartPlot_1D(samples, E_INT_16, numSamp, 44100, -1, "Mic", "Samp");
}


int main (int argc, char *argv[])
{
   smartPlot_createFlushThread(10);

   sleep(1);

   std::unique_ptr<AlsaMic> mic;

   mic.reset(new AlsaMic("hw:1", 44100, 441, 1, alsaMicSamples));

   sleep(5);

   mic.reset();

   sleep(2);


  return 0;
}