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
 *   $Id: seHarmony.c 4029 2017-03-14 21:19:09Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2017
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
 */


#include "../../selib.h"

#ifndef SharkSSLHarmony
#error SharkSSLHarmony not defined -> Using incorrect selibplat.h
#endif


U32
millisec2Ticks(U32 time)
{
   return (time * SYS_TMR_TickCounterFrequencyGet()) / 1000;
}

   
/* Returns 0 on success.
   Error codes returned:
   -1: Cannot create socket: Fatal
   -2: Cannot resolve 'address'
   -3: Cannot connect
*/
int
se_connect(SOCKET* sock, const char* name, U16 port)
{

   IP_MULTI_ADDRESS addr;
   TCPIP_DNS_RESULT result = TCPIP_DNS_Resolve(name, TCPIP_DNS_TYPE_A);
   if(TCPIP_DNS_RES_PENDING == result)
   {
      sock->ctx->state = SELIB_DNS_PENDING;
      for(;;)
      {
         sock->ctx->htcp = sock->htcp;
         SeCtx_save(sock->ctx);
        L_fetchIp:
         result = TCPIP_DNS_IsNameResolved(name, &addr.v4Add, 0);
         if(TCPIP_DNS_RES_OK == result)
            break;
         if(TCPIP_DNS_RES_PENDING != result)
            return -2;
      }
   }
   else if(TCPIP_DNS_RES_NAME_IS_IPADDRESS == result)
   {
      if( ! TCPIP_Helper_StringToIPAddress(name, &addr.v4Add) )
         return -2;
   }
   else if(TCPIP_DNS_RES_OK == result)
      goto L_fetchIp;
   else
      return -2;
   sock->htcp = TCPIP_TCP_ClientOpen(IP_ADDRESS_TYPE_IPV4, port, &addr);
   if(INVALID_SOCKET == sock->htcp)
      return -1;
   sock->ctx->state = SELIB_CONNECT_PENDING;
   sock->ctx->timeout = millisec2Ticks(5000); /* hardcoded 5 secs */
   sock->ctx->htcp = sock->htcp;
   SeCtx_save(sock->ctx);
   if(TCPIP_TCP_IsConnected(sock->htcp))
      return 0;
   TCPIP_TCP_Abort(sock->htcp, TRUE);
   sock->htcp = INVALID_SOCKET;
   return -3;
}



void
se_close(SOCKET* sock)
{
   if(INVALID_SOCKET != sock->htcp)
   {
      TCPIP_TCP_Close(sock->htcp);
      sock->htcp = INVALID_SOCKET;
   }
}


S32
se_send(SOCKET* sock, const void* buf, U32 len)
{
   U32 ix = len;
   uint8_t* ptr = (uint8_t*)buf;
   sock->ctx->state = SELIB_SEND_PENDING;
   for(;;)
   {
      U32 max = (U32)TCPIP_TCP_PutIsReady(sock->htcp);
      if(max > 0)
      {
         if(max > ix)
            max = ix;
         if(max!=(U32)TCPIP_TCP_ArrayPut(sock->htcp,ptr,(uint16_t)max))
            return -1;
         ix = ix - max;
         if(ix == 0)
            return (S32)len;
         ptr+=max;
      }
      sock->ctx->htcp = sock->htcp;
      sock->ctx->timeout = millisec2Ticks(30000); /* hardcoded 30 secs */
      SeCtx_save(sock->ctx);
      if(! TCPIP_TCP_IsConnected(sock->htcp) )
         return -1;
      if( ! TCPIP_TCP_PutIsReady(sock->ctx->htcp) )
         return -2;
   }
}


S32
se_recv(SOCKET* sock, void* buf, U32 len, U32 timeout)
{
   uint16_t rlen;
   int i;
   sock->ctx->state = SELIB_RECV;
   if(timeout != INFINITE_TMO)
      sock->ctx->timeout = millisec2Ticks(timeout);
   for(i = 0 ; 0 == (rlen = TCPIP_TCP_GetIsReady(sock->htcp)) ; i++)
   {
      if( ! TCPIP_TCP_IsConnected(sock->htcp) )
         return -1;
      if(i > 0) /* timeout */
      {
         baAssert(timeout != INFINITE_TMO);
         return 0;
      }
      sock->ctx->htcp = sock->htcp;
      SeCtx_save(sock->ctx);
   }
   if((U32)rlen > len)
      rlen = (uint16_t)len;
   return (S32)TCPIP_TCP_ArrayGet(sock->htcp, (uint8_t*)buf, rlen);
}


int
se_accept(SOCKET** listenSock, U32 timeout, SOCKET** outSock)
{
   (void)listenSock;
   (void)timeout;
   (void)outSock;
   baAssert(0); /* not implemented */
   return -1;
}


int
se_bind(SOCKET* sock, U16 port)
{
   (void)sock;
   (void)port;
   baAssert(0); /* not implemented */
   return -1;
}


int
se_sockValid(SOCKET* sock)
{
   return TCPIP_TCP_IsConnected(sock->htcp);
}


int
SeCtx_run(SeCtx* ctx)
{
   auto U8 stackMark;
   if(ctx->hasContext) /* If function mainTask is running */
   {
      switch(ctx->state)
      {
         case SELIB_DNS_PENDING:
            SeCtx_restore(ctx);
            break;
         case SELIB_CONNECT_PENDING:
            if(TCPIP_TCP_IsConnected(ctx->htcp))
            {
               ctx->startTime = ctx->timeout = 0;
               SeCtx_restore(ctx);
            }
            break;

         case SELIB_SEND_PENDING:
            if(TCPIP_TCP_PutIsReady(ctx->htcp) > 0 || 
               ! TCPIP_TCP_IsConnected(ctx->htcp))
            {
               ctx->startTime = ctx->timeout = 0;
               SeCtx_restore(ctx);
            }
            break;

         case SELIB_RECV:
            if(TCPIP_TCP_GetIsReady(ctx->htcp) > 0 || 
               ! TCPIP_TCP_IsConnected(ctx->htcp))
            {
               ctx->startTime = ctx->timeout = 0;
               SeCtx_restore(ctx);
            }
            break;

         default:
            baAssert(0);            
      }
      if(ctx->timeout) /* Timeout on se_connect or se_recv */
      {
         if( ! ctx->startTime )
         {
            ctx->startTime = SYS_TMR_TickCountGet(); /* Start timer */
         }
         else if( (S32)(SYS_TMR_TickCountGet() - ctx->startTime) >=
                  (S32)ctx->timeout )
         {  /* timeout */
            ctx->startTime = ctx->timeout = 0;
            SeCtx_restore(ctx);
         }
      }
   }
   else if( ! SeCtx_setStackTop(ctx, &stackMark) )
   {
      ctx->state = SELIB_INVALID;
      ctx->task(ctx);
      baAssert( ! ctx->hasContext );
      return -1;
   }
   return 0;
}
