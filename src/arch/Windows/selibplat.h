/**
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
 *   $Id: selibplat.h 4769 2021-06-11 17:29:36Z gianluca $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2017
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
 *               http://realtimelogic.com
 *               http://sharkssl.com
 ****************************************************************************
 *
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif


#define SharkSSLWindows
#ifndef HOST_PLATFORM
#define HOST_PLATFORM 1
#endif

#include <string.h>
#include <windows.h>
#include <winsock2.h>

#define WINFD_SET(sock,fd) FD_SET((u_int)sock, fd)

#ifdef SELIB_C

#include <ws2tcpip.h>

#ifndef NO_getaddrinfo
#define X_se_connect
int se_connect(int* sock, const char* address, U16 port)
{
   int retVal=-3;
   struct addrinfo* ptr;
   struct addrinfo* result = 0;
   if(getaddrinfo(address, 0, 0, &result))
      return -2;
   for(ptr=result ; ptr ; ptr=ptr->ai_next)
   {
      int sockfd;
      sockfd=socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
      if(sockfd < 0)
      {
         retVal=-1;
      }
      else
      {
         u_long nonBlock=1;
         if(ptr->ai_family == AF_INET)
            ((struct sockaddr_in*)ptr->ai_addr)->sin_port = htons(port);
         else if(ptr->ai_family == AF_INET6)
            ((struct sockaddr_in6*)ptr->ai_addr)->sin6_port = htons(port);
         else
            goto L_close;
         ioctlsocket(sockfd, FIONBIO, &nonBlock);
         if(connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) < 0)
         {
            if(WSAGetLastError() == WSAEWOULDBLOCK)
            {
               struct timeval tv;
               fd_set fds;
               FD_ZERO(&fds);
               WINFD_SET(sockfd, &fds);
               tv.tv_sec = 3;
               tv.tv_usec = 0;
               if(select(sockfd + 1, 0, &fds, 0, &tv)==1)
               {
                  struct sockaddr_in6 in6;
                  int size=sizeof(struct sockaddr_in6);
                  if(!getpeername(sockfd, (struct sockaddr*)&in6, &size))
                     goto L_ok;
               }
            }
           L_close:
            retVal=-3;
            closesocket(sockfd);
         }
         else
         {
           L_ok:
            nonBlock=0;
            ioctlsocket(sockfd, FIONBIO, &nonBlock);
            *sock=sockfd;
            retVal=0;
            break;
         }
      }
   }
   freeaddrinfo(result);
   return retVal;
}
#endif

#endif
