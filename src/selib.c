/*
 *     ____             _________                __                _
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/
 *                                                       /____/
 *
 *                 SharkSSL Embedded SSL/TLS Stack
 ****************************************************************************
 *   PROGRAM MODULE
 *
 *   $Id: selib.c 3894 2016-06-06 21:01:39Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014
 *
 *   This software is copyrighted by and is the sole property of Real
 *   Time Logic LLC.  All rights, title, ownership, or other interests in
 *   the software remain the property of Real Time Logic LLC.  This
 *   software may only be used in accordance with the terms and
 *   conditions stipulated in the corresponding license agreement under
 *   which the software has been supplied.  Any unauthorized use,
 *   duplication, transmission, distribution, or disclosure of this
 *   software is expressly forbidden.
 *
 *   This Copyright notice may not be removed or modified without prior
 *   written consent of Real Time Logic LLC.
 *
 *   Real Time Logic LLC. reserves the right to modify this software
 *   without notice.
 *
 *               https://realtimelogic.com
 ****************************************************************************
 */

#define SELIB_C

#include "selib.h"

#ifndef NO_BSD_SOCK

#if !defined(_WIN32) && !defined(closesocket)
#define INVALID_SOCKET -1
#define closesocket close
#endif


/* Wait 'tmo' milliseconds for socket 'read' activity.
   Returns 0 on pending data and -1 on timeout.
*/
#ifndef X_readtmo
static int readtmo(SOCKET sock, U32 tmo)
{
   fd_set recSet;
   struct timeval tv;
   tv.tv_sec = tmo / 1000;
   tv.tv_usec = (tmo % 1000) * 1000;
   FD_ZERO(&recSet);
   FD_SET(sock, &recSet);
   return select(sock+1, &recSet, 0, 0, &tv) > 0 ? 0 : -1;
}
#endif

#ifndef X_se_connect
int se_connect(SOCKET* sock, const char* address, U16 port)
{
   unsigned int ip;
   struct sockaddr_in addr;
   struct hostent* hostInfo = gethostbyname(address);
   *sock=-1;
   if(hostInfo)
      ip=((struct in_addr *)hostInfo->h_addr)->s_addr;
   else
   {
      ip=inet_addr(address);
      if(ip == INADDR_NONE)
         return -2;
   }
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = ip;
   if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      return -1;
   if(connect(*sock, (struct sockaddr*)&addr, sizeof(addr)) != INVALID_SOCKET)
      return 0;
   se_close(sock);
   return -3;
}
#endif


#ifndef X_se_bind
int se_bind(SOCKET* sock, U16 port)
{
   struct sockaddr_in  addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;
   if((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      xprintf(("Socket error\n"));
      return -1;
   }
   if(bind(*sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
   {
      xprintf(("Bind error: port %d\n", (int)port));
      se_close(sock);
      return -3;
   }
   if(listen(*sock, SOMAXCONN) < 0)
   {
      xprintf(("Listen error\n"));
      se_close(sock);
      return -2;
   }
   return 0;
}
#endif



/*
  Extract the first connection on the queue of pending connections,
  create a new socket, and allocate a new file descriptor for that
  socket.

  Returns:
  1: Success
  0: timeout
  -1: error

*/
int se_accept(SOCKET** listenSock, U32 timeout, SOCKET** outSock)
{
   if(timeout != INFINITE_TMO)
   {
      **outSock = -1;
      if(readtmo(**listenSock,timeout))
         return 0;
   }
   if( (**outSock=accept(**listenSock, 0, 0)) < 0 )
      return -1;
   return 1;
}


void se_close(SOCKET* sock)
{
   closesocket(*sock);
   *sock=-1;
}


int se_sockValid(SOCKET* sock)
{
   return *sock > 0 ? 1 : 0;
}


S32 se_send(SOCKET* sock, const void* buf, U32 len)
{
   return send(*sock,(void*)buf,len,0);
}


#ifndef X_se_recv
S32 se_recv(SOCKET* sock, void* buf, U32 len, U32 timeout)
{
   int recLen;
   if(timeout != INFINITE_TMO)
   {
      if(readtmo(*sock,timeout))
         return 0;
   }

   recLen = recv(*sock,buf,len,0);
   if (recLen <= 0)
   {
      /* If the virtual circuit was closed gracefully, and
       * all data was received, then a recv will return
       * immediately with zero bytes read.
       * We return -1 for above i.e. if(recLen == 0) return -1;
       * Note: this construction does not work with non blocking sockets.
       */
      return -1;
   }
   return recLen;
}
#endif


#endif /* NO_BSD_SOCK */
