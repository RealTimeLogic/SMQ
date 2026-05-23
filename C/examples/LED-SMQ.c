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
 *   $Id: LED-SMQ.c 5029 2022-01-16 21:32:09Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2022
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
  
   SMQ LED example.

   This code is the device side for the SMQ LED controller, an example
   that enables LEDs in a device to be controlled from a browser and
   Java (including Android).

   When this code runs, it connects to our public SMQ test broker. The
   device will show up on the following page:
     https://simplemq.com/m2m-led/
   The device code can also be controlled using an Android app:
     https://play.google.com/store/apps/details?id=demo.smq_android

   This example's tutorial can be found online:
   https://makoserver.net/articles/Browser-to-Device-LED-Control-using-SimpleMQ
   The device code details can be found under section "Device C
   code".

   TL;DR:
   The bulk of the implementation is in function m2mled() below. For a
   more basic SMQ C example, see the following tutorial and the
   accompanying C code example:
   https://realtimelogic.com/articles/Modern-Approach-to-Embedding-a-Web-Server-in-a-Device

 SharkMQ v.s. SMQ (secure v.s. non secure):
   This example can be compiled with SharkMQ (TLS) or standard SMQ (non secure).
   SharkMQ lib: https://realtimelogic.com/ba/doc/en/C/shark/group__SMQLib.html
   SMQ lib: https://realtimelogic.com/ba/doc/en/C/reference/html/structSMQ.html
   This example uses the SharkMQ compatibility API when compiled for
   standard SMQ, thus making it easy to upgraded to a secure version,
   if needed. The macro SMQ_SEC is defined when SMQ.h is the SharkMQ
   (secure) version.
 GitHub:
   This example is included in the following GitHub repositories:
     Using SharkMQ: https://github.com/RealTimeLogic/SharkSSL
     Using SMQ:     https://github.com/RealTimeLogic/SMQ
 Ready to run RTOS example:
   This example is also included in the SharkSSL ESP32 IDE:
     https://realtimelogic.com/downloads/sharkssl/ESP32/

 SMQ Broker:
   To set up your own SMQ broker (server), download the Mako Server
   and activate the tutorials. A ready to use broker is included in
   the tutorials:
      https://makoserver.net/download/
   Online broker tutorial:
     https://makoserver.net/articles/Setting-up-a-Low-Cost-SMQ-IoT-Broker

 Porting:
   The code is designed for embedded devices, but can also be run on a
   host computer (Windows/Linux) as a simulated device with four
   simulated LEDs.

   The macro HOST_PLATFORM must be defined if compiled and run on a
   non embedded platform (HLOS). The code within this section sets up
   the simulated environment found in led-host-sim.ch. Do not set this
   macro if the code is to be cross compiled for an embedded device.

   When cross compiling for an embedded device, create a separate C
   file and include ledctrl.h. The C file must have the following
   functions (used by the generic code in this file):
     * int getUniqueId(const U8* id);
     * const LedInfo* getLedInfo(int* len);
     * const char* getDevName(void);
     * int setLed(int ledId, int on);
     * int setLedFromDevice(int* ledId, int* on);
     * int getLedState(int ledId, int on, int set);
     * void setProgramStatus(ProgramStatus s);
     * xprintf (selib.h)
   See the following documentation for how to interface the LED
   functions with your hardware:
   https://realtimelogic.com/ba/doc/en/C/shark/md_md_Examples.html#LedDemo

   The above functions are documented in ledctrl.h. You may also study
   the simulated versions under the HOST_PLATFORM code section
   below. Function xprintf is optional and used if macro XPRINTF=1. If
   enabled, program status is printed during operation.

 */


/* SMQ Broker URL:
   Change the domain/url if you are running your own broker
   Note: you can also set the domain at the command prompt when in
   simulation mode. The default settings connect to our online SMQ broker.
 Local SMQ Broker:
   When running this program and Mako Server examples on the same server:
   #define SMQ_DOMAIN "https://localhost"
 */
