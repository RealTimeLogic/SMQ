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
 *   $Id: seLwIP.c 4769 2021-06-11 17:29:36Z gianluca $
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
#include <lwip/sys.h>
#include <lwip/dns.h>

#ifndef SharkSSLlwIPRaw
#error SharkSSLlwIPRaw not defined -> Using incorrect selibplat.h
#endif

/* dns_gethostbyname callback
 */
static void
se_DnsCb(const char *name, struct ip_addr *ipaddr, void* s)
{
   SOCKET* sock = (SOCKET*)s;
   (void)name;
   if ((ipaddr) && (ipaddr->addr))
      sock->u.ipaddr = *ipaddr;
   else
      sock->ecode = -2; /* Cannot resolve 'address' */
   sock->ctx->ready=TRUE;
}


/* tcp_connect callback
 */
static err_t
se_connectCb(void* s, struct tcp_pcb * pcb, err_t err)
{
   SOCKET* sock = (SOCKET*)s;
   (void)pcb;
   sock->ecode = err ? -3 : 0; /* -3: Cannot connect */
   sock->ctx->ready = TRUE;
   return 0;
}


static void
se_errCb(void* s, err_t err)
{
   SOCKET* sock = (SOCKET*)s;
   se_close(sock);
   sock->ecode = err;
   sock->ctx->ready = TRUE;
}


static err_t
se_sentCb(void* s, struct tcp_pcb * pcb, u16_t len)
{
   SOCKET* sock = (SOCKET*)s;
   (void)pcb;
   (void)len;
   sock->outLen -= len;
   if(sock->flags & SOCKFLAG_inSend)
      sock->ctx->ready = TRUE;
   return 0;
}


static err_t
se_recCb(void* s, struct tcp_pcb* tpcb, struct pbuf* p, err_t err)
{
   SOCKET* sock = (SOCKET*)s;
   (void)tpcb;
   if(!p || err)
   {
      se_close(sock);
      sock->ecode = err ? err : ERR_CLSD;
   }
   else
   {
      pbuf_ref(p);
      if(sock->u.rec.buf)
         pbuf_cat(sock->u.rec.buf, p);
      else
      {
         sock->u.rec.cur = sock->u.rec.buf = p;
         sock->u.rec.len = p->len;
         sock->u.rec.data = p->payload;
      }
   }
   if((sock->flags & SOCKFLAG_inRecv) && (sock->u.rec.buf || sock->ecode))
      sock->ctx->ready = TRUE;
   return 0;
}


static void
se_setCB(SOCKET* sock)
{
   sock->pcb->callback_arg=sock;
   tcp_err(sock->pcb, se_errCb);
   tcp_sent(sock->pcb, se_sentCb);
   tcp_recv(sock->pcb, se_recCb);
}


static err_t
se_acceptCB(void* s, struct tcp_pcb* newPcb, err_t err)
{
   SOCKET* lsock = (SOCKET*)s;
   if(newPcb)
   {
      tcp_accepted(lsock->pcb);
      if( ! lsock->u.accept.outSock ) /* not blocking in func se_accept */
      {
         SOCCONT* sc = (SOCCONT*)mem_malloc(sizeof(SOCCONT));
         if( ! sc )
            return 1; /* reject */
         memset(sc,0,sizeof(SOCCONT));
         sc->sock.pcb = newPcb;
         sc->sock.ctx = lsock->ctx;
         se_setCB(&sc->sock);
         if(lsock->u.accept.pending)
         {
            int ix=0;
            SOCCONT* iter = lsock->u.accept.pending;
            while(iter->next)
            {
               if(++ix > 4)
               {
                  mem_free(sc);
                  return 1; /* full */
               }
               iter = iter->next;
            }
            iter->next = sc;
         }
         else
            lsock->u.accept.pending = sc;
         return 0;
      }
      lsock->u.accept.outSock->pcb = newPcb;
      if( ! lsock->u.accept.outSock->ctx )
         lsock->u.accept.outSock->ctx = lsock->ctx;
      se_setCB(lsock->u.accept.outSock);
      lsock->u.accept.outSock = 0;
   }
   else
      lsock->ecode = err ? err : -1;
   lsock->ctx->ready = TRUE;
   return 0;
}


