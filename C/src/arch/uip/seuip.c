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
 *   $Id: seuip.c 4769 2021-06-11 17:29:36Z gianluca $
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
 */


#include "../../selib.h"

#ifndef SharkSSLuIP
#error SharkSSLuIP not defined -> Using incorrect selibplat.h
#endif

#include <uip.h>
#include <uiplib.h>
#if 0
#include <resolv.h>
#endif

static SOCKET* resolvSock;
   
void se_resolv_found(char *name, u16_t *ipaddr)
{
   if(resolvSock)
   {
      if(ipaddr)
      {
         uip_ipaddr_copy(resolvSock->u.resolv.ipaddr, ipaddr);
         resolvSock->u.resolv.OK = TRUE;
      }
      else
         resolvSock->u.resolv.OK = FALSE;
      resolvSock->ctx->ready = TRUE;
      resolvSock=0;
   }
}

static void se_sendChunk(SOCKET* sock)
{
   int chunk;
   if(uip_acked() && sock->u.data.buf)
   {
      chunk = sock->u.data.size > uip_mss() ? uip_mss() : sock->u.data.size;
      baAssert(sock->u.data.size > 0);
      sock->u.data.size -= chunk;
      sock->u.data.buf += chunk;
   }
   if(sock->u.data.size)
   {
      baAssert(sock->u.data.size > 0 && sock->u.data.buf);
      if(((sock->flags & SOCKFLAG_startSend) && uip_poll())
         || uip_rexmit() || uip_acked())
      {
         chunk=sock->u.data.size > uip_mss() ? uip_mss() : sock->u.data.size;
         sock->flags &= ~(U8)SOCKFLAG_startSend;
         uip_send(sock->u.data.buf, chunk);
      }
   }
   else
   {
      sock->flags &= ~(U8)SOCKFLAG_inSend;
      sock->u.data.buf = 0;
      sock->ctx->ready = TRUE;
   }
}



int
se_appcall(void)
{
   SOCKET* sock;
   if(GET_SOCKET()->conn != uip_conn)
      return -1;
   sock = GET_SOCKET();
   if(uip_newdata() && uip_datalen())
   {
      if( (sock->flags & SOCKFLAG_inRecv) &&
          ! (sock->flags & SOCKFLAG_newdata)
          && uip_datalen() <= sock->u.data.size &&
          ! sock->msg )
      {
         sock->flags |= SOCKFLAG_newdata;
         sock->u.data.size = uip_datalen();
         memcpy(sock->u.data.buf, uip_appdata, sock->u.data.size);
      }
      else
      {
         SOCKETMSG* msg = baMalloc(sizeof(SOCKETMSG)+uip_datalen());
         if( ! msg )
         {
            sock->ctx->ready = TRUE;
            se_close(sock);
            return 0;
         }
         msg->size = uip_datalen();
         msg->next = 0;
         memcpy(msg+1, uip_appdata, msg->size);
         if(sock->msg)
         {
            SOCKETMSG* iter = sock->msg;
            while(iter->next)
               iter = iter->next;
            iter->next = msg;
         }
         else
            sock->msg = msg;
      }
      if(sock->flags & SOCKFLAG_inRecv)
         sock->ctx->ready = TRUE;
   }
   if(uip_closed() || uip_aborted() || uip_timedout())
   {
      se_close(sock);
      sock->ctx->ready = TRUE;
   }
   else if(uip_connected() && sock->flags & SOCKFLAG_inConnect)
   {
      sock->ctx->ready = TRUE;
   }
   else if(sock->flags & SOCKFLAG_inSend)
   {
      se_sendChunk(sock);
   }
   return 0;
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
   uip_ipaddr_t addr;
   uip_ipaddr_t* ipaddr = &addr;
   if(sock->conn)
      return -1;
   if( ! uiplib_ipaddrconv((char*)name, (unsigned char *)ipaddr) ||
       (ipaddr = (uip_ipaddr_t *)resolv_lookup((char*)name)) == 0)
   {
      resolvSock = sock;
      resolv_query((char*)name);
      SeCtx_save(sock->ctx);
      resolvSock=0;
      if( ! sock->u.resolv.OK )
         return -2;
   }
   sock->conn = uip_connect(&sock->u.resolv.ipaddr,HTONS(port));
   if( ! sock->conn )
      return -1;
   SET_SOCKET(sock->conn, sock);
   sock->flags |= SOCKFLAG_inConnect;
   SeCtx_save(sock->ctx);
   sock->flags &= ~(U8)SOCKFLAG_inConnect;
   return sock->conn ? 0 : -3;
}