#include "SMQ.h"
#define SMQ_DOMAIN "simplemq.com"
#ifdef SMQ_SEC
#define SMQ_PROTOCOL "https://"
#else
#define SMQ_PROTOCOL "http://"
#endif
/* The broker complete URL: http(s)://domain/smq.lsp */
#define SMQ_URL SMQ_PROTOCOL SMQ_DOMAIN "/smq.lsp"

#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include "ledctrl.h"

/*                   sharkSslCAList (Ref-CA)

The data from the include file below is a CA (Certificate Authority)
Root Certificate. The CA enables SharkSSL and this example to verify the identity of
the broker. We include data for only one CA in this example
since only the signer of simplemq.com's certificate is required
for validation in this example. See the "certcheck" example for how to
validate any type of certificate and for more options on how to store
certificates, in addition to embedding the CA as it is done in this
example.

The Certification Authority (CA) Root Certificate was
converted to 'sharkSslCAList' as follows:
SharkSSLParseCAList CA_RTL_EC_256.pem > CA_RTL_EC_256.h

An introduction to certificate management can be found here:
https://goo.gl/rjdQjg

The SMQ broker at simplemq.com uses an RSA certificate for browsers
and an ECC certificate for devices. See the following tutorial for
details:
https://realtimelogic.com/ba/doc/en/C/shark/certmngmtIoT.html

*/
#ifdef SMQ_SEC
#include <SharkSslCrypto.h>
#include "certificates/CA_RTL_EC_256.h"
#endif


/****************************************************************************
 **************************-----------------------***************************
 **************************| BOARD SPECIFIC CODE |***************************
 **************************-----------------------***************************
 ****************************************************************************/

/* The macro HOST_PLATFORM should be set if you are running the example
   on Windows or Linux. The code below provides a simulated "host" environment. 
*/
#if HOST_PLATFORM

/* Include the simulated LED environment/functions */
#include "led-host-sim.ch"

static const char* simpleMqUrl=0; /* Defaults to value in macro SMQ_DOMAIN */

/* Enable simulated temperature */
#ifndef ENABLE_TEMP
#define ENABLE_TEMP
#endif

/* This example can optionally connect via a SOCKS5 or HTTP proxy.
   See proxy.ch
*/
#ifdef ENABLE_PROXY
static Proxy proxy;
static void extractProxyArgs(const char* arg)
{
   char* ptr = strdup(arg);
   const char* proxyName = ptr;
   ptr = strchr(ptr,':');
   if(ptr)
   {
      const char*  port;
      *ptr = 0;
      port = ++ptr;
      ptr = strchr(ptr,':');
      if(ptr)
      {
         const char* proxyUserPass=0;
         int socks=FALSE;
         int portNo;
         *ptr++ = 0;
         portNo = atoi(port);
         if(strncmp(ptr,"https",5))
         {
            if(strncmp(ptr,"socks",5))
               printUsage();
            socks=TRUE;
         }
         ptr = strchr(ptr,':');
         if(ptr)
            proxyUserPass=ptr+1;
         Proxy_constructor(&proxy,proxyName,(U16)portNo,proxyUserPass,socks);
         printf("Using %s proxy %s:%d\n",
                socks ? "SOCKS" : "HTTPS",
                proxyName,
                portNo);
         return;
      }
   }
   printUsage();
}
#else /* ENABLE_PROXY */
#define extractProxyArgs(arg) printUsage()
#endif  /* ENABLE_PROXY */

static void  printUsage(void)
{
   printf("m2m-led [-bURL]");
#ifdef ENABLE_PROXY
   printf(" [-pname:port:https:socks[:username:password]]");
#endif
   printf("\n");
   exit(1);
}



