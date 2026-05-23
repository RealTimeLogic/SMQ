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
 *   $Id: seLwIP.c 4963 2021-12-17 00:32:59Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2021
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


#include <selib.h>
#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/api.h>

#ifndef SharkSSLLwIP
#error SharkSSLLwIP not defined -> Using incorrect selibplat.h
#endif


#if LWIP_SO_RCVTIMEO != 1
#error LWIP_SO_RCVTIMEO must be set
#endif

#ifndef netconn_set_recvtimeout
#define OLD_LWIP
#define netconn_set_recvtimeout(conn, timeout) \
   ((conn)->recv_timeout = (timeout))
#endif




int se_accept(SOCKET** listenSock, U32 timeout, SOCKET** outSock)
{
   err_t err;
   memset(*outSock, 0, sizeof(SOCKET));
   netconn_set_recvtimeout(
      (*listenSock)->con, timeout == INFINITE_TMO ? 0 : timeout);
#ifdef OLD_LWIP
   (*outSock)->con = netconn_accept((*listenSock)->con);
   err = (*outSock)->con->err;
   if(!(*outSock)->con && !err) err = ERR_CONN;
#else
   err = netconn_accept((*listenSock)->con, &(*outSock)->con);
#endif
   if(err != ERR_OK)
   {
      return err == ERR_TIMEOUT ? 0 : -1;
   }
   return 1;
}


int se_bind(SOCKET* sock, U16 port)
{
   int err;
   memset(sock, 0, sizeof(SOCKET));
   sock->con = netconn_new(NETCONN_TCP);
   if( ! sock->con )
      return -1;
   if(netconn_bind(sock->con, IP_ADDR_ANY, port) == ERR_OK)
   {
      if(netconn_listen(sock->con) == ERR_OK)
         return 0;
      err = -2;
   }
   else
      err = -3;
   netconn_delete(sock->con);
   sock->con=0;
   return err;
}



/* Returns 0 on success.
   Error codes returned:
   -1: Cannot create socket: Fatal
   -2: Cannot resolve 'address'
   -3: Cannot connect
*/
int se_connect(SOCKET* sock, const char* name, U16 port)
{
#ifdef OLD_LWIP
   struct ip_addr addr;
#else
   ip_addr_t addr;
#endif
   memset(sock, 0, sizeof(SOCKET));
   if(netconn_gethostbyname(name, &addr) != ERR_OK)
      return -2;
   sock->con = netconn_new(NETCONN_TCP);
   if( ! sock->con )
      return -1;
   if(netconn_connect(sock->con, &addr, port) == ERR_OK)
      return 0;
   netconn_delete(sock->con);
   sock->con=0;
   return -3;
}



void se_close(SOCKET* sock)
{
   if(sock->con)
   {
      netconn_close(sock->con);
      netconn_delete(sock->con);
   }
   if(sock->nbuf)
      netbuf_delete(sock->nbuf);
   memset(sock, 0, sizeof(SOCKET));
}



S32 se_send(SOCKET* sock, const void* buf, U32 len)
{
   err_t err=netconn_write(sock->con, buf, len, NETCONN_COPY);
   if(err != ERR_OK)
   {
      se_close(sock);
	  return (S32)err;
   }
   return len;
}



S32 se_recv(SOCKET* sock, void* data, U32 len, U32 timeout)
{
   int rlen;
   netconn_set_recvtimeout(sock->con, timeout == INFINITE_TMO ? 0 : timeout);
   if( ! sock->nbuf )
   {
      err_t err;
      sock->pbOffs = 0;
#ifdef OLD_LWIP
      sock->nbuf = netconn_recv(sock->con);
      err = sock->con->err;
      if(!sock->nbuf && !err) err = ERR_CONN;
#else
      err = netconn_recv(sock->con, &sock->nbuf);
#endif
      if(ERR_OK != err)
      {
         if(sock->nbuf)
            netbuf_delete(sock->nbuf);
         sock->nbuf=0;
         return err == ERR_TIMEOUT ? 0 : (S32)err;
      }
   }
   rlen=(int)netbuf_copy_partial(sock->nbuf,(U8*)data,len,sock->pbOffs);
   if(!rlen)
      return -1;
   sock->pbOffs += rlen;
   if(sock->pbOffs >= netbuf_len(sock->nbuf))
   {
      netbuf_delete(sock->nbuf);
      sock->nbuf=0;
   }
   return rlen;
}



int se_sockValid(SOCKET* sock)
{
   return sock->con != 0;
}
