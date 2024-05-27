/* Copyright 2022 - 2024 Dan Williams. All Rights Reserved.
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
#include <string.h>
#include "AmbRemoteControl.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

AmbRemoteControl::AmbRemoteControl(uint16_t port, newRemoveCtrlMsgCallback callback):
   m_callback(callback)
{
   // Set up the TCP server. Packets will be received in the "rxPacketCallback" function.
   dServerSocket_init(&m_server, port, rxPacketCallback, nullptr, nullptr, this);
   dServerSocket_bind(&m_server);
   dServerSocket_accept(&m_server);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

AmbRemoteControl::~AmbRemoteControl()
{
   dServerSocket_killAll(&m_server);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

AmbRemoteControl::eDirection AmbRemoteControl::checkGradientChange()
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   if(m_commands.size() > 0)
   {
      if(m_commands[0].cmd == eCommands::E_GRADIENT_POS)
      {
         m_commands.erase(m_commands.begin());
         return E_DIRECTION_POS;
      }
      if(m_commands[0].cmd == eCommands::E_GRADIENT_NEG)
      {
         m_commands.erase(m_commands.begin());
         return E_DIRECTION_NEG;
      }
   }
   return E_DIRECTION_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

AmbRemoteControl::eDirection AmbRemoteControl::checkDisplayChange()
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   if(m_commands.size() > 0)
   {
      if(m_commands[0].cmd == eCommands::E_DISPLAY_CHANGE_POS)
      {
         m_commands.erase(m_commands.begin());
         return E_DIRECTION_POS;
      }
      if(m_commands[0].cmd == eCommands::E_DISPLAY_CHANGE_NEG)
      {
         m_commands.erase(m_commands.begin());
         return E_DIRECTION_NEG;
      }
   }
   return E_DIRECTION_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool AmbRemoteControl::checkReverseGradientToggle()
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   if(m_commands.size() > 0)
   {
      if(m_commands[0].cmd == eCommands::E_REVERSE_GRADIENT_TOGGLE)
      {
         m_commands.erase(m_commands.begin());
         return true;
      }
   }
   return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void AmbRemoteControl::clear()
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   m_commands.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void AmbRemoteControl::rxPacketCallback(void* usrPtr, SOCKET fd, struct sockaddr_storage* sockInfo, char* packetPtr, unsigned int packetSize)
{
   AmbRemoteControl* _this = reinterpret_cast<AmbRemoteControl*>(usrPtr); // Determine which instance of this class we are in.
   _this->processPacket(packetPtr, packetSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void AmbRemoteControl::processPacket(char* packetPtr, unsigned int packetSize)
{
   if(packetPtr == nullptr || packetSize <= 0)
      return; // Invalid packet.

   // Remove invalid bytes from the end of the packet (newline, null term, etc)
   while(packetPtr[packetSize-1] == '\r' || packetPtr[packetSize-1] == '\n' || packetPtr[packetSize-1] == '\0')
   {
      packetSize--;
      if(packetSize <= 0)
         return; // Nothing valid is in the packet, return early.
   }

   // Interpreting input packet as string, so make sure it is null terminated.
   std::vector<char> packetCopy(packetSize+1);
   memcpy(packetCopy.data(), packetPtr, packetSize & 0x3FFFFFFF); // And with large mask of avoid new GCC warning.
   packetCopy[packetSize] = '\0';


   static const std::string GRAD_NAME_STR = "E_GRADIENT_NAME";

   std::string cmdStr(packetCopy.data());
   tCmdAndVal cmdAndVal;
   cmdAndVal.cmd = eCommands::E_INVALID_COMMAND;

   if(cmdStr == "E_GRADIENT_POS")
      cmdAndVal.cmd = eCommands::E_GRADIENT_POS;
   else if(cmdStr == "E_GRADIENT_NEG")
      cmdAndVal.cmd = eCommands::E_GRADIENT_NEG;
   else if(cmdStr.find(GRAD_NAME_STR) == 0)
   {
      cmdAndVal.cmd = eCommands::E_GRADIENT_NAME;
      cmdAndVal.val_str = cmdStr.substr(GRAD_NAME_STR.size());
   }
   else if(cmdStr == "E_DISPLAY_CHANGE_POS")
      cmdAndVal.cmd = eCommands::E_DISPLAY_CHANGE_POS;
   else if(cmdStr == "E_DISPLAY_CHANGE_NEG")
      cmdAndVal.cmd = eCommands::E_DISPLAY_CHANGE_NEG;
   else if(cmdStr == "E_REVERSE_GRADIENT_TOGGLE")
      cmdAndVal.cmd = eCommands::E_REVERSE_GRADIENT_TOGGLE;
   else
   {
      // Check for Gain / Brightness values.
      static const std::string GAIN_STR = "E_GAIN_VALUE";
      auto gainPos = cmdStr.find(GAIN_STR);
      if(gainPos == 0 && cmdStr.size() > GAIN_STR.size()) // GAIN_STR is at the beginning and there are more characters.
      {
         std::lock_guard<std::mutex> lock(m_brightGainMutex);
         strTo(cmdStr.substr(GAIN_STR.size()), m_gainValue);
         cmdAndVal.cmd = eCommands::E_GAIN_VALUE;
         cmdAndVal.val_num = m_gainValue;
      }

      static const std::string BRIGHT_STR = "E_BRIGHT_VALUE";
      auto brightPos = cmdStr.find(BRIGHT_STR);
      if(brightPos == 0 && cmdStr.size() > BRIGHT_STR.size()) // GAIN_STR is at the beginning and there are more characters.
      {
         std::lock_guard<std::mutex> lock(m_brightGainMutex);
         strTo(cmdStr.substr(BRIGHT_STR.size()), m_brightnessValue);
         cmdAndVal.cmd = eCommands::E_BRIGHT_VALUE;
         cmdAndVal.val_num = m_brightnessValue;
      }
   }
   
   // Add the command to the queue.
   bool validCmd = (cmdAndVal.cmd != eCommands::E_INVALID_COMMAND);
   if(validCmd)
   {
      std::lock_guard<std::mutex> lock(m_cmdMutex);
      m_commands.push_back(cmdAndVal);

      // Make sure command queue doesn't get too big.
      if(m_commands.size() > MAX_CMDS_IN_QUEUE)
      {
         m_commands.erase(m_commands.begin(), m_commands.begin() + m_commands.size() - MAX_CMDS_IN_QUEUE); // Erase oldest commands
      }
   }

   // Call the callback (do this without any mutex locked)
   if(validCmd)
   {
      // Call the callback to indicate a new remote control message has come in.
      m_callback();
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool AmbRemoteControl::getRemoteCmd(AmbRemoteControl::tCmdAndVal& cmdAndVal) // Returns true if this is the last.
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   if(m_commands.size() > 0)
   {
      cmdAndVal = m_commands.front();
      m_commands.erase(m_commands.begin());
   }
   else
   {
      cmdAndVal.cmd = eCommands::E_INVALID_COMMAND;
   }
   return (m_commands.size() == 0);
}
