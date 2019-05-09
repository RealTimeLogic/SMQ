/**
 *     ____             _________                __                _
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/
 *                                                       /____/
 *
 ****************************************************************************
 *   PROGRAM MODULE
 *
 *   $Id: SMQClient.c 4379 2019-05-09 20:15:11Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2019
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
 * SMQ C library:
 *  https://realtimelogic.com/ba/doc/en/C/reference/html/group__SMQClient.html
 */


#include "SMQ.h"
#include <ctype.h>

#define MSG_INIT         1
#define MSG_CONNECT      2
#define MSG_CONNACK      3
#define MSG_SUBSCRIBE    4
#define MSG_SUBACK       5
#define MSG_CREATE       6
#define MSG_CREATEACK    7
#define MSG_PUBLISH      8
#define MSG_UNSUBSCRIBE  9
#define MSG_DISCONNECT   11
#define MSG_PING         12
#define MSG_PONG         13
#define MSG_OBSERVE      14
#define MSG_UNOBSERVE    15
#define MSG_CHANGE       16
#define MSG_CREATESUB    17
#define MSG_CREATESUBACK 18
#define MSG_PUBFRAG      19



#define SMQ_S_VERSION 1
#define SMQ_C_VERSION 2

#if defined(B_LITTLE_ENDIAN)
static void
netConvU16(U8* out, const U8* in)
{
   out[0] = in[1];
   out[1] = in[0];
}
static void
netConvU32(U8* out, const U8* in)
{
   out[0] = in[3];
   out[1] = in[2];
   out[2] = in[1];
   out[3] = in[0];
}
#elif defined(B_BIG_ENDIAN)
#define netConvU16(out, in) memcpy(out,in,2)
#define netConvU32(out, in) memcpy(out,in,4)
#else
#error ENDIAN_NEEDED_Define_one_of_B_BIG_ENDIAN_or_B_LITTLE_ENDIAN
#endif

#define SMQ_resetRB(o) (o)->rBufIx=0
#ifdef SMQ_ENABLE_SENDBUF
#define SMQ_resetSB(o) (o)->sBufIx=0
#define SMQSBufIx(o) o->sBufIx
#define SMQSBuf(o) o->sBuf
#else
#define SMQ_resetSB SMQ_resetRB
#define SMQSBufIx(o) o->rBufIx
#define SMQSBuf(o) o->buf
#endif

#ifdef SMQ_ENABLE_SENDBUF
#define SMQ_recv(o,buf,len) se_recv(&o->sock, buf, len, o->rBufIx ? INFINITE_TMO : o->timeout)
#else
static int
SMQ_recv(SMQ* o, U8* buf, int len)
{
   o->inRecv=TRUE;
   len = se_recv(&o->sock, buf, len, o->rBufIx ? INFINITE_TMO : o->timeout);
   o->inRecv=FALSE;
   return len;
}
#endif

/* Reads and stores the 3 frame header bytes (len:2 & msg:1) in the buffer.
   Returns zero on success and a value (error code) less than zero on error
 */
static int
SMQ_readFrameHeader(SMQ* o)
{
   int x;
   SMQ_resetRB(o);
   do
   {
      x=SMQ_recv(o, o->buf, 3 - o->rBufIx);
      o->rBufIx += (U16)x; /* assume it's OK */
   } while(x > 0 && o->rBufIx < 3);
   if(x > 0)
   {
      netConvU16((U8*)&o->frameLen, o->buf);
      return 0;
   }
   o->status = x == 0 ? SMQE_TIMEOUT : x;
   return o->status;
}



/* Reads a complete frame.
   Designed to be used by control frames.

   Returns Frame Len (FL) on success and a value (error code) less
   than zero on error.
*/
static int
SMQ_readFrame(SMQ* o, int hasFH)
{
   int x;
   if(!hasFH && SMQ_readFrameHeader(o)) return o->status;
   if(o->frameLen > o->bufLen || o->frameLen < 3)
      return o->status = SMQE_BUF_OVERFLOW;
   do
   {
      x=SMQ_recv(o, o->buf+o->rBufIx, o->frameLen - o->rBufIx);
      o->rBufIx += (U16)x; /* assume it's OK */
   } while(x > 0 && o->rBufIx < o->frameLen);
   SMQ_resetRB(o);
   if(x > 0) return 0;
   o->status = x == 0 ? SMQE_TIMEOUT : x;
   return o->status;
}


