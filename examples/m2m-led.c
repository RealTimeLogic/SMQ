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
 *   $Id: m2m-led-SharkMQ.c 4340 2018-12-06 22:56:56Z wini $
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
  
   SMQ LED example.

   This code is the device side for the SMQ LED controller, an example
   that enables LEDs in a device to be controlled from a browser and
   Java (including Android).

   When this code runs, it connects to our public SMQ test broker. The
   device will show up as a tab on the following page:
   https://simplemq.com/m2m-led/

   Introductory information on how the complete SMQ LED demo works
   (including the browser UI) can be found online. The device code
   details can be found under section " Device C code".
   https://goo.gl/phXnWp

   The code is designed for embedded devices, but can also be run on a
   host computer (Windows/Linux) as a simulated device with four
   simulated LEDs.

   The macro HOST_PLATFORM must be defined if compiled and run on a
   non embedded platform (HLOS). The code within this section sets up the
   simulated environment. Do not set this macro if the code is to be
   cross compiled for an embedded device.

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
   * int getUniqueId(const char** id); unless GETUNIQUEID is defined
   * xprintf (selib.h)

   See the following documentation for how to interface the LED
   functions to the hardware:
   https://realtimelogic.com/ba/doc/en/C/shark/md_md_Examples.html#LedDemo

   The above functions are documented in ledctrl.h. You may also study
   the simulated versions in the HOST_PLATFORM code section
   below. Function xprintf is optional and used if macro XPRINTF=1. If
   enabled, program status is printed during operation.

   A ready to use (non secure) mbed version can be found on the ARM
   mbed web site: https://goo.gl/Rnhp3b

   To set up your own SMQ broker (server), download the Mako Server
   and activate the tutorials. A ready to use broker is included in
   the tutorials. The following non active copy of the tutorials
   provide information on how the broker operates:
   https://realtimelogic.com/bas-tutorials/IoT.lsp
 */


/* Change the domain/url if you are running your own broker
   Note: you can also set the domain at the command prompt when in
   simulation mode. The default settings connect to our online SMQ cluster.

Examples:
   When running this program and Mako Server examples on the same server:
   Mako examples: https://makoserver.net/documentation/manual/
   #define SMQ_DOMAIN "https://localhost"

   When running this program on an ESP8266 and connecting to the BAS
   HW eval kit:
      https://realtimelogic.com/downloads/docs/BAS-Linkit-Getting-Started.pdf
   #define SMQ_DOMAIN "https:/barracuda.local/"

   Note: this example can be compiled with SharkMQ or standard SMQ
   Sec: https://realtimelogic.com/ba/doc/en/C/shark/group__SMQLib.html
   Std: https://realtimelogic.com/ba/doc/en/C/reference/html/structSMQ.html
   This example uses the SharkMQ compatibility API when compiled for
   standard SMQ, thus making it easy to upgraded to a secure version,
   if needed. The macro SMQ_SEC is defined when SMQ.h is the SharkMQ
   (secure) version.
 */
#include "SMQ.h"
#define SMQ_DOMAIN "simplemq.com"
#ifdef SMQ_SEC
#define SMQ_PROTOCOL "https://"
#else
#define SMQ_PROTOCOL "http://"
#endif
#define SMQ_URL SMQ_PROTOCOL SMQ_DOMAIN "/smq.lsp"

#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>

#include "ledctrl.h"

/*                   sharkSslCAList (Ref-CA)

The data from the include file below is a CA (Certificate Authority)
Root Certificate. The CA enables SharkSSL to verify the identity of
realtimelogic.com. We include data for only one CA in this example
since only the signer of realtimelogi.com's certificate is required
for validation in this example. See the "certcheck" example for how to
validate any type of certificate and for more options on how to store
certificates, in addition to embedding the CA as it is done in this
example.

The Certification Authority Root Certificate was
converted to 'sharkSslCAList' as follows:
SharkSSLParseCAList CA_RTL_EC_256.pem > CA_RTL_EC_256.h

An introduction to certificate management can be found here:
https://goo.gl/rjdQjg

*/
#ifdef SMQ_SEC
#include "certificates/CA_RTL_EC_256.h"
#endif


/****************************************************************************
 **************************-----------------------***************************
 **************************| BOARD SPECIFIC CODE |***************************
 **************************-----------------------***************************
 ****************************************************************************/

/*

The macro HOST_PLATFORM should be set if you are running the example
on Windows or Linux. The code below provides a simulated version of
the device functions found in ledctrl.h. See the function
documentation in this header file for details on how to provide a
LED/device porting layer for your device.

*/

#if HOST_PLATFORM

/* Include the simulated LED environment/functions */
#include "led-host-sim.ch"