#ifndef NO_MAIN
int main(int argc, char* argv[])
{
   int i;
#ifdef _WIN32
   WSADATA wsaData;
   /* Windows specific: Start winsock library */
    WSAStartup(MAKEWORD(1,1), &wsaData);
#endif

   printf("%sEnter %s -? for information on optional arguments.\n\n",
#ifdef SMQ_SEC
          "SharkMQ/SharkSSL SMQ LED Demo.\n"
#else
          "SMQ LED Demo.\n"
#endif
          "Copyright (c) 2018 Real Time Logic.\n",
          argv[0]);

   for(i=1 ; i < argc ; i++)
   {
      char* ptr = argv[i];
      if(*ptr++ == '-')
      {
         switch(*ptr)
         {
            case 'b':
               simpleMqUrl = ++ptr;
               xprintf(("Overriding broker URL\n\tfrom %s\n\tto %s\n%s\n\n",
                        SMQ_URL,simpleMqUrl,
                        "Note: error messages will include original "
                        "broker name"));
               break;

            case 'p':
               extractProxyArgs(++ptr);
               break;
                  
            default:
               printUsage();
         }
      }
      else
         printUsage();
   }
   if(!simpleMqUrl)
      simpleMqUrl = SMQ_URL;
   mainTask(0);
   printf("Press return key to exit\n");
   getchar();
   return 0;
}
#endif


/* This example can optionally connect via a SOCKS5 or HTTP proxy.
   See proxy.ch
*/
#ifdef ENABLE_PROXY
static void setProxy(SharkMQ* smq)
{
   if(proxy.connect)
      SharkMQ_setProxy(smq, &proxy);
}
#else
#define setProxy(x)
#endif
#else /* HOST_PLATFORM */
#define setProxy(x)
#endif /* HOST_PLATFORM */




/****************************************************************************
 **************************----------------------****************************
 **************************| GENERIC CODE BELOW |****************************
 **************************----------------------****************************
 ****************************************************************************/

/* LED color type to string conversion. */
static const char*
ledType2String(LedColor t)
{
   switch(t)
   {
      case LedColor_red: return "red";
      case LedColor_yellow: return "yellow";
      case LedColor_green: return "green";
      case LedColor_blue: return "blue";
   }
   baAssert(0);
   return "";
}


/* Send the device capabilities as JSON to the browser. Note: we could
   have used our JSON library for creating the JSON, but the library
   adds additional code. We have opted to manually craft the JSON
   instead of using the JSON library. This keeps the code size
   down. Manually crafting JSON data is easy, parsing JSON is not
   easy. However, we have no need for parsing JSON in this demo.

   You can optionally rewrite this code and use the following JSON
   library: https://realtimelogic.com/products/json/

   When you manually craft JSON, it can be good to use a JSON lint
   parser if you should get a parse error in the browser. The JSON
   lint parser will give you much better error reporting:
   http://jsonlint.com/ 
 */
static int
sendDevInfo(SharkMQ* smq, const char* ipaddr, U32 tid, U32 subtid)
{
   int i, ledLen;
   char buf[11];
   char *ptr;
   int val;
   const LedInfo* linfo = getLedInfo(&ledLen);

   SharkMQ_wrtstr(smq, "{\"ipaddr\":\"");
   SharkMQ_wrtstr(smq, ipaddr);
   SharkMQ_wrtstr(smq, "\",\"devname\":\"");
   SharkMQ_wrtstr(smq, getDevName());
   SharkMQ_wrtstr(smq, "\",\"leds\":[");

   /* Write JSON:
     {
        "id": number,
        "name": string,
        "color": string,
        "on": boolean
     }
   */
   for(i = 0 ; i < ledLen ; i++)
   {
      ptr = &buf[(sizeof buf) - 1];
      val = linfo[i].id;
      *ptr = 0; 
      if(i != 0)
         SharkMQ_wrtstr(smq,",");
      SharkMQ_wrtstr(smq, "{\"id\":");
      do { /* convert number to string */
         int r = val % 10U;
         val /= 10U;
         *--ptr = (char)('0' + r);
      } while (val && ptr > buf); 
      SharkMQ_wrtstr(smq, ptr); /* number converted to string */
      SharkMQ_wrtstr(smq, ",\"name\":\"");
      SharkMQ_wrtstr(smq, linfo[i].name);
      SharkMQ_wrtstr(smq, "\",\"color\":\"");
      SharkMQ_wrtstr(smq, ledType2String(linfo[i].color));
      SharkMQ_wrtstr(smq, "\",\"on\":");
      SharkMQ_wrtstr(smq, getLedState(linfo[i].id)?"true":"false");
      SharkMQ_wrtstr(smq, "}");
   }
#ifdef ENABLE_TEMP
   SharkMQ_wrtstr(smq, "],\"temp\":");
   ptr = &buf[(sizeof buf) - 1];
   *ptr = 0; 
   val = getTemp();
   if(val < 0)
   {
      val = -val;
      i=TRUE;
   }
   else
      i=FALSE;
   do { /* convert number to string */
      int r = val % 10U;
      val /= 10U;
      *--ptr = (char)('0' + r);
   } while (val && ptr > buf);
   if(i)
      *--ptr='-';
   SharkMQ_wrtstr(smq, ptr); /* number converted to string */
   SharkMQ_wrtstr(smq, "}");
#else
   SharkMQ_wrtstr(smq, "]}");
#endif
   return SharkMQ_pubflush(smq,tid,subtid);
}

