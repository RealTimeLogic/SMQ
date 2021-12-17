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

#include <lwip/tcp.h>
#include <lwip/dns.h>
#include "../../SeCtx.h"

#define SharkSSLlwIPRaw
#define NO_BSD_SOCK

struct SOCCONT;

typedef struct SOCKET
{
   union {
      ip_addr_t ipaddr; /* connect and gethostbyname */
      struct {
         struct SOCCONT* pending;
         struct SOCKET* outSock;
      } accept;
      struct {
         struct pbuf* buf; /* head */
         struct pbuf* cur; /* cursor */
         U8* data; /* ptr into cursor */
         u16_t len; /* len left in cursor */
      } rec;
   } u;
   struct tcp_pcb * pcb;
   SeCtx* ctx;
   int ecode;
   S32 outLen;
   U8 flags;
} SOCKET;


typedef struct SOCCONT
{
   struct SOCCONT* next;
   SOCKET sock;
} SOCCONT;


#define SOCKFLAG_inRecv 1
#define SOCKFLAG_inSend 2


#define SOCKET_constructor(o, ctxMA) memset(o,0,sizeof(SOCKET)),(o)->ctx=ctxMA

int SeCtx_run(SeCtx* ctx);
