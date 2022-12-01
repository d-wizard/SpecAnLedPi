/* Copyright 2013, 2016 - 2017 Dan Williams. All Rights Reserved.
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
#ifndef TCPThreads_h
#define TCPThreads_h

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#if (defined(_WIN32) || defined(__WIN32__))
   #define TCP_SERVER_THREADS_WIN_BUILD
#else
   #define TCP_SERVER_THREADS_LINUX_BUILD
#endif

#ifdef TCP_SERVER_THREADS_WIN_BUILD
   #ifdef __MINGW32_VERSION
      #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
      #endif
      #define _WIN32_WINNT 0x501
   #endif

   // winsock2.h with QT 5.8, mingw530_32 throws a compiler warning about 'windows.h' being included before 'winsock2.h'
   // Undef _WINSOCKAPI_ as a workaround. TODO find a proper solution.
   #ifdef _WINSOCKAPI_
   #undef _WINSOCKAPI_
   #endif
   #include <winsock2.h>
   #include <windows.h>
   #include <ws2tcpip.h>
   
#elif defined TCP_SERVER_THREADS_LINUX_BUILD
   #include <unistd.h>
   #include <string.h>
   #include <netdb.h>
   #include <sys/types.h>
   #include <netinet/in.h>
   #include <sys/socket.h>
   
   typedef unsigned int SOCKET;
   #define closesocket(fd) close(fd)
#endif

#define MAX_PACKET_SIZE (2048)
#define MAX_PACKETS (2048)
#define MAX_STORED_PACKETS (2048)

#define INVALID_FD (0xFFFFFFFF)

typedef struct
{
   pthread_t thread;
   // Multiple threads could read/write to these parameters, so make them volatile.
   volatile int active;
   volatile int kill;
}dSocketThread;

typedef struct
{
   unsigned int buff[((MAX_PACKET_SIZE*MAX_PACKETS)+(sizeof(unsigned int)-1))/sizeof(unsigned int)];

   char* buffPtr;
   unsigned int buffIndex;
   unsigned int maxPacketSize;

   char* packetAddr[MAX_STORED_PACKETS];
   unsigned int packetSize[MAX_STORED_PACKETS];
   unsigned int readIndex;
   unsigned int writeIndex;
}dSocketRxBuff;


typedef void (*dRxPacketCallback)(void*, struct sockaddr_storage*, char*, unsigned int);
typedef void (*dClientConnStartCallback)(void*, struct sockaddr_storage*);
typedef void (*dClientConnEndCallback)(void*, struct sockaddr_storage*);

typedef struct
{
   SOCKET fd;
   struct sockaddr_storage info;
   dSocketThread rxThread;
   dSocketThread procThread;
   dRxPacketCallback rxPacketCallback;
   void* callbackInputPtr;
   sem_t sem;
   dSocketRxBuff rxBuff;
   sem_t* killThisThreadSem;
}dClientConnection;

struct dClientConnList
{
   struct dClientConnList* prev;
   dClientConnection cur;
   struct dClientConnList* next;
};

typedef struct
{
   unsigned short port;
   SOCKET socketFd;
   dSocketThread acceptThread;
   dSocketThread killThread;
   sem_t killThreadSem;
   dRxPacketCallback rxPacketCallback;
   dClientConnStartCallback clientConnStartCallback;
   dClientConnEndCallback clientConnEndCallback;
   void* callbackInputPtr;
   struct dClientConnList* clientList;
   pthread_mutex_t mutex;
}dServerSocket;



void dServerSocket_initClientConn(dClientConnection* dConn, dServerSocket* dSock);
void dServerSocket_wsaInit();
void dServerSocket_init(dServerSocket* dSock,
                        unsigned short port,
                        dRxPacketCallback rxPacketCallback,
                        dClientConnStartCallback clientConnStartCallback,
                        dClientConnEndCallback clientConnEndCallback,
                        void* inputPtr);
void* dServerSocket_acceptThread(void* voidDSock);
void* dServerSocket_rxThread(void* voidDClientConn);
void* dServerSocket_procThread(void* voidDClientConn);
void* dServer_killThreads(void* voidDSock);
void dServerSocket_bind(dServerSocket* dSock);
void dServerSocket_accept(dServerSocket* dSock);
dClientConnection* dServerSocket_createNewClientConn(dServerSocket* dSock);
void dServerSocket_removeClientFromList(struct dClientConnList* clientListPtr, dServerSocket *dSock);
void dServerSocket_deleteClient(struct dClientConnList* clientListPtr, dServerSocket* dSock);
void dServerSocket_newClientConn(dServerSocket* dSock, SOCKET clientFd, struct sockaddr_storage* clientAddr);
void dServerSocket_killAll(dServerSocket* dSock);
void dServerSocket_writeNewPacket(dClientConnection* dConn, char* packet, unsigned int size);
void dServerSocket_readAllPackets(dClientConnection* dConn);
void dServerSocket_updateIndexForNextPacket(dSocketRxBuff* rxBuff);

#endif

