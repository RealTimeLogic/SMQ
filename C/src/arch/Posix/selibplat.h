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
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2019
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


#define SharkSSLPosix
#define HOST_PLATFORM 1

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>

#ifdef __CYGWIN__
#define __linux__ 1
#endif

#ifdef SELIB_C
#ifndef __CYGWIN__
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
         int nonBlock=1;
         if(ptr->ai_family == AF_INET)
            ((struct sockaddr_in*)ptr->ai_addr)->sin_port = htons(port);
         else if(ptr->ai_family == AF_INET6)
            ((struct sockaddr_in6*)ptr->ai_addr)->sin6_port = htons(port);
         else
            goto L_close;
         ioctl(sockfd, FIONBIO, &nonBlock);
         if(connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) < 0)
         {
            if(errno == EINPROGRESS)
            {
               struct timeval tv;
               fd_set fds;
               FD_ZERO(&fds);
               FD_SET(sockfd, &fds);
               tv.tv_sec = 4;
               tv.tv_usec = 0;
               if(select(sockfd + 1, 0, &fds, 0, &tv)==1)
               {
                  struct sockaddr_in6 in6;
                  socklen_t size=sizeof(struct sockaddr_in6);
                  if(!getpeername(sockfd, (struct sockaddr*)&in6, &size))
                     goto L_ok;
               }
            }
           L_close:
            retVal=-3;
            close(sockfd);
         }
         else
         {
           L_ok:
            nonBlock=0;
            ioctl(sockfd, FIONBIO, &nonBlock);
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
