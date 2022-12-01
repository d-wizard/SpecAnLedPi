/* Copyright 2013, 2017, 2019 Dan Williams. All Rights Reserved.
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
#include "TCPThreads.h"


#if defined TCP_SERVER_THREADS_WIN_BUILD && !defined __MINGW32_VERSION
   #ifdef __cplusplus
      #define snprintf _snprintf_s
   #else
      #define snprintf _snprintf
   #endif
#endif

void dServerSocket_updateIndexForNextPacket(dSocketRxBuff* rxBuff)
{
   unsigned int buffIndex = (rxBuff->buffIndex + 3) & ((unsigned int)-4);
   if( (buffIndex + rxBuff->maxPacketSize) >= sizeof(rxBuff->buff))
   {
      rxBuff->buffIndex = 0;
   }
   else
   {
      rxBuff->buffIndex = buffIndex;
   }
}

void dServerSocket_writeNewPacket(dClientConnection* dConn, char* packet, unsigned int size)
{
   dConn->rxBuff.packetAddr[dConn->rxBuff.writeIndex] = packet;
   dConn->rxBuff.packetSize[dConn->rxBuff.writeIndex] = size;
   if(++dConn->rxBuff.writeIndex == MAX_STORED_PACKETS)
   {
      dConn->rxBuff.writeIndex = 0;
   }
}

void dServerSocket_readAllPackets(dClientConnection* dConn)
{
   while(dConn->rxBuff.readIndex != dConn->rxBuff.writeIndex)
   {
      dConn->rxPacketCallback(
         dConn->callbackInputPtr,
         &dConn->info,
         dConn->rxBuff.packetAddr[dConn->rxBuff.readIndex],
         dConn->rxBuff.packetSize[dConn->rxBuff.readIndex] );
      if(++dConn->rxBuff.readIndex == MAX_STORED_PACKETS)
      {
         dConn->rxBuff.readIndex = 0;
      }
   }
}

void dServerSocket_initClientConn(dClientConnection* dConn, dServerSocket* dSock)
{
   memset(dConn, 0, sizeof(dClientConnection));
   dConn->fd = INVALID_FD;
   dConn->rxBuff.maxPacketSize = MAX_PACKET_SIZE;
   dConn->rxBuff.buffPtr = (char*)dConn->rxBuff.buff;
   dConn->rxPacketCallback = dSock->rxPacketCallback;
   dConn->callbackInputPtr = dSock->callbackInputPtr;
   dConn->killThisThreadSem = &dSock->killThreadSem;

}

#ifdef TCP_SERVER_THREADS_WIN_BUILD
void dServerSocket_wsaInit()
{
   WSADATA wsda;
   WSAStartup(0x0101, &wsda);
}
#endif

void dServerSocket_init(dServerSocket* dSock,
                        unsigned short port,
                        dRxPacketCallback rxPacketCallback,
                        dClientConnStartCallback clientConnStartCallback,
                        dClientConnEndCallback clientConnEndCallback,
                        void* inputPtr)
{
   memset(dSock, 0, sizeof(dServerSocket));
   dSock->port = port;
   dSock->rxPacketCallback = rxPacketCallback;
   dSock->clientConnStartCallback = clientConnStartCallback;
   dSock->clientConnEndCallback = clientConnEndCallback;
   dSock->callbackInputPtr = inputPtr;

   sem_init(&dSock->killThreadSem, 0, 0);
   pthread_mutex_init(&dSock->mutex, NULL);

   dSock->clientList = NULL;

#ifdef TCP_SERVER_THREADS_WIN_BUILD
   dServerSocket_wsaInit();
#endif
}

void* dServerSocket_acceptThread(void* voidDSock)
{
   dServerSocket* dSock = (dServerSocket*)voidDSock;
   struct sockaddr_storage clientAddr;
   SOCKET connectionFd = 0;

   dSock->acceptThread.active = 1;
   //printf("acceptThread start\n");

   while(!dSock->acceptThread.kill)
   {
      if( listen(dSock->socketFd, 10) != -1)
      {
         socklen_t sockStoreSize = sizeof(clientAddr);
         connectionFd = accept(dSock->socketFd, (struct sockaddr*)&clientAddr, &sockStoreSize);
         if(connectionFd != INVALID_FD)
         {
            dServerSocket_newClientConn(dSock, connectionFd, &clientAddr);
         }
      }
      else
      {
         dSock->acceptThread.kill = 1;
      }
   }
   dSock->acceptThread.active = 0;
   //printf("acceptThread end\n");

   return NULL;
}

void* dServerSocket_rxThread(void* voidDClientConn)
{
   dClientConnection* dClientConn = (dClientConnection*)voidDClientConn;
   int packetSize = 0;

   dClientConn->rxThread.active = 1;
   //printf("rxThread start\n");
   while(!dClientConn->rxThread.kill)
   {
      dServerSocket_updateIndexForNextPacket(&dClientConn->rxBuff);

      packetSize = recv( dClientConn->fd, 
                         &dClientConn->rxBuff.buffPtr[dClientConn->rxBuff.buffIndex], 
                         dClientConn->rxBuff.maxPacketSize, 
                         0 );
      if(packetSize > 0)
      {
         dServerSocket_writeNewPacket( dClientConn, 
                                       &dClientConn->rxBuff.buffPtr[dClientConn->rxBuff.buffIndex], 
                                       packetSize );
         sem_post(&dClientConn->sem);
         dClientConn->rxBuff.buffIndex += packetSize;
      }
      else
      {
         //printf("rxThread wrong packet size\n");
         dClientConn->rxThread.kill = 1;
      }

   }
   dClientConn->rxThread.active = 0;
   //printf("rxThread end\n");

   sem_post(dClientConn->killThisThreadSem);

   return NULL;
}

void* dServerSocket_procThread(void* voidDClientConn)
{
   dClientConnection* dClientConn = (dClientConnection*)voidDClientConn;
   dClientConn->procThread.active = 1;
   //printf("procThread start\n");

   do
   {
      sem_wait(&dClientConn->sem);
      dServerSocket_readAllPackets(dClientConn);
   }while(!dClientConn->procThread.kill);

   dClientConn->procThread.active = 0;
   //printf("procThread end\n");

   return NULL;
}

void* dServer_killThreads(void* voidDSock)
{
   dServerSocket* dSock = (dServerSocket*)voidDSock;
   struct dClientConnList* clientListPtr = NULL;
   struct dClientConnList* curClient = NULL;
   
   dSock->killThread.active = 1;
   //printf("killThread Start\n");


   // Make sure all the client connection threads have been
   // closed before exiting this thread.
   while(!dSock->killThread.kill || dSock->clientList != NULL)
   {
      sem_wait(&dSock->killThreadSem);
      clientListPtr = dSock->clientList;
      curClient = NULL;
      while(clientListPtr != NULL)
      {
         if( clientListPtr->cur.fd != INVALID_FD &&
             clientListPtr->cur.rxThread.active == 0 && 
             clientListPtr->cur.rxThread.kill == 1 )
         {

            pthread_mutex_lock(&dSock->mutex);
            curClient = clientListPtr;
            clientListPtr = clientListPtr->next;
            dServerSocket_removeClientFromList(curClient, dSock);
            pthread_mutex_unlock(&dSock->mutex);

            pthread_join(curClient->cur.rxThread.thread, NULL);

            while(curClient->cur.procThread.active == 0);
            curClient->cur.procThread.kill = 1;
            sem_post(&curClient->cur.sem);
            pthread_join(curClient->cur.procThread.thread, NULL);
            
            dServerSocket_deleteClient(curClient, dSock);
         }
         else
         {
            clientListPtr = clientListPtr->next;
         }
      }
   }

   dSock->killThread.active = 0;
   //printf("killThread end\n");

   return NULL;
}

void dServerSocket_bind(dServerSocket* dSock)
{
   struct addrinfo hints;
   struct addrinfo* serverInfoList;
   struct addrinfo* serverInfo;
   SOCKET socketFd;

   char portStr[6];

   int success = 0;

   dSock->socketFd = INVALID_FD;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;// AF_UNSPEC; IPv4 only for now
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE; // use my IP

   snprintf( portStr, sizeof(portStr), "%d", dSock->port);

   if( !getaddrinfo(NULL, portStr, &hints, &serverInfoList) )
   {
      for(serverInfo = serverInfoList; serverInfo != NULL; serverInfo = serverInfo->ai_next)
      {
         socketFd = socket( serverInfo->ai_family,
                            serverInfo->ai_socktype,
                            serverInfo->ai_protocol );
         if(socketFd != INVALID_FD)
         {
            int numParam = 1;
            if( setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (char*)&numParam, sizeof(numParam)) != -1 )
            {
               if( bind(socketFd, serverInfo->ai_addr, (int)serverInfo->ai_addrlen) != -1)
               {
                  dSock->socketFd = socketFd;
                  success = 1;
                  // Only use the first successfull bind, might need to use all for IPv4 and IPv6 ???
                  break;
               }
            }
            
            if(!success)
            {
               closesocket(socketFd);
            }
         }
      }
   }

   freeaddrinfo(serverInfoList);

}

void dServerSocket_accept(dServerSocket* dSock)
{
   pthread_create(&dSock->acceptThread.thread, NULL, dServerSocket_acceptThread, dSock);
   pthread_create(&dSock->killThread.thread, NULL, dServer_killThreads, dSock);
}

dClientConnection* dServerSocket_createNewClientConn(dServerSocket* dSock)
{
   struct dClientConnList* newDClientConn = NULL;
   pthread_mutex_lock(&dSock->mutex);

   newDClientConn = (struct dClientConnList*)malloc(sizeof(struct dClientConnList));
   if(dSock->clientList == NULL)
   {
      dSock->clientList = newDClientConn;
      dSock->clientList->prev = NULL;
      dSock->clientList->next = NULL;
   }
   else
   {
      struct dClientConnList* findMaxPtr = dSock->clientList;
      while(findMaxPtr->next != NULL)
      {
         findMaxPtr = findMaxPtr->next;
      }
      newDClientConn->next = NULL;
      newDClientConn->prev = findMaxPtr;
      findMaxPtr->next = newDClientConn;
   }

   pthread_mutex_unlock(&dSock->mutex);

   return &newDClientConn->cur;
}

void dServerSocket_removeClientFromList(struct dClientConnList* clientListPtr, dServerSocket* dSock)
{
   if(clientListPtr->prev != NULL && clientListPtr->next != NULL)
   {
      // Not the beginning or the end
      clientListPtr->prev->next = clientListPtr->next;
      clientListPtr->next->prev = clientListPtr->prev;
   }
   else if(clientListPtr->prev == NULL && clientListPtr->next == NULL)
   {
      // The beginning and the end
      dSock->clientList = NULL;
   }
   else if(clientListPtr->prev == NULL)
   {
      // The beginning
      dSock->clientList = clientListPtr->next;
      clientListPtr->next->prev = NULL;
   }
   else
   {
      // The end
      clientListPtr->prev->next = NULL;
   }

}

void dServerSocket_deleteClient(struct dClientConnList* clientListPtr, dServerSocket* dSock)
{
   sem_destroy(&clientListPtr->cur.sem);

   // inform parent that a client has been disconnected
   if(dSock->clientConnEndCallback != NULL)
   {
      dSock->clientConnEndCallback(dSock->callbackInputPtr, &clientListPtr->cur.info);
   }
   free(clientListPtr);
}

void dServerSocket_newClientConn(dServerSocket* dSock, SOCKET clientFd, struct sockaddr_storage* clientAddr)
{
   // setup the new client connection structure
   dClientConnection* dConn = dServerSocket_createNewClientConn(dSock);
   dServerSocket_initClientConn(dConn, dSock);
   dConn->fd = clientFd;
   dConn->info = *clientAddr;
   sem_init(&dConn->sem, 0, 0);

   // inform parent that a new client has connected
   if(dSock->clientConnStartCallback != NULL)
   {
      dSock->clientConnStartCallback(dSock->callbackInputPtr, &dConn->info);
   }

   pthread_create((pthread_t*)&dConn->rxThread, NULL, dServerSocket_rxThread, dConn);
   pthread_create((pthread_t*)&dConn->procThread, NULL, dServerSocket_procThread, dConn);
   //printf("New Client %d\n", clientFd);
}

void dServerSocket_killAll(dServerSocket* dSock)
{
   struct dClientConnList* clientListPtr = NULL;

   // Kill the accept thread.
   while(dSock->acceptThread.active == 0 && dSock->acceptThread.kill == 0);
   dSock->acceptThread.kill = 1;
#ifdef TCP_SERVER_THREADS_WIN_BUILD
   closesocket(dSock->socketFd);
#elif defined TCP_SERVER_THREADS_LINUX_BUILD
   shutdown(dSock->socketFd, 2);
#endif
   pthread_join(dSock->acceptThread.thread, NULL);

   // Close all the client connections still active.
   pthread_mutex_lock(&dSock->mutex);
   clientListPtr = dSock->clientList;
   while(clientListPtr != NULL)
   {
      closesocket(clientListPtr->cur.fd);
      clientListPtr = clientListPtr->next;
   }
   pthread_mutex_unlock(&dSock->mutex);

   // Kill the kill thread
   while(dSock->killThread.active == 0);
   dSock->killThread.kill = 1;
   sem_post(&dSock->killThreadSem);
   pthread_join(dSock->killThread.thread, NULL);

   sem_destroy(&dSock->killThreadSem);
   pthread_mutex_destroy(&dSock->mutex);

}

