/* Copyright 2022 Dan Williams. All Rights Reserved.
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
#include <mutex>
#include <sstream>
#include <atomic>
#include "TCPThreads.h"

class RemoteControl
{
// Some public types
public:
   typedef enum
   {
      E_DIRECTION_NO_CHANGE,
      E_DIRECTION_POS,
      E_DIRECTION_NEG
   }eDirection;

// Some private types / constants
private:
   static constexpr size_t MAX_CMDS_IN_QUEUE = 100; // Limit to avoid heap overrun.

   typedef enum
   {
      E_GRADIENT_POS,
      E_GRADIENT_NEG,
      E_DISPLAY_CHANGE_POS,
      E_DISPLAY_CHANGE_NEG,
      E_REVERSE_GRADIENT_TOGGLE,
      E_GAIN_BRIGHT_LOCAL,
      E_GAIN_BRIGHT_REMOTE,
      E_GAIN_VALUE,
      E_BRIGHT_VALUE,
      E_INVALID_COMMAND
   }eCommands;

   typedef struct
   {
      eCommands cmd;
      std::vector<uint8_t> data;
   }tCmdDataPair;
   
public:
   RemoteControl(uint16_t port, bool useRemoteGainBrightness = false);
   virtual ~RemoteControl();

   eDirection checkGradientChange();
   eDirection checkDisplayChange();
   bool checkReverseGradientToggle();

   bool useRemoteGainBrightness(){ return m_useRemoteGainBrightness; }
   int getGain(){ std::lock_guard<std::mutex> lock(m_brightGainMutex); return m_gainValue; }
   float getBrightness(){ std::lock_guard<std::mutex> lock(m_brightGainMutex); return m_brightnessValue; }

   void clear();

   
private:
   // Make uncopyable
   RemoteControl();
   RemoteControl(RemoteControl const&);
   void operator=(RemoteControl const&);

   static void rxPacketCallback(void* usrPtr, struct sockaddr_storage* sockInfo, char* packetPtr, unsigned int packetSize);

   void processPacket(char* packetPtr, unsigned int packetSize);

   // Parameters for keeping track of the received commands.
   std::mutex m_cmdMutex;
   std::vector<tCmdDataPair> m_commands;

   // This is the server for receiving the remote commands.
   dServerSocket m_server;

   // Parameters for keeping track of brightness and gain.
   std::mutex m_brightGainMutex;
   std::atomic<bool> m_useRemoteGainBrightness;
   int m_gainValue = 0;
   float m_brightnessValue = 0;

   
   // Function for safely converting string to other values.
   template <class type> bool strTo(const std::string& t_input, type& toVal)
   {
      std::istringstream iss(t_input);
      type tryStrTo;

      // Try to convert string to type.
      iss >> std::noskipws >> tryStrTo; // noskipws means leading whitespace is invalid

      // Make sure the conversion used the entire string without failure.
      bool success = iss.eof() && !iss.fail();

      if(success)
      {
         // Write valid value 
         toVal = tryStrTo;
      }

      // Indicate whether conversion was successful.
      return success; 
   }

};