static const char* simpleMqUrl=0; /* defaults to SMQ_DOMAIN */

/* Enable simulated temperature */
#ifndef ENABLE_TEMP
#define ENABLE_TEMP
#endif

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
#else
#define extractProxyArgs(arg) printUsage()
#endif

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
#else
   setConioTerminalMode();
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

#ifdef GETUNIQUEID
/*
  The following getUniqueId function can be enabled on development
  boards where you cannot fetch the MAC address or if the MAC address
  is not globally unique (not set from factory).

  The SMQ broker requires a persistent (non random) unique ID for each
  client. The broker uses this ID to detect board restarts, where the
  old connection still lingers in the broker. The ID must for this
  reason be the same when the board restarts. The following logic
  fetches the WAN address from a specially constructed service. This
  address is then combined with the local LAN address. The LAN address
  will most likely be the same when the board restarts, even with
  DHCP. The address may change if the board has been offline for a
  longer time, but in that case, the lingering connection in the
  broker will have been removed by the time the board restarts.
 */
int getUniqueId(const char** id)
{
   SOCKET sock;
   S32 len = -1;
   if( ! se_connect(&sock, "simplemq.com", 80) )
   {
      /* The following service is designed to send back the WAN IP
       * address without sending any HTTP headers.
       */
      static const char cmd[]={
         "GET /addr.lsp HTTP/1.0\r\nHost:simplemq.com\r\n\r\n"};
      static char uuid[40];
      se_send(&sock, cmd, sizeof(cmd)-1);
      len = se_recv(&sock, uuid, sizeof(uuid) - 16, 1000);
      if(len > 0)
      {
         int status;
         se_getSockName(&sock,uuid+len,&status);
         if( status > 1 )
         {
            *id = uuid;
            len += status;
#if HOST_PLATFORM
            printUniqueID(uuid, len);
#endif
         }
         else
            len=-1;
      }
      se_close(&sock);
   }
   return len;
}
#endif



/* Type to string conv. */
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
   const LedInfo* ledInfo = getLedInfo(&ledLen);

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
      val = ledInfo[i].id;
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
      SharkMQ_wrtstr(smq, ledInfo[i].name);
      SharkMQ_wrtstr(smq, "\",\"color\":\"");
      SharkMQ_wrtstr(smq, ledType2String(ledInfo[i].color));
      SharkMQ_wrtstr(smq, "\",\"on\":");
      SharkMQ_wrtstr(smq, getLedState(ledInfo[i].id)?"true":"false");
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
   if(SharkMQ_init(smq, scon, str, 0) < 0)
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
   else if(smq->status != SharkSslConTrust_CertCn)
   {
      setProgramStatus(ProgramStatus_CertificateNotTrustedError);
      xprintf(("%cWARNING: certificate received from %s not trusted!\n",
               7,SMQ_DOMAIN));
   }
#endif

   /* Fetch the IP address sent by the broker. We use this for the
    * text shown in the left pane tab in the browser's user interface.
    */
   strncpy(ipaddr, (char*)smq->buf, 16);
   ipaddr[15]=0;
   if(SharkMQ_connect(smq,
                      smqUniqueId, smqUniqueIdLen,
                      0, 0, /* credentials */
                      getDevName(), strlen(getDevName())))
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
    */
   SharkMQ_create(smq, "/m2m/led/device");

#ifdef ENABLE_TEMP
   /* Request broker to return a topic ID for "/m2m/temp", the
    * topic where we publish the device temperature.
    */
   SharkMQ_create(smq, "/m2m/temp");
#endif


   /** Request broker to create two sub-topic IDs. We use the
    * sub-topic IDs to further refine the messages published to the
    * browser(s).
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
      else if(len > 0)
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
            /* Publish to "/m2m/led/device", sub-topic "led" */
            SharkMQ_publish(smq, outData, 2, deviceTid, ledSubTid);
         }
         else
         {
            xprintf(("Received unknown tid %X\n", smq->tid));
            return 0;
         }
      }
      else /* timeout */
      {
         int x; /* used for storing ledId and temperature */
         int on;
         if(setLedFromDevice(&x,&on)) /* If a local button was pressed */
         {  /* Publish to all subscribed browsers and set the LED on/off */
            outData[0] = (U8)x; /* x is ledId */
            outData[1] = (U8)on;
            /* Publish to "/m2m/led/device", sub-topic "led" */
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
   }
}



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
                        3000,   /* initial inBuf size: Can grow */
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
   SharkSsl_setCAList(&sharkSsl, sharkSslCAList);

   SharkMQ_constructor(&smq, buf, sizeof(buf));
   SharkMQ_setCtx(&smq, ctx);  /* Required for non RTOS env. */
   setProxy(&smq); /* Implemented for host build */

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
