/*
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
 *   $Id: selib.c 4127 2017-12-15 18:05:10Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2018
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
 *               https://realtimelogic.com
 ****************************************************************************
 */

#define SELIB_C

#include "selib.h"

#ifndef NO_BSD_SOCK

#if !defined(_WIN32) && !defined(closesocket)
#define INVALID_SOCKET -1
#define closesocket close
#endif


/* Wait 'tmo' milliseconds for socket 'read' activity.
   Returns 0 on pending data and -1 on timeout.
*/
#ifndef X_readtmo
static int readtmo(SOCKET sock, U32 tmo)
{
   fd_set recSet;
   struct timeval tv;
   tv.tv_sec = tmo / 1000;
   tv.tv_usec = (tmo % 1000) * 1000;
   FD_ZERO(&recSet);
   FD_SET(sock, &recSet);
   return select(sock+1, &recSet, 0, 0, &tv) > 0 ? 0 : -1;
}
#endif

#ifndef X_se_connect
int se_connect(SOCKET* sock, const char* address, U16 port)
{
   unsigned int ip;
   struct sockaddr_in addr;
   struct hostent* hostInfo = gethostbyname(address);
   *sock=-1;
   if(hostInfo)
      ip=((struct in_addr *)hostInfo->h_addr)->s_addr;
   else
   {
      ip=inet_addr(address);
      if(ip == INADDR_NONE)
         return -2;
   }
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = ip;
   if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      return -1;
   if(connect(*sock, (struct sockaddr*)&addr, sizeof(addr)) != INVALID_SOCKET)
      return 0;
   se_close(sock);
   return -3;
}
#endif


