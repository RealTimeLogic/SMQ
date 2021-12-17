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
 *   $Id: seNetX.c 4769 2021-06-11 17:29:36Z gianluca $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2013 - 2014
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

#ifndef SharkSSLNetX
#error SharkSSLNetX not defined -> Using incorrect selibplat.h
#endif

#include "nx_api.h"

static NX_IP* nxip;
static NX_DNS* nxdns;

void seNetX_init(NX_IP* ip, NX_DNS* dns)
{
   nxip = ip;
   nxdns = dns;
}

/* Verify Response: reboot if not OK */
#define VRSP(x) do { int s=x; if(s) time2Reboot(s); } while(0)

/* Redesign this function and make it reboot your system */
static void time2Reboot(int status)
{
   xprintf(("Err %d\n",status));
   for(;;)
   {
   }
}


static U32 txTime2Ticks(U32 msec)
{
   U32 sec;
   /* Make sure msec does not wrap on 32 bit.
    */
   if(msec < 4294967)
      return msec * TX_TIMER_TICKS_PER_SECOND / 1000;
   sec = msec / 1000;
   msec = msec % 1000;
   return sec * TX_TIMER_TICKS_PER_SECOND +
      msec * TX_TIMER_TICKS_PER_SECOND / 1000;
}


int se_accept(SOCKET** listenSock, U32 timeout, SOCKET** outSock)
{
   ULONG waitOption;
   int status,ns;
   NX_TCP_SOCKET* msock = &(*listenSock)->nxSock;
   NX_TCP_SOCKET* newsock = &(*outSock)->nxSock;
   for(;;)
   {
      waitOption = INFINITE_TMO == timeout ?
         NX_WAIT_FOREVER : txTime2Ticks(timeout);
      status = nx_tcp_server_socket_accept(msock, waitOption);
      if( ! status )
      {
         memset(*outSock,0, sizeof(SOCKET));
         status = nx_tcp_socket_create(
            msock->nx_tcp_socket_ip_ptr,
            newsock,
            "selib",
            NX_IP_NORMAL,
            NX_FRAGMENT_OKAY,
            NX_IP_TIME_TO_LIVE,
            2048,
            NX_NULL,
            NX_NULL);
         if(status == NX_SUCCESS)
         {
            status = nx_tcp_server_socket_relisten(
               msock->nx_tcp_socket_ip_ptr,
               msock->nx_tcp_socket_port,
               newsock);
            if(status == NX_SUCCESS || status == NX_CONNECTION_PENDING)
            {
               /* NetX logic to BSD logic */
               SOCKET* s = *listenSock;
               *listenSock = *outSock;
               *outSock = s;
               return 1;
            }
            nx_tcp_socket_delete(newsock);
         }
      }
      else if(status == NX_NOT_CONNECTED)
      {
         nx_tcp_server_socket_unaccept(msock);
         ns=nx_tcp_server_socket_relisten(msock->nx_tcp_socket_ip_ptr,
                                          msock->nx_tcp_socket_port,
                                          msock);
         if(ns != 0 && ns != NX_CONNECTION_PENDING)
            time2Reboot(ns);
      }
      else
         return -1;
   }
}


int se_bind(SOCKET* sock, U16 port)
{
   memset(sock, 0, sizeof(SOCKET));
   if(nx_tcp_socket_create(nxip,
                           &sock->nxSock,
                           "selib",
                           NX_IP_NORMAL,
                           NX_FRAGMENT_OKAY,
                           NX_IP_TIME_TO_LIVE,
                           2048,
                           NX_NULL,
                           NX_NULL))
   {
      return -1;
   }
   if(nx_tcp_server_socket_listen(nxip, port, &sock->nxSock, 5, 0))
   {
      return -3;
   }
   return 0;
}

 

/* Returns 0 on success.
   Error codes returned:
   -1: Cannot create socket: Fatal
   -2: Cannot resolve 'address'
   -3: Cannot connect
*/
int se_connect(SOCKET* sock, const char* address, U16 port)
{
   ULONG ipaddr;
   UINT status;
   memset(sock, 0, sizeof(SOCKET));
   status = nx_dns_host_by_name_get(nxdns,(UCHAR*)address,&ipaddr,
                                    txTime2Ticks(1000));
   if(status != NX_SUCCESS)
   {
      status = nx_dns_host_by_name_get(nxdns,(UCHAR*)address,&ipaddr,
                                       txTime2Ticks(2000));
      if(status != NX_SUCCESS)
      {
         return -2;
      }
   }
   if(nx_tcp_socket_create(nxip,
                           &sock->nxSock,
                           "SharkSSL Demo",
                           NX_IP_NORMAL,
                           NX_FRAGMENT_OKAY,
                           NX_IP_TIME_TO_LIVE,
                           2048,
                           NX_NULL,
                           NX_NULL))
   {
      return -1;
   }

   if( ! nx_tcp_client_socket_bind(&sock->nxSock,NX_ANY_PORT,NX_NO_WAIT) &&
       ! nx_tcp_client_socket_connect(&sock->nxSock, ipaddr, port,
                                      txTime2Ticks(5000)))
   {
      return 0;
   }

   nx_tcp_socket_disconnect(&sock->nxSock, NX_WAIT_FOREVER);
   nx_tcp_client_socket_unbind(&sock->nxSock);
   VRSP(nx_tcp_socket_delete(&sock->nxSock));
   
   return -3;
}



