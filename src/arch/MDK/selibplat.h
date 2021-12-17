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
 *   COPYRIGHT:  Real Time Logic LLC, 2016
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


#define SharkSSLMDK
#include "rl_net.h"

#if !defined(B_BIG_ENDIAN) && !defined(B_LITTLE_ENDIAN)
#ifdef __ARM_BIG_ENDIAN
#define B_BIG_ENDIAN
#else
#define B_LITTLE_ENDIAN
#endif
#endif

#ifndef baAssert
#ifndef NDEBUG
#define baAssert_func
#define baAssert(x)        ((x) ? 0 : sharkAssert(__FILE__, __LINE__))
#ifdef __cplusplus
extern "C" {
#endif
int sharkAssert(const char* file, int line);
#ifdef __cplusplus
}
#endif
#else
#define baAssert(x)
#endif
#endif

#ifdef SELIB_C

#ifdef baAssert_func
const char* assert_file;
int assert_line;
int sharkAssert(const char* file, int line)
{
   assert_file = file;
   assert_line = line;
   for(;;);
}
#endif

#define X_readtmo
#define X_se_connect
#define X_se_recv
#define SOMAXCONN 3
#define closesocket closesocket

/* Not implemented for accept */
static int readtmo(int sock, U32 tmo)
{
   return 0;
}


int se_connect(int* sock, const char* address, U16 port)
{
   int err;
   HOSTENT* host = gethostbyname (address, &err);
   if(host && host->h_addrtype == AF_INET) /* only IPv4 for now */
   {
      int i;
      *sock = socket (AF_INET, SOCK_STREAM, 0);
      if(*sock)
      {
         for (i = 0; host->h_addr_list[i]; i++)
         {
            SOCKADDR_IN addr;
            IN_ADDR* ia = (IN_ADDR *)host->h_addr_list[i];
            addr.sin_port = htons(port);
            addr.sin_family = PF_INET;
            addr.sin_addr.s_b1 = ia->s_b1;
            addr.sin_addr.s_b2 = ia->s_b2;
            addr.sin_addr.s_b3 = ia->s_b3;
            addr.sin_addr.s_b4 = ia->s_b4;
            if(BSD_SUCCESS == connect(*sock, (SOCKADDR *)&addr, sizeof (addr)))
            {
               /* Required for each persistent connection in this TCP/IP stack.
                  The connection goes down after 120 seconds otherwise.
               */
               char optval = 1;
               setsockopt(*sock,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
               return 0;
            }
         }
         closesocket(*sock);
         return -3;
      }
      return -1;
   }
   return -2;
}


S32 se_recv(int* sock, void* buf, U32 len, U32 tmo)
{
   int recLen;
   uint32_t t = tmo;
   if(setsockopt(*sock,SOL_SOCKET,SO_RCVTIMEO,(const char *)&t, sizeof(t)) < 0)
      return -10;
  L_again:
   recLen = recv(*sock,buf,len,0);
   if (recLen <= 0)
   {
      if(recLen == BSD_ERROR_TIMEOUT)
      {
         if(tmo == INFINITE_TMO)
            goto L_again;
         return 0;
      }
      return -1;
   }
   return recLen;
}





#endif