static int
se_getPendingAccept(SOCKET* lsock, SOCKET* outSock)
{
   int ok;
   SOCCONT* sc = lsock->u.accept.pending;
   if(sc->sock.pcb)
   {
      outSock->pcb = sc->sock.pcb;
      outSock->pcb->callback_arg=outSock;
      if(sc->sock.u.rec.buf)
      {
         struct pbuf* p = sc->sock.u.rec.buf;
         outSock->u.rec.cur = outSock->u.rec.buf = p;
         outSock->u.rec.len = p->len;
         outSock->u.rec.data = p->payload;
      }
      if( ! outSock->ctx )
         outSock->ctx = lsock->ctx;
      ok = TRUE;
   }
   else
      ok = FALSE;
   lsock->u.accept.pending = sc->next;
   mem_free(sc);
   return ok;
}


int
se_accept(SOCKET** listenSock, U32 timeout, SOCKET** outSock)
{
   SOCKET* lsock = *listenSock;
   if( ! lsock->pcb || lsock->ecode || (*outSock)->pcb)
      return -1;
  L_tryAgain:
   if(lsock->u.accept.pending)
   {
      if(se_getPendingAccept(lsock, *outSock))
         return 1;
      goto L_tryAgain;
   }
   if(timeout != INFINITE_TMO)
      lsock->ctx->timeout = timeout;
   lsock->u.accept.outSock = *outSock;
   SeCtx_save(lsock->ctx);
   lsock->ctx->startTime = lsock->ctx->timeout = 0;
   if(lsock->ecode)
   {
      int ecode = lsock->ecode;
      lsock->ecode = 0;
      return ecode;
   }
   return 1;
}


int
se_bind(SOCKET* sock, U16 port)
{
   if( sock->pcb || sock->ecode || (sock->pcb = tcp_new()) == 0 ) return -1;
   sock->ecode = tcp_bind(sock->pcb, IP_ADDR_ANY, port) ? -3 : 0;
   if( ! sock->ecode )
   {
      struct tcp_pcb * pcb = tcp_listen(sock->pcb);
      if(pcb)
      {
         sock->pcb = pcb;
         pcb->callback_arg=sock;
         tcp_accept(pcb, se_acceptCB);
         return 0;
      }
      sock->ecode = -2;
   }
   se_close(sock);
   return sock->ecode;
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
   ip_addr_t ipaddr;
   switch(dns_gethostbyname(name, &sock->u.ipaddr, se_DnsCb, sock))
   {
      case ERR_INPROGRESS:
         sock->u.rec.buf=0;
         SeCtx_save(sock->ctx);
         if(sock->ecode)
         {
            sock->u.rec.buf=0;
            return sock->ecode;
         }
         /* fall through */
      case ERR_OK:
         break;
      default:
         sock->u.rec.buf=0;
         return -2;
   }
   ipaddr = sock->u.ipaddr;
   sock->u.rec.buf=0;
   if( sock->pcb || (sock->pcb = tcp_new()) == 0 )
      return -1;
   se_setCB(sock);
   if(tcp_connect(sock->pcb, &ipaddr, port, se_connectCb))
      sock->ecode=1;
   else
      SeCtx_save(sock->ctx);
   if(sock->ecode)
   {
      se_close(sock);
      return -3;
   }
   return 0;
}



void
se_close(SOCKET* sock)
{
   SeCtx* ctx;
   struct tcp_pcb* pcb = sock->pcb;
   if(pcb)
   {
      if(pcb->state == LISTEN)
      {
         SOCCONT* sc = sock->u.accept.pending;
         while(sc)
         {
            SOCCONT* x = sc;
            sc = sc->next;
            se_close(&x->sock);
            mem_free(x);
         }
      }
      else if(sock->u.rec.buf)
      {
         struct pbuf* p = sock->u.rec.buf;
         while(p)
         {
            if(p->ref > 1) /* pbuf_ref in se_recCb */
               pbuf_free(p); /* Decrement */
            p = p->next;
         }
         tcp_recved(pcb, sock->u.rec.buf->tot_len);
         pbuf_free(sock->u.rec.buf); 
      }
      pcb->errf = 0;
      pcb->sent = 0;
      pcb->recv = 0;
      tcp_close(pcb);
   }
   ctx = sock->ctx;
   memset(sock, 0, sizeof(SOCKET));
   sock->ctx = ctx;
}



