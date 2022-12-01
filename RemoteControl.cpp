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
#include <string.h>
#include "RemoteControl.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

RemoteControl::RemoteControl(uint16_t port)
{
   // Set up the TCP server. Packets will be received in the "rxPacketCallback" function.
   dServerSocket_init(&m_server, port, rxPacketCallback, nullptr, nullptr, this);
   dServerSocket_bind(&m_server);
   dServerSocket_accept(&m_server);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

RemoteControl::~RemoteControl()
{
   dServerSocket_killAll(&m_server);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

RemoteControl::eDirection RemoteControl::checkGradientChange()
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   if(m_commands.size() > 0)
   {
      if(m_commands[0].cmd == E_GRADIENT_POS)
      {
         m_commands.erase(m_commands.begin());
         return E_DIRECTION_POS;
      }
      if(m_commands[0].cmd == E_GRADIENT_NEG)
      {
         m_commands.erase(m_commands.begin());
         return E_DIRECTION_NEG;
      }
   }
   return E_DIRECTION_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

RemoteControl::eDirection RemoteControl::checkDisplayChange()
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   if(m_commands.size() > 0)
   {
      if(m_commands[0].cmd == E_DISPLAY_CHANGE_POS)
      {
         m_commands.erase(m_commands.begin());
         return E_DIRECTION_POS;
      }
      if(m_commands[0].cmd == E_DISPLAY_CHANGE_NEG)
      {
         m_commands.erase(m_commands.begin());
         return E_DIRECTION_NEG;
      }
   }
   return E_DIRECTION_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteControl::checkReverseGradientToggle()
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   if(m_commands.size() > 0)
   {
      if(m_commands[0].cmd == E_REVERSE_GRADIENT_TOGGLE)
      {
         m_commands.erase(m_commands.begin());
         return true;
      }
   }
   return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteControl::clear()
{
   std::lock_guard<std::mutex> lock(m_cmdMutex);
   m_commands.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteControl::rxPacketCallback(void* usrPtr, struct sockaddr_storage* sockInfo, char* packetPtr, unsigned int packetSize)
{
   RemoteControl* _this = reinterpret_cast<RemoteControl*>(usrPtr); // Determine which instance of this class we are in.
   _this->processPacket(packetPtr, packetSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteControl::processPacket(char* packetPtr, unsigned int packetSize)
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

   // Interpretting input packet as string, so make sure it is null terminated.
   std::vector<char> packetCopy(packetSize+1);
   memcpy(packetCopy.data(), packetPtr, packetSize);
   packetCopy[packetSize] = '\0';

   std::string cmdStr(packetCopy.data());
   eCommands cmdVal = E_INVALID_COMMAND;

   if(cmdStr == "E_GRADIENT_POS")
      cmdVal = E_GRADIENT_POS;
   else if(cmdStr == "E_GRADIENT_NEG")
      cmdVal = E_GRADIENT_NEG;
   else if(cmdStr == "E_DISPLAY_CHANGE_POS")
      cmdVal = E_DISPLAY_CHANGE_POS;
   else if(cmdStr == "E_DISPLAY_CHANGE_NEG")
      cmdVal = E_DISPLAY_CHANGE_NEG;
   else if(cmdStr == "E_REVERSE_GRADIENT_TOGGLE")
      cmdVal = E_REVERSE_GRADIENT_TOGGLE;
   
   if(cmdVal != E_INVALID_COMMAND)
   {
      std::lock_guard<std::mutex> lock(m_cmdMutex);
      tCmdDataPair cmdDataPair;
      cmdDataPair.cmd = cmdVal;
      m_commands.push_back(cmdDataPair);
   }
}