#ifndef X_se_bind
int se_bind(SOCKET* sock, U16 port)
{
   struct sockaddr_in  addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;
   if((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      xprintf(("Socket error\n"));
      return -1;
   }
   if(bind(*sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
   {
      xprintf(("Bind error: port %d\n", (int)port));
      se_close(sock);
      return -3;
   }
   if(listen(*sock, SOMAXCONN) < 0)
   {
      xprintf(("Listen error\n"));
      se_close(sock);
      return -2;
   }
   return 0;
}
#endif



/*
  Extract the first connection on the queue of pending connections,
  create a new socket, and allocate a new file descriptor for that
  socket.

  Returns:
  1: Success
  0: timeout
  -1: error

*/
int se_accept(SOCKET** listenSock, U32 timeout, SOCKET** outSock)
{
   if(timeout != INFINITE_TMO)
   {
      **outSock = -1;
      if(readtmo(**listenSock,timeout))
         return 0;
   }
   if( (**outSock=accept(**listenSock, 0, 0)) < 0 )
      return -1;
   return 1;
}


void se_close(SOCKET* sock)
{
   closesocket(*sock);
   *sock=-1;
}


int se_sockValid(SOCKET* sock)
{
   return *sock > 0 ? 1 : 0;
}


S32 se_send(SOCKET* sock, const void* buf, U32 len)
{
   return send(*sock,(void*)buf,len,0);
}


#ifndef X_se_recv
S32 se_recv(SOCKET* sock, void* buf, U32 len, U32 timeout)
{
   int recLen;
   if(timeout != INFINITE_TMO)
   {
      if(readtmo(*sock,timeout))
         return 0;
   }

   recLen = recv(*sock,buf,len,0);
   if (recLen <= 0)
   {
      /* If the virtual circuit was closed gracefully, and
       * all data was received, then a recv will return
       * immediately with zero bytes read.
       * We return -1 for above i.e. if(recLen == 0) return -1;
       * Note: this construction does not work with non blocking sockets.
       */
      return -1;
   }
   return recLen;
}
#endif


#endif /* NO_BSD_SOCK */


#if SE_SHA1
#include <string.h>


#define GET_U32_BE(w,a,i)                 \
{                                         \
   (w) = ((U32)(a)[(i)] << 24)            \
       | ((U32)(a)[(i) + 1] << 16)        \
       | ((U32)(a)[(i) + 2] <<  8)        \
       | ((U32)(a)[(i) + 3]);             \
}


#define PUT_U32_BE(w,a,i)                 \
{                                         \
   (a)[(i)]     = (U8)((w) >> 24);        \
   (a)[(i) + 1] = (U8)((w) >> 16);        \
   (a)[(i) + 2] = (U8)((w) >>  8);        \
   (a)[(i) + 3] = (U8)((w));              \
}



static const U8 sharkSslHashPadding[64] =
{
   0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/**
 *  SHA-1 implementation based on FIPS PUB 180-3
 *  http://csrc.nist.gov/publications/fips/fips180-3/fips180-3_final.pdf
 */
static void SharkSslSha1Ctx_process(SharkSslSha1Ctx *ctx, const U8 data[64])
{
   U32 a, b, c, d, e, temp;
   unsigned int i;
   U32 W[16];

   GET_U32_BE(W[0],  data,  0);
   GET_U32_BE(W[1],  data,  4);
   GET_U32_BE(W[2],  data,  8);
   GET_U32_BE(W[3],  data, 12);
   GET_U32_BE(W[4],  data, 16);
   GET_U32_BE(W[5],  data, 20);
   GET_U32_BE(W[6],  data, 24);
   GET_U32_BE(W[7],  data, 28);
   GET_U32_BE(W[8],  data, 32);
   GET_U32_BE(W[9],  data, 36);
   GET_U32_BE(W[10], data, 40);
   GET_U32_BE(W[11], data, 44);
   GET_U32_BE(W[12], data, 48);
   GET_U32_BE(W[13], data, 52);
   GET_U32_BE(W[14], data, 56);
   GET_U32_BE(W[15], data, 60);

   #define ROTL(x,n) ((U32)((U32)x << n) | ((U32)x >> (32 - n)))

   #define Ft1(x,y,z) ((x & (y ^ z)) ^ z)  /* equivalent to ((x & y) ^ ((~x) & z)) */
   #define Ft2(x,y,z) (x ^ y ^ z)
   #define Ft3(x,y,z) ((x & y) | ((x | y) & z))
   #define Ft4(x,y,z) (x ^ y ^ z)

   #define K1 0x5A827999
   #define K2 0x6ED9EBA1
   #define K3 0x8F1BBCDC
   #define K4 0xCA62C1D6

   a = ctx->state[0];
   b = ctx->state[1];
   c = ctx->state[2];
   d = ctx->state[3];
   e = ctx->state[4];

   for (i = 0; i < 80; i++)
   {
      if (i >= 16)
      {
         temp = W[i & 0xF] ^ W[(i + 2) & 0xF] ^ W[(i + 8) & 0xF] ^ W[(i + 13) & 0xF];
         W[i & 0xF] = temp = ROTL(temp, 1);
      }
      temp = W[i & 0xF];
      temp += e + ROTL(a, 5);
      if (i < 20)
      {
         temp += Ft1(b,c,d) + K1;
      }
      else if (i < 40)
      {
         temp += Ft2(b,c,d) + K2;
      }
      else if (i < 60)
      {
         temp += Ft3(b,c,d) + K3;
      }
      else
      {
         temp += Ft4(b,c,d) + K4;
      }
      e = d;
      d = c;
      c = ROTL(b, 30);
      b = a;
      a = temp;
   }

   ctx->state[0] += a;
   ctx->state[1] += b;
   ctx->state[2] += c;
   ctx->state[3] += d;
   ctx->state[4] += e;

   #undef K4
   #undef K3
   #undef K2
   #undef K1

   #undef Ft4
   #undef Ft3
   #undef Ft2
   #undef Ft1

   #undef ROTL
}


void SharkSslSha1Ctx_constructor(SharkSslSha1Ctx *ctx)
{
   /* ASSERT(((unsigned int)(ctx->buffer) & (sizeof(int)-1)) == 0); */

   ctx->total[0] = 0;
   ctx->total[1] = 0;

   ctx->state[0] = 0x67452301;
   ctx->state[1] = 0xEFCDAB89;
   ctx->state[2] = 0x98BADCFE;
   ctx->state[3] = 0x10325476;
   ctx->state[4] = 0xC3D2E1F0;
}


void SharkSslSha1Ctx_append(SharkSslSha1Ctx *ctx, const U8 *in, U32 len)
{
   unsigned int left, fill;

   left = (unsigned int)(ctx->total[0]) & 0x3F;
   fill = 64 - left;

   ctx->total[0] += len;
   if (ctx->total[0] < len)
   {
      ctx->total[1]++;
   }

   if((left) && (len >= fill))
   {
      memcpy((ctx->buffer + left), in, fill);
      SharkSslSha1Ctx_process(ctx, ctx->buffer);
      len -= fill;
      in  += fill;
      left = 0;
   }

   while (len >= 64)
   {
      SharkSslSha1Ctx_process(ctx, in);
      len -= 64;
      in  += 64;
   }

   if (len)
   {
      memcpy((ctx->buffer + left), in, len);
   }
}


void SharkSslSha1Ctx_finish(SharkSslSha1Ctx *ctx, U8 digest[20])
{
   U32 last, padn;
   U32 high, low;
   U8  msglen[8];

   high = (ctx->total[0] >> 29) | (ctx->total[1] <<  3);
   low  = (ctx->total[0] <<  3);

   PUT_U32_BE(high, msglen, 0);
   PUT_U32_BE(low,  msglen, 4);

   last = ctx->total[0] & 0x3F;
   padn = (last < 56) ? (56 - last) : (120 - last);

   SharkSslSha1Ctx_append(ctx, (U8*)sharkSslHashPadding, padn);
   SharkSslSha1Ctx_append(ctx, msglen, 8);

   PUT_U32_BE(ctx->state[0], digest,  0);
   PUT_U32_BE(ctx->state[1], digest,  4);
   PUT_U32_BE(ctx->state[2], digest,  8);
   PUT_U32_BE(ctx->state[3], digest, 12);
   PUT_U32_BE(ctx->state[4], digest, 16);
}
#endif