S32
se_send(SOCKET* sock, const void* buf, U32 len)
{
   U16 left;
   struct tcp_pcb *pcb = sock->pcb;
   if( len > 0xFFFF || ! pcb || sock->ecode)
      return sock->ecode ? sock->ecode : -1;
   sock->outLen += (S32)len;
   left = tcp_sndbuf(pcb);
   if(len <= 1400 && left > len)
   {
      sock->ecode=tcp_write(pcb,buf,(U16)len,TCP_WRITE_FLAG_COPY);
   }
   else
   {
      U16 chunk;
      U8* ptr = (U8*)buf;
      U16 plen = (U16)len;
      while( ! sock->ecode )
      {
         chunk = plen > left ? left : plen;
         if( (sock->ecode = tcp_write(pcb,ptr,chunk,0)) != 0)
            return sock->ecode;
         plen -= chunk;
         ptr += chunk;
         if(plen)
         {
            sock->flags |= SOCKFLAG_inSend;
            SeCtx_save(sock->ctx);
            sock->flags &= ~(U8)SOCKFLAG_inSend;
         }
         else
            break;
      }
      while(sock->outLen > 0 && !sock->ecode)
      {
         sock->flags |= SOCKFLAG_inSend;
         SeCtx_save(sock->ctx);
         sock->flags &= ~(U8)SOCKFLAG_inSend;
      }
      sock->outLen=0;
   }
   return sock->ecode ? sock->ecode : (S32)len;
}



S32
se_recv(SOCKET* sock, void* buf, U32 len, U32 timeout)
{
   U8* bPtr;
   size_t recLen=0;
   if( ! sock->pcb || sock->ecode ) return -1;
   if( ! sock->u.rec.buf )
   {
      if(timeout != INFINITE_TMO)
         sock->ctx->timeout = timeout;
      sock->flags |= SOCKFLAG_inRecv;
      SeCtx_save(sock->ctx);
      sock->flags &= ~(U8)SOCKFLAG_inRecv;
      sock->ctx->startTime = sock->ctx->timeout = 0;
      if( ! sock->u.rec.buf )
         return sock->ecode; /* no error = 0 = timeout code */
   }
   bPtr = (U8*)buf;
   while(len)
   {
      size_t chunk = len > sock->u.rec.len ? sock->u.rec.len : len;
      memcpy(bPtr, sock->u.rec.data, chunk);
      sock->u.rec.len -= chunk;
      len -= chunk;
      recLen += chunk;
      bPtr += chunk;
      if(sock->u.rec.len == 0)
      {
         if(sock->u.rec.cur->ref > 1)
            pbuf_free(sock->u.rec.cur);
         sock->u.rec.cur = sock->u.rec.cur->next;
         if( ! sock->u.rec.cur )
         {
            tcp_recved(sock->pcb, sock->u.rec.buf->tot_len);
            pbuf_free(sock->u.rec.buf);
            sock->u.rec.buf=0;
            break;
         }
         sock->u.rec.len = sock->u.rec.cur->len;
         sock->u.rec.data = sock->u.rec.cur->payload;
      }
      else
      {
         baAssert(len == 0);
         sock->u.rec.data += chunk;
      }
   }
   return recLen;
}


int
se_sockValid(SOCKET* sock)
{
   return sock->pcb != 0;
}


int
SeCtx_run(SeCtx* ctx)
{
   auto U8 stackMark;
   if(ctx->hasContext) /* If function mainTask is running */
   {
      if(ctx->ready) /* If lwIP called an event function callback */
      {
         SeCtx_restore(ctx);  /* Jump to code just after SeCtx_save */
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