static int
SMQ_readData(SMQ* o, U16 size)
{
   int x;
   do {
      x=SMQ_recv(o, o->buf+o->rBufIx, size-o->rBufIx);
      o->rBufIx += (U16)x; /* assume it's OK */
   } while(x > 0 && o->rBufIx < size);
   if(x > 0) return o->rBufIx;
   o->status = x == 0 ? SMQE_TIMEOUT : x;
   return o->status;
}



static int
SMQ_flushb(SMQ* o)
{
   if(SMQSBufIx(o))
   {
      int x = se_send(&o->sock, SMQSBuf(o), SMQSBufIx(o));
      SMQ_resetSB(o);
      if(x < 0)
      {
         o->status = x;
         return x;
      }
   }
   return 0;
}



static int
SMQ_putb(SMQ* o, const void* data, int len)
{
   if(len < 0)
      len = strlen((char*)data);
   if(o->bufLen <= (SMQSBufIx(o) + len))
      return -1;
   memcpy(SMQSBuf(o) + SMQSBufIx(o), data, len);
   SMQSBufIx(o) += (U16)len;
   return 0;
}



static int
SMQ_writeb(SMQ* o, const void* data, int len)
{
   if(len < 0)
      len = strlen((char*)data);
   if(o->bufLen <= (SMQSBufIx(o) + len))
   {
      if(SMQ_flushb(o)) return o->status;
      if((len+20) >= o->bufLen)
      {
         len=se_send(&o->sock, data, len);
         if(len < 0)
         {
            o->status = len;
            return len;
         }
         return 0;
      }
   }
   memcpy(SMQSBuf(o) + SMQSBufIx(o), data, len);
   SMQSBufIx(o) += (U16)len;
   return 0;
}


void
SMQ_constructor(SMQ* o, U8* buf, U16 bufLen)
{
   memset(o, 0, sizeof(SMQ));
   o->buf = buf;
#ifdef SMQ_ENABLE_SENDBUF
   bufLen = bufLen / 2;
   o->sBuf = buf+bufLen;
#endif
   o->bufLen = bufLen;
   o->timeout = 60 * 1000;
   o->pingTmo = 20 * 60 * 1000;
}


int
SMQ_init(SMQ* o, const char* url, U32* rnd)
{
   int x;
   const char* path;
   const char* eohn; /* End Of Hostname */
   U16 portNo=0;
   if( ! strncmp("http://",url,7) )
      url+=7;
   else if( ! strncmp("https:", url, 6) )
      return SMQE_INVALID_URL;
   path=strchr(url, '/');
   if(!path)
      path = url+strlen(url);
   if(path > url && ISDIGIT((unsigned char)*(path-1)))
   {
      for(eohn = path-2 ; ; eohn--)
      {
         if(path > url)
         {
            if( ! ISDIGIT((unsigned char)*eohn) )
            {
               const char* ptr = eohn;
               if(*ptr != ':')
                  goto L_defPorts;
               while(++ptr < path)
                  portNo = 10 * portNo + (*ptr-'0');
               break;
            }
         }
         else
            return o->status = SMQE_INVALID_URL;
      }
   }
   else
   {
L_defPorts:
      portNo=80;
      eohn=path; /* end of eohn */
   }
   /* Write hostname */

   SMQ_resetSB(o);
   SMQSBufIx(o) = (U16)(eohn-url); /* save hostname len */
   if((SMQSBufIx(o)+1) >= o->bufLen)
      return o->status = SMQE_BUF_OVERFLOW;
   memcpy(SMQSBuf(o), url, SMQSBufIx(o)); 
   SMQSBuf(o)[SMQSBufIx(o)]=0;

   /* connect to 'hostname' */
   if( (x = se_connect(&o->sock, (char*)SMQSBuf(o), portNo)) != 0 )
      return o->status = x;

   /* Send HTTP header. Host is included for multihomed servers */
   SMQ_resetSB(o);
   if(SMQ_writeb(o, SMQSTR("GET ")) ||
      (*path == 0 ? SMQ_writeb(o, "/", -1) : SMQ_writeb(o, path, -1)) ||
      SMQ_writeb(o,SMQSTR(" HTTP/1.0\r\nHost: ")) ||
      SMQ_writeb(o, url, eohn-url) ||
      SMQ_writeb(o, SMQSTR("\r\nSimpleMQ: 1\r\n")) ||
      SMQ_writeb(o, SMQSTR("User-Agent: SimpleMQ/1\r\n\r\n")) ||
      SMQ_flushb(o))
   {
      return o->status;
   }

   /* Get the Init message */
   if(SMQ_readFrame(o, FALSE)) return o->status;
   if(o->frameLen < 11 || o->buf[2] != MSG_INIT || o->buf[3] != SMQ_S_VERSION)
      return o->status=SMQE_PROTOCOL_ERROR;
   if(rnd)
      netConvU32((U8*)rnd,o->buf+4);
   memmove(o->buf, o->buf+8, o->frameLen-8);
   o->buf[o->frameLen-8]=0;
   return 0;
}