/*
  Calculate SHA1(password + salt) and return value in out param sha1Digest

  The online broker does not use authentication. This function simply
  shows how one can create seeded hash based authentication. You can
  use any type of hash algorithm as long as the same hash algorithm is
  used on the server side.

  See the online security documentation for details:
  https://realtimelogic.com/ba/doc/?url=SMQ-Security.html

  Nonce: Details: https://en.wikipedia.org/wiki/Cryptographic_nonce

*/
void calculateSaltedPassword(U32 nonce, U8 sha1Digest[20])
{
   /* The device secret */
   const char* password="qwerty";

   SharkSslSha1Ctx ctx;
   U8 nonceData[4];
   SharkSslSha1Ctx_constructor(&ctx);
   /* Add salt (aka nonce) value provided by server and generate the salted
    * password hash.
    */
   nonceData[0] = (U8)nonce;
   nonceData[1] = (U8)(nonce >> 8);
   nonceData[2] = (U8)(nonce >> 16);
   nonceData[3] = (U8)(nonce >> 24);
   SharkSslSha1Ctx_append(&ctx, (U8*)password, strlen(password));
   SharkSslSha1Ctx_append(&ctx, nonceData, 4);
   SharkSslSha1Ctx_finish(&ctx, sha1Digest);
}


/*
  The M2M-LED main function does not return unless the code cannot
  connect to the broker or the connection should break. The return
  value 0 means that the caller can attempt to reconnect by calling
  this function again.
 */
