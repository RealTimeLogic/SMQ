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
 *   COPYRIGHT:  Real Time Logic LLC, 2018
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

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#if !defined(B_LITTLE_ENDIAN) || defined(B_BIG_ENDIAN)
#if ipconfigBYTE_ORDER == pdFREERTOS_BIG_ENDIAN
#define B_BIG_ENDIAN
#else
#define B_LITTLE_ENDIAN
#endif
#endif

#define NO_BSD_SOCK

#define SOCKET Socket_t


#ifdef SELIB_C

int se_sockValid(SOCKET* sock)
{
   return *sock != FREERTOS_INVALID_SOCKET;
}


void se_close(SOCKET* sock)
{
   TickType_t start_time;
   U8 buffer[10];

   if(*sock != FREERTOS_INVALID_SOCKET)
   {
      FreeRTOS_shutdown(*sock, FREERTOS_SHUT_RDWR);
      start_time = xTaskGetTickCount();

      do
      {
         int rc = FreeRTOS_recv(*sock, buffer, sizeof(buffer), 0);
         if(rc < 0 && rc != -pdFREERTOS_ERRNO_EAGAIN)
         {
            break;
         }
      } while((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(1000));

      FreeRTOS_closesocket(*sock);
      *sock=FREERTOS_INVALID_SOCKET;
   }
}

int se_bind(SOCKET* sock, uint16_t port)
{
   struct freertos_sockaddr addr;
   socklen_t addrlen=sizeof(addr);
   *sock = FreeRTOS_socket(
      FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
   if(*sock == FREERTOS_INVALID_SOCKET)
   {
      return -1;
   }
   addr.sin_addr=INADDR_ANY;
   addr.sin_port = FreeRTOS_htons(port);
   if(FreeRTOS_bind(*sock, &addr, addrlen))
   {
      se_close(sock);
      return -3;
   }
   if(FreeRTOS_listen(*sock, SOMAXCONN))
   {
      se_close(sock);
      return -2;
   }
   return 0;
}

int se_connect(SOCKET* sock, const char* address, uint16_t port)
{
   struct freertos_sockaddr addr;
   socklen_t addrlen=sizeof(addr);
   uint32_t ip = FreeRTOS_gethostbyname(address);
   if( ! ip )
   {
      ip = FreeRTOS_inet_addr(address);
      if( ! ip )
      {
         *sock = FREERTOS_INVALID_SOCKET;
         return -2;
      }
   }
   *sock = FreeRTOS_socket(
      FREERTOS_AF_INET,FREERTOS_SOCK_STREAM,FREERTOS_IPPROTO_TCP);
   if( ! *sock )
   {
      return -1;
   }
   if(*sock == FREERTOS_INVALID_SOCKET)
   {
      return -1;
   }
   addr.sin_addr=ip;
   addr.sin_port = FreeRTOS_htons(port);
   if(FreeRTOS_connect(*sock, &addr, sizeof(addr)))
   {
      se_close(sock);
      return -3;
   }
   return 0;
}


int se_accept(SOCKET** listenSock, U32 timeout, SOCKET** outSock)
{
   struct freertos_sockaddr addr;
   socklen_t addrlen=sizeof(addr);
   TickType_t tickTmo;
   tickTmo = timeout==INFINITE_TMO ? portMAX_DELAY : timeout/portTICK_PERIOD_MS;
   FreeRTOS_setsockopt(**listenSock,0,FREERTOS_SO_RCVTIMEO,&tickTmo,0); 
   **outSock=FreeRTOS_accept(**listenSock,&addr,&addrlen);
   return **outSock == 0 ? 0 : (**outSock < 0 ? -1 : 1);
}


S32 se_recv(SOCKET* sock, void* buf, U32 len, U32 timeout)
{
   TickType_t tickTmo;
   tickTmo = timeout==INFINITE_TMO ? portMAX_DELAY : timeout/portTICK_PERIOD_MS;
   FreeRTOS_setsockopt(*sock,0,FREERTOS_SO_RCVTIMEO,&tickTmo,0); 
   return FreeRTOS_recv(*sock,buf,len,0);
}

S32 se_send(SOCKET* sock, const void* buf, U32 len)
{
   return FreeRTOS_send(*sock, buf, len, 0);
}

#endif /* SELIB_C */

