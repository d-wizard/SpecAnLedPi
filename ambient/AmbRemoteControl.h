/* Copyright 2022, 2024 Dan Williams. All Rights Reserved.
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

class AmbRemoteControl
{
// Some public types
public:
   typedef enum
   {
      E_DIRECTION_NO_CHANGE,
      E_DIRECTION_POS,
      E_DIRECTION_NEG
   }eDirection;

   typedef void (*newRemoveCtrlMsgCallback)(void);
// Some private types / constants
public:
   static constexpr size_t MAX_CMDS_IN_QUEUE = 100; // Limit to avoid heap overrun.

   enum class eCommands
   {
      E_GRADIENT_POS,
      E_GRADIENT_NEG,
      E_GRADIENT_NAME,
      E_DISPLAY_CHANGE_POS,
      E_DISPLAY_CHANGE_NEG,
      E_REVERSE_GRADIENT_TOGGLE,
      E_GAIN_VALUE,
      E_BRIGHT_VALUE,
      E_TOGGLE_SPOTLIGHT,
      E_INVALID_COMMAND
   };

   typedef struct
   {
      eCommands cmd;
      double val_num;
      std::string val_str;
   }tCmdAndVal;
   
public:
   AmbRemoteControl(uint16_t port, newRemoveCtrlMsgCallback callback);
   virtual ~AmbRemoteControl();

   eDirection checkGradientChange();
   eDirection checkDisplayChange();
   bool checkReverseGradientToggle();

   float getGain(){ std::lock_guard<std::mutex> lock(m_brightGainMutex); return m_gainValue; }
   float getBrightness(){ std::lock_guard<std::mutex> lock(m_brightGainMutex); return m_brightnessValue; }

   bool getRemoteCmd(tCmdAndVal& cmdAndVal); // Returns true if this is the last.

   void clear();

   
private:
   // Make uncopyable
   AmbRemoteControl();
   AmbRemoteControl(AmbRemoteControl const&);
   void operator=(AmbRemoteControl const&);

   static void rxPacketCallback(void* usrPtr, SOCKET fd, struct sockaddr_storage* sockInfo, char* packetPtr, unsigned int packetSize);

   void processPacket(char* packetPtr, unsigned int packetSize);

   // Callback
   newRemoveCtrlMsgCallback m_callback;

   // Parameters for keeping track of the received commands.
   std::mutex m_cmdMutex;
   std::vector<tCmdAndVal> m_commands;

   // This is the server for receiving the remote commands.
   dServerSocket m_server;

   // Parameters for keeping track of brightness and gain.
   std::mutex m_brightGainMutex;
   float m_gainValue = -1;       // initialize to invalid value.
   float m_brightnessValue = -1; // initialize to invalid value.


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