static int
m2mled(SharkMQ* smq, SharkSslCon* scon,
       const char* smqUniqueId, int smqUniqueIdLen)
{
   U32 displayTid=0;    /* Topic ID (Tid) for "/m2m/led/display" */
   U32 ledSubTid=0;     /* Sub Topic ID for "led" */
   U32 deviceTid=0;     /* Tid for "/m2m/led/device" */
   U32 devInfoSubTid=0; /* Sub Topic ID for "devinfo" */
#ifdef ENABLE_TEMP
   U32 tempTid=0;       /* Tid for "/m2m/temp" */
   S16 temperature = (S16)getTemp(); /* current temperature */
#endif
   char ipaddr[16];
   U32 nonce; /* Sent by server and received when SharkMQ_init() returns */
   U8 sha1Digest[20]; /* See comment in function calculateSaltedPassword() */

/* We make it possible to override the URL at the command prompt when
 * in simulation mode.
 */
#if HOST_PLATFORM && !defined(NO_MAIN)
   const char* str=simpleMqUrl;
#else
   const char* str=SMQ_URL;
#endif
#ifndef SMQ_SEC
   (void)scon;
#endif

   xprintf(("Connecting to %s\n", str));
   smq->timeout = 3000; /* Bail out if the connection takes this long */
   setProgramStatus(ProgramStatus_Connecting);
   if(SharkMQ_init(smq, scon, str, &nonce) < 0)
   {
      xprintf(("Cannot establish connection, status: %d\n", smq->status));
      switch(smq->status)
      {
         case -1:
            str="Socket error!";
            setProgramStatus(ProgramStatus_SocketError);
            break;
         case -2:
            str="Cannot resolve IP address for " SMQ_DOMAIN ".";
            setProgramStatus(ProgramStatus_DnsError);
            break;
         case SMQE_INVALID_URL:
            str="Invalid URL.";
            setProgramStatus(ProgramStatus_ConnectionError);
            break;

#ifdef ENABLE_SOCKS_PROXY
         case E_PROXY_AUTH: str="E_PROXY_AUTH"; break;
         case E_PROXY_GENERAL: str="E_PROXY_GENERAL"; break;
         case E_PROXY_NOT_ALLOWED: str="E_PROXY_NOT_ALLOWED"; break;
         case E_PROXY_NETWORK: str="E_PROXY_NETWORK"; break;
         case E_PROXY_HOST: str="E_PROXY_HOST"; break;
         case E_PROXY_REFUSED: str="E_PROXY_REFUSED"; break;
         case E_PROXY_TTL: str="E_PROXY_TTL"; break;
         case E_PROXY_COMMAND_NOT_SUP: str="E_PROXY_COMMAND_NOT_SUP"; break;
         case E_PROXY_ADDRESS_NOT_SUP: str="E_PROXY_ADDRESS_NOT_SUP"; break;
         case E_PROXY_NOT_COMPATIBLE: str="E_PROXY_NOT_COMPATIBLE"; break;
         case E_PROXY_UNKNOWN: str="E_PROXY_UNKNOWN"; break;
         case E_PROXY_CLOSED: str="E_PROXY_CLOSED"; break;
         case E_PROXY_CANNOTCONNECT: str="E_PROXY_CANNOTCONNECT"; break;
#endif
         default:
            str="Cannot connect to " SMQ_DOMAIN ".";
            setProgramStatus(ProgramStatus_ConnectionError);
      }
      xprintf(("%s\n", str));
      /* Cannot reconnect if any of the following are true: */
      return
         smq->status==SMQE_BUF_OVERFLOW ||
         smq->status==SMQE_INVALID_URL ||
         smq->status <= -1000; /* Proxy error codes */
   }
#ifdef SMQ_SEC
#if SHARKSSL_CHECK_DATE
   else if(smq->status != SharkSslConTrust_CertCnDate)
#else
   else if (smq->status != SharkSslConTrust_CertCn)
#endif
   {
      setProgramStatus(ProgramStatus_CertificateNotTrustedError);
      xprintf(("%cWARNING: certificate received from %s not trusted!\n",
               7,SMQ_DOMAIN));
   }
#endif

   /* Broker does not authenticate. See comment in calculateSaltedPassword. */
   calculateSaltedPassword(nonce, sha1Digest); /* Store pwd in sha1Digest */

   /* Fetch the IP address sent by the broker. We use this for the
    * text shown in the left pane tab in the browser's user interface.
    */
   strncpy(ipaddr, (char*)smq->buf, 16);
   ipaddr[15]=0;
   if(SharkMQ_connect(smq,
                      smqUniqueId, smqUniqueIdLen,
                      (char*)sha1Digest, sizeof(sha1Digest), /* credentials */
                      getDevName(), strlen(getDevName()),1420))
   {
      xprintf(("Connect failed, status: %d\n", smq->status));
      return smq->status == SMQE_BUF_OVERFLOW || smq->status > 0;
   }

   xprintf(("\nConnected to: %s\n"
            "Use a browser and navigate to this domain.\n\n",
            str));
   setProgramStatus(ProgramStatus_DeviceReady);

   /* Request broker to return a topic ID for "/m2m/led/device", the
    * topic where we publish the device capabilities as JSON data.
    * Topic names and IDs: https://realtimelogic.com/ba/doc/?url=SMQ.html#TopicNames
    */
   SharkMQ_create(smq, "/m2m/led/device");

#ifdef ENABLE_TEMP
   /* Request broker to return a topic ID for "/m2m/temp", the
    * topic where we publish the device temperature.
    */
   SharkMQ_create(smq, "/m2m/temp");
#endif

   /** Request broker to create two subtopic IDs. We use the
    * subtopic IDs to further refine the messages published to the
    * browser(s).
    * Subtopics: https://realtimelogic.com/ba/doc/?url=SMQ.html#SubTopics
    */
   SharkMQ_createsub(smq, "devinfo");
   SharkMQ_createsub(smq, "led");

   /* Subscribe to browser "hello" messages. We send the device
    * capabilities as JSON data to the browser's ephemeral ID when we
    * receive a hello message from a browser.
    */
   SharkMQ_subscribe(smq, "/m2m/led/display");

   smq->timeout=50; /* Poll so we can locally update the LED buttons. */
   for(;;)
   {
      U8* msg;
      U8 outData[2];
      int len =  SharkMQ_getMessage(smq, &msg);
      if(len < 0) /* We received a control message or an error code */
      {
         switch(len)
         {  
            /* Control messages */
            case SMQ_TIMEOUT:
            {
               int x; /* used for storing ledId and temperature */
               int on;
               if(setLedFromDevice(&x,&on)) /* If a local button was pressed */
               { /* Publish to all subscribed browsers and set the LED on/off */
                  outData[0] = (U8)x; /* x is ledId */
                  outData[1] = (U8)on;
                  /* Publish to "/m2m/led/device", subtopic "led" */
                  SharkMQ_publish(smq, outData, 2, deviceTid, ledSubTid);
                  setLed(x, on); /* set the LED on/off */
               }
#ifdef ENABLE_TEMP
               x = getTemp();
               if(x != (int)temperature)
               {
                  temperature = (S16)x;
                  outData[0] = (U8)(temperature >> 8);
                  outData[1] = (U8)temperature;
                  SharkMQ_publish(smq, outData, 2, tempTid, 0);
               }
#endif
            }
            break;

            /* Manage responses for create, createsub, and subscribe */
            case SMQ_SUBACK: /* ACK: "/m2m/led/display" */
               displayTid = smq->ptid;
               break;
            case SMQ_CREATEACK:  /* ACK: "/m2m/temp" or "/m2m/led/device" */
#ifdef ENABLE_TEMP
               if( ! strcmp("/m2m/temp", (char*)msg ) )
                  tempTid = smq->ptid;
               else
#endif
               {
                  deviceTid = smq->ptid;
                  SharkMQ_observe(smq, deviceTid);
               }
               break;

            case SMQ_CREATESUBACK: /* We get a total of two messages */
                /* We get two suback messages ("devinfo" and "led") */
               if( ! strcmp("led", (char*)msg ) )
               {  /* Ack for: SMQ_createsub(smq, "led"); */
                  ledSubTid = smq->ptid;
               }
               else /* Must be ACK for devinfo */
               {  /* Ack for: SMQ_createsub(smq, "devinfo"); */
                  baAssert( ! strcmp("devinfo", (char*)msg) );
                  baAssert(deviceTid); /* acks are in sequence */
                  devInfoSubTid = smq->ptid;
                  /* We have sufficient info for publishing the device
                     info message.  All connected browsers will
                     receive this message and update their UI accordingly.
                  */
                  sendDevInfo(smq, ipaddr, deviceTid, devInfoSubTid);
               }
               break;

            case SMQ_SUBCHANGE:
               xprintf(("Connected browsers: %d\n", smq->status));
               break;

            /* Error codes */

            case SMQE_DISCONNECT: 
               xprintf(("Disconnect request from server\n"));
               setProgramStatus(ProgramStatus_CloseCommandReceived);
               return -1;

            case SMQE_BUF_OVERFLOW:
               xprintf(("Increase buffer size\n"));
               setProgramStatus(ProgramStatus_MemoryError);
               return -1; /* Must exit */

            default:
               xprintf(("Rec Error: %d.\n",smq->status));
               setProgramStatus(ProgramStatus_InvalidCommandError);
               return 0;
         }
      }
      else
      {
         if(smq->tid == displayTid) /* topic "display" */
         {
            /* Send device info to the new display unit: Send to
             * browser's ephemeral ID (ptid).
             */
            sendDevInfo(smq, ipaddr, smq->ptid, devInfoSubTid);
         }
         else if(smq->tid == smq->clientTid) /* sent to our ephemeral tid */
         {
            if(setLed(msg[0], msg[1]))
            {
               xprintf(("ptid %X attempting to set invalid LED %d\n",
                        smq->ptid, msg[0]));
               return 0;
            }
            /* Update all display units */
            outData[0] = msg[0];
            outData[1] = msg[1];
            /* Publish to "/m2m/led/device", subtopic "led" */
            SharkMQ_publish(smq, outData, 2, deviceTid, ledSubTid);
         }
         else
         {
            xprintf(("Received unknown tid %X\n", smq->tid));
            return 0;
         }
      }
   }
}


