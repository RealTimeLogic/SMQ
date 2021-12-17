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
 ****************************************************************************
 *
 */

#include <uip.h>
#include "../../SeCtx.h"

#define SharkSSLuIP
#define NO_BSD_SOCK

typedef struct SOCKETMSG
{
   struct SOCKETMSG* next;
   int size;
} SOCKETMSG;

#ifndef SEUIP_CONN
#define SEUIP_CONN struct uip_conn
#endif

typedef struct SOCKET
{
   SeCtx* ctx;
   SEUIP_CONN* conn;
   union {
      struct {
         uip_ipaddr_t ipaddr;
         BaBool OK;
      } resolv;
      struct {
         U8* buf;
         int size;
      } data;
   } u;
   SOCKETMSG* msg;
   U8 flags;
} SOCKET;

#define SOCKFLAG_inRecv 1
#define SOCKFLAG_inSend 2
#define SOCKFLAG_inConnect 4
#define SOCKFLAG_newdata 8
#define SOCKFLAG_startSend 16


#define SOCKET_constructor(o, ctxMA) memset(o,0,sizeof(SOCKET)),(o)->ctx=ctxMA

int SeCtx_run(SeCtx* ctx);