int
SMQ_connect(SMQ* o, const char* uid, int uidLen, const char* credentials,
            U8 credLen, const char* info, int infoLen)
{
   if(o->bufLen < 5+uidLen+credLen+infoLen) return SMQE_BUF_OVERFLOW;
   SMQSBufIx(o) = 2;
   SMQSBuf(o)[SMQSBufIx(o)++] = MSG_CONNECT;
   SMQSBuf(o)[SMQSBufIx(o)++] = SMQ_C_VERSION;
   SMQSBuf(o)[SMQSBufIx(o)++] = 0;
   SMQSBuf(o)[SMQSBufIx(o)++] = 0;
   SMQSBuf(o)[SMQSBufIx(o)++] = (U8)uidLen;
   SMQ_putb(o,uid,uidLen);
   SMQSBuf(o)[SMQSBufIx(o)++] = credLen;
   if(credLen)
      SMQ_putb(o,credentials,credLen);
   if(info)
      SMQ_putb(o,info,infoLen);
   netConvU16(SMQSBuf(o), (U8*)&SMQSBufIx(o)); /* Frame Len */
   if(SMQ_flushb(o)) return o->status;

   /* Get the response message Connack */
   if(SMQ_readFrame(o, FALSE)) return o->status;
   if(o->frameLen < 8 || o->buf[2] != MSG_CONNACK)
      return SMQE_PROTOCOL_ERROR;
   netConvU32((U8*)&o->clientTid, o->buf+4);
   if(o->buf[3] != 0)
   {
      memmove(o->buf, o->buf+8, o->frameLen-8);
      o->buf[o->frameLen-8]=0;
   }
   else
      o->buf[0]=0; /* No error message */
   o->status = (int)o->buf[3]; /* OK or error code */
   return o->status;
}


void
SMQ_disconnect(SMQ* o)
{
   if(se_sockValid(&o->sock))
   {
      SMQSBufIx(o) = 3;
      netConvU16(SMQSBuf(o), (U8*)&SMQSBufIx(o)); /* Frame Len */
      SMQSBuf(o)[2] = MSG_DISCONNECT;
      SMQ_flushb(o);
      se_close(&o->sock);
   }
}


void
SMQ_destructor(SMQ* o)
{
   se_close(&o->sock);
}

/* Send MSG_SUBSCRIBE, MSG_CREATE, or MSG_CREATESUB */
static int
SMQ_subOrCreate(SMQ* o,const char* topic,int msg)
{
   int len = strlen(topic);
   if( ! len ) return SMQE_PROTOCOL_ERROR;
   if((3+len) > o->bufLen) return SMQE_BUF_OVERFLOW;
   SMQSBufIx(o) = 2;
   SMQSBuf(o)[SMQSBufIx(o)++] = (U8)msg;
   SMQ_putb(o,topic,len);
   netConvU16(SMQSBuf(o), (U8*)&SMQSBufIx(o)); /* Frame Len */
   return SMQ_flushb(o);
}


int
SMQ_subscribe(SMQ* o, const char* topic)
{
   return SMQ_subOrCreate(o,topic, MSG_SUBSCRIBE);
}


int
SMQ_create(SMQ* o, const char* topic)
{
   return SMQ_subOrCreate(o,topic,MSG_CREATE);
}