/* Force cipher: TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256
   TLS_ECDHE_ECDSA: See end of comment above for: sharkSslCAList (Ref-CA)
   CHACHA20_POLY1305_SHA256: https://ieeexplore.ieee.org/document/7927078
 */
#if SHARKSSL_ENABLE_SELECT_CIPHERSUITE == 1
static void
setChaChaCipher(SharkSslCon* scon)
{
   if(scon)
      SharkSslCon_selectCiphersuite(
         scon, TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256);
}
#else
#define setChaChaCipher(scon)
#endif


void
mainTask(SeCtx* ctx)
{
#ifdef SMQ_SEC
   static SharkSsl sharkSsl;
#endif
   static SharkSslCon* scon;
   static SharkMQ smq;
   static U8 buf[127];

   const char* smqUniqueId;
   int smqUniqueIdLen;
   smqUniqueIdLen = getUniqueId(&smqUniqueId);
   if(smqUniqueIdLen < 1)
   {
      xprintf(("Cannot get unique ID: aborting.\n"));
      setProgramStatus(ProgramStatus_SocketError);
      return;
   }

#ifdef SMQ_SEC
   SharkSsl_constructor(&sharkSsl,
                        SharkSsl_Client, /* Two options: client or server */
                        0,      /* Not using SSL cache */
#if DUPTR==U64 /* if 64 bit CPU: larger buffer required */
                        2000,   /* initial inBuf size: Can grow */
                        3000      /* outBuf size: Fixed */
#else
                        2000,   /* initial inBuf size: Can grow */
                        2000      /* outBuf size: Fixed */
#endif
      );
#endif

   /* (Ref-CA)
      The following construction force the server to authenticate itself by
      using an Elliptic Curve Certificate.

      CA_RTL_EC_256.pem is a Certificate Authority (CA) certificate
      and we use this certificate to validate that we are in fact
      connecting to simplemq.com and not someone pretending to
      be this server.

      The online server simplemq.com has two certificates installed:
      one standard RSA certificate, which you can see if you navigate
      to https://simplemq.com, and one Elliptic Curve Certificate
      (ECC).

      The online server is configured to favor the RSA certificate
      which will be presented to the client unless the client tells
      the server that it can only use ECC, at which point the server
      is forced to send its ECC certificate to the client.

      The following construction makes sure we only use ECC by either
      using a SharkSSL library compiled with ECC support only (no RSA)
      or by specifically setting a cipher using ECC.

      See the following link for more information:
      realtimelogic.com/ba/doc/en/C/shark/md_md_Certificate_Management.html
      */
   SharkSsl_setCAList(&sharkSsl, sharkSSL_New_RTL_ECC_CA);

   SharkMQ_constructor(&smq, buf, sizeof(buf));
   SharkMQ_setCtx(&smq, ctx);  /* Required for bare-metal env. */
   setProxy(&smq); /* Implemented for host build (SOCKS5 or HTTP proxy) */

   /* It is very important to seed the SharkSSL RNG generator */
   sharkssl_entropy(baGetUnixTime() ^ (ptrdiff_t)&sharkSsl);
   scon = SharkSsl_createCon(&sharkSsl);
   setChaChaCipher(scon);
   setProgramStatus(ProgramStatus_Starting);
   while(scon && ! m2mled(&smq, scon, smqUniqueId, smqUniqueIdLen) )
   {
      xprintf(("Closing connection; status: %d\n",smq.status));
      SharkMQ_disconnect(&smq);
      SharkSsl_terminateCon(&sharkSsl, scon);
      scon = SharkSsl_createCon(&sharkSsl);
      setChaChaCipher(scon);
      setProgramStatus(ProgramStatus_Restarting);
   }
   SharkMQ_destructor(&smq);
   if(scon)
      SharkSsl_terminateCon(&sharkSsl, scon);
   else
      setProgramStatus(ProgramStatus_MemoryError);
   SharkSsl_destructor(&sharkSsl);
}