void
se_close(SOCKET* sock)
{
   if(sock->conn == uip_conn)
   {
      SOCKETMSG* iter = sock->msg;
      SeCtx* ctx = sock->ctx;
      while(iter)
      {
         SOCKETMSG* next = iter->next;
         baFree(iter);
         iter = next;
      }
      uip_close();

      memset(sock, 0, sizeof(SOCKET));
      sock->ctx = ctx;
   }
}

S32
se_send(SOCKET* sock, const void* buf, U32 len)
{
   if( ! sock->conn ) return -1;
   if(sock->flags & SOCKFLAG_inRecv)
      return -2;
   if(len > 0)
   {
      sock->u.data.size = (int)len;
      sock->u.data.buf = (U8*)buf;
      sock->flags |= (U8)SOCKFLAG_inSend;
      sock->flags |= (U8)SOCKFLAG_startSend;
      se_sendChunk(sock);
      SeCtx_save(sock->ctx);
      if(!sock->conn)
         return -1;
   }
   return (S32)len;
}


static S32
se_getMsg(SOCKET* sock, void* buf, int len)
{
   int chunk;
   baAssert(sock->msg);
   if( ! sock->u.data.buf )
   {
      sock->u.data.buf = (U8*)(sock->msg+1);
      sock->u.data.size = sock->msg->size;
   }
   chunk = len > sock->u.data.size ? sock->u.data.size : len;
   memcpy(buf, sock->u.data.buf, chunk);
   sock->u.data.size -= chunk;
   if(sock->u.data.size <= 0)
   {
      SOCKETMSG* next = sock->msg->next;
      sock->u.data.buf = 0;
      baFree(sock->msg);
      sock->msg = next;
   }
   else
      sock->u.data.buf += chunk;
   return (S32)chunk;
}



S32
se_recv(SOCKET* sock, void* buf, U32 len, U32 timeout)
{
   if(sock->msg)
      return se_getMsg(sock, buf, (int)len);
   if( ! sock->conn ) return -1;
   sock->flags |= (U8)SOCKFLAG_inRecv;
   if(timeout != INFINITE_TMO)
      sock->ctx->timeout = timeout;
   sock->u.data.buf = buf;
   sock->u.data.size = (int)len;
   SeCtx_save(sock->ctx);
   sock->ctx->startTime = sock->ctx->timeout = 0;
   sock->u.data.buf = 0;
   sock->flags &= ~(U8)SOCKFLAG_inRecv;
   if(sock->flags & SOCKFLAG_newdata)
   {
      sock->flags &= ~(U8)SOCKFLAG_newdata;
      return sock->u.data.size;
   }
   if(sock->msg)
      return se_getMsg(sock, buf, (int)len);
   return sock-> conn ? 0 : -1; /* timeout if conn still valid */
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
   return sock->conn != 0;
}


int
SeCtx_run(SeCtx* ctx)
{
   auto U8 stackMark;
   if( ! uip_hostaddr[0] )
      return 0;
   if(ctx->hasContext) /* If function mainTask is running */
   {
      if(ctx->ready) /* If uIP called se_appcall */
      {
         SeCtx_restore(ctx); /* Jump to code just after SeCtx_save */
      }
      else if(ctx->timeout) /* Timeout on se_recv activated */
      {
         if( ! ctx->startTime )
         {
            ctx->startTime = sys_now(); /* Start timer */
         }
         else if( (S32)(sys_now() - ctx->startTime) >= (S32)ctx->timeout )
         {  /* timeout */
            ctx->startTime = ctx->timeout = 0;
            SeCtx_restore(ctx); /*  Jump to code in seLwIP.c */
         }
      }
   }
   else if( ! SeCtx_setStackTop(ctx, &stackMark) )
   {
      ctx->task(ctx);
      return -1;
   }
   return 0;
}