int
SMQ_createsub(SMQ* o, const char* topic)
{
   return SMQ_subOrCreate(o,topic,MSG_CREATESUB);
}


static int
SMQ_sendMsgWithTid(SMQ* o, int msgType, U32 tid)
{
   SMQSBufIx(o)=7;
   netConvU16(SMQSBuf(o), (U8*)&SMQSBufIx(o)); /* Frame Len */
   SMQSBuf(o)[2] = (U8)msgType;
   netConvU32(SMQSBuf(o)+3, (U8*)&tid);
   return SMQ_flushb(o) ? o->status : 0;
}


int
SMQ_unsubscribe(SMQ* o, U32 tid)
{
   return SMQ_sendMsgWithTid(o, MSG_UNSUBSCRIBE, tid);
}


int
SMQ_publish(SMQ* o, const void* data, int len, U32 tid, U32 subtid)
{
   U16 tlen=(U16)len+15;
   if(tlen <= o->bufLen && ! o->inRecv)
   {
      netConvU16(SMQSBuf(o), (U8*)&tlen); /* Frame Len */
      SMQSBuf(o)[2] = MSG_PUBLISH;
      netConvU32(SMQSBuf(o)+3, (U8*)&tid);
      netConvU32(SMQSBuf(o)+7,(U8*)&o->clientTid);
      netConvU32(SMQSBuf(o)+11,(U8*)&subtid);
      SMQSBufIx(o) = 15;
      if(SMQ_writeb(o, data, len) || SMQ_flushb(o)) return o->status;
   }
   else
   {
      U8 buf[15];
      netConvU16(buf, (U8*)&tlen); /* Frame Len */
      buf[2] = MSG_PUBLISH;
      netConvU32(buf+3, (U8*)&tid);
      netConvU32(buf+7,(U8*)&o->clientTid);
      netConvU32(buf+11,(U8*)&subtid);
      o->status=se_send(&o->sock, buf, 15);
      if(o->status < 0) return o->status;
      o->status=se_send(&o->sock, data, len);
      if(o->status < 0) return o->status;
      o->status=0;
   }
   return 0;
}


int
SMQ_wrtstr(SMQ* o, const char* data)
{
   return SMQ_write(o,data,strlen(data));
}


int
SMQ_write(SMQ* o,  const void* data, int len)
{
   U8* ptr = (U8*)data;
   if(o->inRecv)
      return SMQE_PROTOCOL_ERROR;
   while(len > 0)
   {
      int chunk,left;
      if(!SMQSBufIx(o))
         SMQSBufIx(o) = 15;
      left = o->bufLen - SMQSBufIx(o);
      chunk = len > left ? left : len;
      memcpy(SMQSBuf(o)+SMQSBufIx(o), ptr, chunk);
      ptr += chunk;
      len -= chunk;
      SMQSBufIx(o) += (U16)chunk;
      if(SMQSBufIx(o) >= o->bufLen && SMQ_pubflush(o, 0, 0))
         return o->status;
   }
   return 0;
}


int
SMQ_pubflush(SMQ* o, U32 tid, U32 subtid)
{
   if(!SMQSBufIx(o))
      SMQSBufIx(o) = 15;
   netConvU16(SMQSBuf(o), (U8*)&SMQSBufIx(o)); /* Frame Len */
   SMQSBuf(o)[2] = MSG_PUBFRAG;
   netConvU32(SMQSBuf(o)+3, (U8*)&tid);
   netConvU32(SMQSBuf(o)+7,(U8*)&o->clientTid);
   netConvU32(SMQSBuf(o)+11,(U8*)&subtid);
   o->status=se_send(&o->sock, SMQSBuf(o), SMQSBufIx(o));
   SMQ_resetSB(o);
   if(o->status < 0) return o->status;
   o->status=0;
   return 0;
}



int
SMQ_observe(SMQ* o, U32 tid)
{
   return SMQ_sendMsgWithTid(o, MSG_OBSERVE, tid);
}


int
SMQ_unobserve(SMQ* o, U32 tid)
{
   return SMQ_sendMsgWithTid(o, MSG_UNOBSERVE, tid);
}