void se_close(SOCKET* sock)
{
   NX_TCP_SOCKET* nxs = &sock->nxSock;
   if(sock->nxFirstPacket)
   {
      nx_packet_release(sock->nxFirstPacket);
      sock->nxFirstPacket=0;
   }

   if( ! se_sockValid(sock) )
      return;

   if(nxs->nx_tcp_socket_client_type)
   {
      nx_tcp_socket_disconnect(nxs, NX_NO_WAIT);
      nx_tcp_client_socket_unbind(nxs);
   }
   else
   { /* Server sock */

      /* We do not know the state of the socket at this point. The socket
       * could be in any state. The first thing to do is to check if this
       * is a listen socket.
       */
      if(nxs->nx_tcp_socket_state ==  NX_TCP_LISTEN_STATE)
      {
         nx_tcp_server_socket_unlisten(nxs->nx_tcp_socket_ip_ptr,
                                       nxs->nx_tcp_socket_port);
      }
      else /* Attempt to close the socket */
      {
         nx_tcp_socket_disconnect(nxs, 1000);
      }
      nx_tcp_server_socket_unaccept(nxs);
   }
   VRSP(nx_tcp_socket_delete(nxs));
}



S32 se_send(SOCKET* sock, const void* buf, U32 len)
{
   NX_PACKET* nxPacket;
   NX_PACKET_POOL* pool =
      sock->nxSock.nx_tcp_socket_ip_ptr->nx_ip_default_packet_pool;
   if(!nx_packet_allocate(pool,&nxPacket,NX_TCP_PACKET,NX_WAIT_FOREVER))
   {
      if(!nx_packet_data_append(
            nxPacket,(void*)buf,(ULONG)len,pool,NX_WAIT_FOREVER))
      {
         if(!nx_tcp_socket_send(&sock->nxSock, nxPacket, NX_WAIT_FOREVER))
            return (S32)len;
      }
      VRSP(nx_packet_release(nxPacket));
   }
   return -1;
}



S32 se_recv(SOCKET* sock, void* buf, U32 len, U32 timeout)
{
   int retLen = 0;
   U8* dataPtr=(U8*)buf;
   if( ! sock->nxPacket )
   {
      NX_PACKET* nxPacket;
      UINT status;
      ULONG waitOption;
      waitOption = INFINITE_TMO == timeout ?
         NX_WAIT_FOREVER : txTime2Ticks(timeout);
      status =  nx_tcp_socket_receive(&sock->nxSock,&nxPacket,waitOption);
      if(status != NX_SUCCESS)
         return status == NX_NO_PACKET ? 0 : -1;
      sock->nxPacket = sock->nxFirstPacket = nxPacket;
      sock->nxPktOffs = nxPacket->nx_packet_prepend_ptr;
   }
   for(;;)
   {
      int pLen = sock->nxPacket->nx_packet_append_ptr - sock->nxPktOffs;
      baAssert(pLen > 0);
      if(pLen > len)
      {
         if(len)
         {
            memcpy(dataPtr, sock->nxPktOffs, len);
            sock->nxPktOffs += len;
            retLen = retLen + len;
         }
         return retLen;
      }
      memcpy(dataPtr, sock->nxPktOffs, pLen);
      retLen = retLen + pLen;
      dataPtr += pLen;
      len -= pLen;
      sock->nxPacket = sock->nxPacket->nx_packet_next;
      if( ! sock->nxPacket )
         break;
      sock->nxPktOffs = sock->nxPacket->nx_packet_prepend_ptr;
   }
   VRSP(nx_packet_release(sock->nxFirstPacket));
   sock->nxPacket=0;
   sock->nxFirstPacket=0;
   return retLen;
}


int se_sockValid(SOCKET* sock)
{
   return sock->nxSock.nx_tcp_socket_id != 0;
}


void seNetX_getSockName(SOCKET* sock, char* buf, int* status)
{
#ifdef FEATURE_NX_IPV6
   if(ip6)
   {
      memcpy(buf, &(sock->nxSock.nx_tcp_socket_ip_ptr->nx_ip_interface[0]->nxd_interface_ipv6_address_list_head.nxd_ipv6_address), 16);
      *status=16;
   }
   else
   {
      memcpy(buf,  &(sock->nxSock.nx_tcp_socket_ip_ptr->nx_ip_interface[0].nx_interface_ip_address), 4);
      *status=4;
   }
#else
   memcpy(buf, &(sock->nxSock.nx_tcp_socket_ip_ptr->nx_ip_interface[0].nx_interface_ip_address), 4);
   *status=4;
#endif
}


