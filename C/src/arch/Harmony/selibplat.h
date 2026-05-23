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
 *   $Id: selibplat.h 4019 2017-03-03 20:24:42Z wini $
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
 ****************************************************************************
 *
 */

#define SharkSSLHarmony
#define NO_BSD_SOCK
#define SECTX_EX TCP_SOCKET htcp;int state

#define SOCKET MCSOCKET
#include <tcpip/tcpip.h>
#include <tcpip/tcp.h>
#undef SOCKET
#include "../../SeCtx.h"


typedef struct SOCKET
{
   SeCtx* ctx;
   TCP_SOCKET htcp;
   int state;
} SOCKET;

#define SELIB_INVALID          0
#define SELIB_DNS_PENDING      1
#define SELIB_CONNECT_PENDING  2
#define SELIB_SEND_PENDING     3
#define SELIB_RECV             4

#define SOCKET_constructor(o, ctxMA) \
  memset(o,0,sizeof(SOCKET)),(o)->htcp=INVALID_SOCKET,(o)->ctx=ctxMA

int SeCtx_run(SeCtx* ctx);
