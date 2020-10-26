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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h> // Needed for close
#include <wiringPiI2C.h>

class SeeedAdc8Ch12Bit
{
public:
   SeeedAdc8Ch12Bit(int deviceAddr = 0x04): // 0x04 appears to be the default address for this ADC Hat.
      m_deviceAddr(deviceAddr)
   {
      m_fd = wiringPiI2CSetup(m_deviceAddr);
   }

   virtual ~SeeedAdc8Ch12Bit()
   {
      if(isActive())
      {
         close(m_fd);
      }
   }

   bool isActive() { return (m_fd >= 0); }

   uint16_t getAdcValue(int adcNum)
   {
      uint16_t retVal = 0;
      if(isActive())
      {
         adcNum = adcNum & 0x7;
         retVal = wiringPiI2CReadReg16(m_fd, ADC_VALUE_REG_ADDR_START + adcNum);
      }
      return retVal;
   }
   
   // Delete constructors / operations that should not be allowed.
   SeeedAdc8Ch12Bit(SeeedAdc8Ch12Bit const&) = delete;
   void operator=(SeeedAdc8Ch12Bit const&) = delete;
private:
   static constexpr int ADC_VALUE_REG_ADDR_START = 0x10;

   int m_deviceAddr;
   int m_fd = -1;

};

