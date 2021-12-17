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
 *               http://realtimelogic.com
 *               http://sharkssl.com
 ****************************************************************************
 *
 */
#include <rtcs.h>

#if (!RTCSCFG_ENABLE_DNS)
#error Please recompile RTCS with RTCSCFG_ENABLE_DNS set to 1 in rtcscfg.h
#endif


#ifdef SELIB_C
#include <ctype.h>

#ifndef SOMAXCONN
#define SOMAXCONN 5
#endif

void se_close(int* sock);

#define X_readtmo
static int readtmo(int sock, U32 tmo)
{
   return RTCS_selectset(&sock, 1, tmo == (~((U32)0)) ? 0 : tmo) ? 0 : -1;
}

#define X_se_bind
int se_bind(int* sock, uint16_t port)
{
   struct sockaddr_in  addr;
   addr.sin_family = AF_INET;
   addr.sin_port = port;
   addr.sin_addr.s_addr = INADDR_ANY;
   if((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      return -1;
   }
   if(bind(*sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
   {
      se_close(sock);
      return -3;
   }
   if(listen(*sock, SOMAXCONN) < 0)
   {
      se_close(sock);
      return -2;
   }
   return 0;
}

#define X_se_connect
int se_connect(int* sock, const char* address, uint16_t port)
{
   unsigned int ip;
   struct sockaddr_in addr;
   in_addr  ipaddr;
   if( isdigit(address[0]) && inet_aton(address, &ipaddr) )
      ip = ipaddr.s_addr;
   else
   {
      HOSTENT_STRUCT_PTR hostInfo = DNS_gethostbyname((char*)address);
      if( ! hostInfo )
         return -2;
      ip = *(uint32_t*)hostInfo->h_addr_list[0];
   }
   memset((char*)&addr, 0, sizeof(sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_port = port;
   addr.sin_addr.s_addr = ip;
   if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == RTCS_SOCKET_ERROR)
   {
      return -1;
   }
   if(connect(*sock, (struct sockaddr*)&addr, sizeof(sockaddr_in)) == RTCS_OK)
   {
      return 0;
   }
   se_close(sock);
   return -3;
}

#define X_se_recv
S32 se_recv(int* sock, void* buf, U32 len, U32 timeout)
{
   int recLen;

   if(timeout == INFINITE_TMO)
   {
      timeout = 0;
   }

   recLen = setsockopt(*sock,SOL_TCP,OPT_RECEIVE_TIMEOUT,&timeout,sizeof(U32));
   recLen = recv(*sock,buf,len,0);

   if (recLen == RTCS_ERROR)
   {
      recLen = RTCS_geterror(*sock);
      if (RTCSERR_TCP_TIMED_OUT == recLen)
      {
         return 0;
      }

      return -1;
   }
   return recLen;
}

#define closesocket(s) shutdown(s,FLAG_CLOSE_TX)

#endif /* SELIB_C */