int
SMQ_getMessage(SMQ* o, U8** msg)
{
   int x;

   if(o->bytesRead)
   {
      if(o->bytesRead < o->frameLen)
      {
         U16 size = o->frameLen - o->bytesRead;
         *msg = o->buf;
         x=SMQ_readData(o, size <= o->bufLen ? size : o->bufLen);
         SMQ_resetRB(o);
         if(x > 0) o->bytesRead += (U16)x;
         else o->bytesRead = 0;
         return x;
      }
      o->bytesRead = 0;
   }

  L_readMore:
   if(SMQ_readFrameHeader(o))
   {
      /* Timeout is not an error in between frames */
      if(o->status == SMQE_TIMEOUT)
      {
         if(o->pingTmoCounter >= 0)
         {
            o->pingTmoCounter += o->timeout;
            if(o->pingTmoCounter >= o->pingTmo)
            {
               o->pingTmoCounter = -10000; /* PONG tmo hard coded to 10 sec */
               SMQSBufIx(o)=3;
               netConvU16(SMQSBuf(o), (U8*)&SMQSBufIx(o)); /* Frame Len */
               SMQSBuf(o)[2] = MSG_PING;
               if(SMQ_flushb(o)) return o->status;
            }
         }
         else
         {
            o->pingTmoCounter += o->timeout;
            if(o->pingTmoCounter >= 0)
               return SMQE_PONGTIMEOUT;
         }
         return 0;
      }
      return o->status;
   }
   o->pingTmoCounter=0;
   switch(o->buf[2])
   {
      case MSG_DISCONNECT:
         if(SMQ_readFrame(o, TRUE))
            o->buf[0]=0;
         else
         {
            memmove(o->buf, o->buf+3, o->frameLen-3);
            o->buf[o->frameLen-3]=0;
         }
         return SMQE_DISCONNECT;

      case MSG_CREATEACK:
      case MSG_CREATESUBACK:
      case MSG_SUBACK:
         if(SMQ_readFrame(o, TRUE)) return o->status;
         if(o->frameLen < 9) return SMQE_PROTOCOL_ERROR;
         if(msg) *msg = o->buf; /* topic name */
         if(o->buf[3]) /* Denied */
         {
            o->ptid=0;
            o->status = -1;
         }
         else
         {
            netConvU32((U8*)&o->ptid, o->buf+4);
            o->status = 0;
         }
         switch(o->buf[2])
         {
            case MSG_CREATEACK:    x = SMQ_CREATEACK;    break;
            case MSG_CREATESUBACK: x = SMQ_CREATESUBACK; break;
            default: x = SMQ_SUBACK;
         }
         memmove(o->buf, o->buf+8, o->frameLen-8);
         o->buf[o->frameLen-8]=0;
         return x;

      case MSG_PUBLISH:
         if(o->frameLen < 15) return SMQE_PROTOCOL_ERROR;
         o->bytesRead = o->frameLen <= o->bufLen ? o->frameLen : o->bufLen;
         x=SMQ_readData(o, o->bytesRead);
         SMQ_resetRB(o);
         if(x > 0)
         {
            netConvU32((U8*)&o->tid, o->buf+3);
            netConvU32((U8*)&o->ptid, o->buf+7);
            netConvU32((U8*)&o->subtid, o->buf+11);
            *msg = o->buf + 15;
            return o->bytesRead - 15;
         }
         o->bytesRead=0;
         return x < 0 ? x : -1;

      case MSG_PING:
      case MSG_PONG:
         if(o->frameLen != 3) return SMQE_PROTOCOL_ERROR;
         if(o->buf[2] == MSG_PING)
         {
            SMQSBufIx(o)=3;
            netConvU16(SMQSBuf(o), (U8*)&SMQSBufIx(o)); /* Frame Len */
            SMQSBuf(o)[2] = MSG_PONG;
            if(SMQ_flushb(o)) return o->status;
         }
         goto L_readMore;

      case MSG_CHANGE:
         if(o->frameLen != 11) return SMQE_PROTOCOL_ERROR;
         if(SMQ_readFrame(o, TRUE)) return o->status;
            netConvU32((U8*)&o->ptid, o->buf+7);
            o->status = (int)o->ptid;
            netConvU32((U8*)&o->ptid, o->buf+3);
            return SMQ_SUBCHANGE;

      default:
         return SMQE_PROTOCOL_ERROR;
   }
}
