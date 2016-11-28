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
 *   $Id: m2m-led.c 3906 2016-08-18 20:54:15Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2016
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
  
   SharkMQ (secure SMQ) LED example.


   NOTE: This example uses the SharkMQ (secure SMQ) compatibility API
         thus making it easy to upgraded to a secure version, if needed.

   This code is the device side for the SMQ LED controller, an example
   that enables LEDs in a device to be controlled from a browser and
   Java (including Android).

   When this code runs, it connects to our public SMQ test broker. The
   device will show up as a tab on the following page:
   http://simplemq.com/m2m-led/

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
   simulation mode.
 */
#define SIMPLEMQ_DOMAIN "http://simplemq.com"
#define SIMPLEMQ_URL SIMPLEMQ_DOMAIN "/smq.lsp"

#include "SMQClient.h"
#include "ledctrl.h"
#include <ctype.h>
#include <stdlib.h>



/****************************************************************************
 **************************-----------------------***************************
 **************************| BOARD SPECIFIC CODE |***************************
 **************************-----------------------***************************
 ****************************************************************************/

#if HOST_PLATFORM

static const char* simpleMqUrl; /* defaults to SIMPLEMQ_DOMAIN */

#ifdef _WIN32
// For calculating unique ID
#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")
#endif

#include <stdio.h>


/* Enable simulated temperature */
#ifndef ENABLE_TEMP
#define ENABLE_TEMP
#endif

/*
  The following is used by the logic managing the simulated temperature.
 */
#define KEY_UP_ARROW 1000
#define KEY_DOWN_ARROW 1001
static int currentTemperature=0; /* simulated value */

/* Replace with a function that prints to a console or create a stub
 * that does nothing.
 */
void
_xprintf(const char* fmt, ...)
{
   va_list varg;
   va_start(varg, fmt);
   vprintf(fmt, varg);
   va_end(varg);
}


/* Not needed on host with printf support. This function is designed for
 * embedded systems without console.
 */
void setProgramStatus(ProgramStatus s)
{
   (void)s;
}



/* The list of LEDs in the device, the name, color, and ID. Adapt this
   list to the LEDs on your evaluation board (Ref-LED).

   The LED name shows up in the UI, the LED type tells the UI the
   color of the LED, and the ID is used as a handle by the UI. The UI
   will send this handle to the device when sending LED control
   messages. You can use any number sequence for the ID. We use the
   sequence 1 to 4 so it's easy to map to a C array.

   This data is encoded as JSON and sent to the UI when a new UI
   requests the device capabilities.
*/
static const LedInfo ledInfo[] = {
   {
      "LED 1",
      LedColor_red,
      1
   },
   {
      "LED 2",
      LedColor_yellow,
      2
   },
   {
      "LED 3",
      LedColor_green,
      3
   },
   {
      "LED 4",
      LedColor_blue,
      4
   }
};

const LedInfo*
getLedInfo(int* len)
{
   *len = sizeof(ledInfo) / sizeof(ledInfo[0]);
   return ledInfo;
}

/* The LEDs: used by getLedState and setLed
 */
static int leds[sizeof(ledInfo)/sizeof(ledInfo[1])];

/* Returns the LED on/off state for led with ID 'ledId'. The 'ledId'
   is the 'handle' sent by the UI.
 */
int
getLedState(int ledId)
{
   baAssert(ledId >= 1 && ledId <= sizeof(ledInfo)/sizeof(ledInfo[1]));
   return leds[ledId-1];
} 


/* Set LED on device. The 'ledId' is the 'handle' sent by the UI.
 */
int
setLed(int ledId, int on)
{
   if(ledId >= 1 && ledId <= sizeof(ledInfo)/sizeof(ledInfo[1]))
   {
      printf("Set LED %d %s\n", ledId, on ? "on" : "off");
      leds[ledId-1] = on;
      return 0;
   }
   return -1;
}


/* The optional function setLedFromDevice requires non blocking
 * keyboard I/O in the simulated version. The following code sets this
 * up for WIN and UNIX. Remove this code for embedded use and change
 * setLedFromDevice as explained below.
 */
#include <ctype.h>
#ifdef _WIN32

#include <conio.h>
#define xkbhit _kbhit

static int
xgetch()
{
   int c = _getch();
   if(c == 224)
   {
      switch(_getch())
      {
         case 72: return KEY_UP_ARROW;
         case 80: return KEY_DOWN_ARROW;
      }
      return 'A'; /* dummy value */
   }
   return c;

}


#else

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <termios.h>
/* UNIX kbhit and getch simulation */
static struct termios orgTs;

static void
resetTerminalMode()
{
   tcsetattr(0, TCSANOW, &orgTs);
}

static void
setConioTerminalMode()
{
   struct termios asyncTs;
   tcgetattr(0, &orgTs);
   memcpy(&asyncTs, &orgTs, sizeof(asyncTs));
   /* register cleanup handler, and set the new terminal mode */
   atexit(resetTerminalMode);
   cfmakeraw(&asyncTs);
   asyncTs.c_oflag=orgTs.c_oflag;
   tcsetattr(0, TCSANOW, &asyncTs);
}

static int
xkbhit()
{
   struct timeval tv = { 0L, 0L };
   fd_set fds;
   FD_ZERO(&fds);
   FD_SET(0, &fds);
   return select(1, &fds, NULL, NULL, &tv);
}

static int
xgetch()
{
   int r;
   unsigned char c;
   if ((r = read(0, &c, sizeof(c))) < 0)
      return r;
   if(c == 3) /* CTRL-C Linux */
      exit(0);
   if(c == 27)
   {
      U16 x;
      read(0, &x, sizeof(x));
      switch(x)
      {
         case 0x415B: return KEY_UP_ARROW;
         case 0x425B: return KEY_DOWN_ARROW;
      }
      return 'A'; /* dummy value */
   }
   return c;
}

static void
die(const char* fmt, ...)
{
   va_list varg;
   va_start(varg, fmt);
   vprintf(fmt, varg);
   va_end(varg);
   printf("\n");
   exit(1);
}

static void
getMacAddr(char macaddr[6], const char* ifname)
{
   char buf[8192] = {0};
   struct ifconf ifc = {0};
   struct ifreq *ifr = NULL;
   int sck = 0;
   int nInterfaces = 0;
   int i = 0;
   struct ifreq *item;
   struct sockaddr *addr;
   /* Get a socket handle. */
   sck = socket(PF_INET, SOCK_DGRAM, 0);
   if(sck < 0) 
      die("socket: %s",strerror(errno));
   /* Query available interfaces. */
   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = buf;
   if(ioctl(sck, SIOCGIFCONF, &ifc) < 0) 
      die("ioctl(SIOCGIFCONF) %s", strerror(errno));
   /* Iterate through the list of interfaces. */
   ifr = ifc.ifc_req;
   nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
   for(i = 0; i < nInterfaces; i++) 
   {
      unsigned long ipaddr;
      item = &ifr[i];
      addr = &(item->ifr_addr);
      /* Get the IP address*/
      if(ioctl(sck, SIOCGIFADDR, item) < 0) 
      {
         perror("ioctl(OSIOCGIFADDR)");
         continue;
      }
      ipaddr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
      if(0x100007F == ipaddr || 0 == ipaddr)
         continue;
      /* Get the MAC address */
      if(ioctl(sck, SIOCGIFHWADDR, item) < 0) {
         perror("ioctl(SIOCGIFHWADDR)");
         continue;
      }
      break;
   }
   close(sck);   
   if(i == nInterfaces)
      die("Cannot get a MAC address\n");
   memcpy(macaddr, item->ifr_hwaddr.sa_data, 6);
}
#endif
/* Endif UNIX/Linux specific code */


/* Optional function that can be used to turn an LED on/off from the
   device by using buttons. The function must return TRUE if a button
   was pressed, otherwise FALSE must be returned. The LED state on/off
   information is managed by the online web service. 
*/
int
setLedFromDevice(int* ledId, int* on)
{
   int ledLen;
   const LedInfo* ledInfo = getLedInfo(&ledLen);
   if(xkbhit())
   {
      int base,i;
      int c = xgetch();
      if(c == KEY_UP_ARROW)
      {
         currentTemperature += 8; /* increment by 0.8 celcius */
         return 0;
      }
      if(c == KEY_DOWN_ARROW)
      {
         currentTemperature -= 8; /* decrement by 0.8 celcius */
         return 0;
      }
      if(isupper(c))
      {
         *on=1;
         base='A';
      }
      else
      {
         *on=0;
         base='a';
      }
      c -= base;
      base=0;
      for(i = 0 ; i < ledLen ; i++)
      {
         if(ledInfo[i].id == c)
         {
            base=1;
            break;
         }
      }
      if(base)
      {
         *ledId = c;
         return 1;
      }
      printf("Invalid LedId %d. Valid keys (upper/lower): ",c);
      for(i = 0 ; i < ledLen ; i++)
         printf("%c ",'A'+ledInfo[i].id);
      printf("\n");
   }

   { /* Print out usage info at startup */
      static int oneshot=0;
      if( ! oneshot )
      {
         oneshot = 1;
         xprintf(
            ("Set LED from keyboard. Uppercase = ON, lowercase = OFF.\n"
             "Switching LED state updates UI in all connected browsers.\n"));
      }
   }
   return 0;
}


int getTemp(void)
{
   return currentTemperature;
}


const char* getDevName(void)
{
   static char devInfo[100];
   char* ptr;
   strcpy(devInfo,"Simulated Device: ");
   ptr = devInfo + strlen(devInfo);
   gethostname(ptr, 100 - (ptr - devInfo));
   return devInfo;
}


static void printUniqueID(const char* uuid, int uuidLen)
{
   int i;
   printf("UUID: ");
   for(i = 0 ; i < uuidLen ; i++)
      printf("%X ", (unsigned int)((U8)uuid[i]));
   printf("\n");
}


int getUniqueId(const char** id)
{
#ifdef _WIN32
   UUID winid;
   static char uuid[8];
   if(RPC_S_OK == UuidCreateSequential(&winid))
      memcpy(uuid,winid.Data4,8);
   else
      memset(uuid,0,8);
   printUniqueID(uuid, 8);
   *id = uuid;
   return 8;
#else
   static char addr[6];
   getMacAddr(addr, "eth0");
   printUniqueID(addr, 6);
   *id = addr;
   return 6;
#endif
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
   WSADATA wsaData;
   /* Windows specific: Start winsock library */
    WSAStartup(MAKEWORD(1,1), &wsaData);
#else
   setConioTerminalMode();
#endif

   if(argc > 1)
   {
      simpleMqUrl = argv[1];
      xprintf(("Overriding broker URL\n\tfrom %s\n\tto %s\n%s\n\n",
               SIMPLEMQ_URL,simpleMqUrl,
               "Note: error messages will include original broker name"));

   }
   else
   {
      simpleMqUrl = SIMPLEMQ_URL;
   }

   mainTask(0);
   printf("Press return key to exit\n");
   getchar();
   return 0;
}

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
#if HOST_PLATFORM
   const char* str=simpleMqUrl;
#else
   const char* str=SIMPLEMQ_URL;
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
            str="Cannot resolve IP address for " SIMPLEMQ_DOMAIN ".";
            setProgramStatus(ProgramStatus_DnsError);
            break;
         default:
            str="Cannot connect to " SIMPLEMQ_DOMAIN ".";
            setProgramStatus(ProgramStatus_ConnectionError);
            break;
      }
      xprintf(("%s\n", str));
      /* Cannot reconnect if any of the following are true: */
      return smq->status==SMQE_BUF_OVERFLOW || smq->status==SMQE_INVALID_URL;
   }

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

   xprintf(("\nConnected to %s.\n"
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
   static SharkSsl sharkSsl;
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

   SharkSsl_constructor(&sharkSsl,
                        SharkSsl_Client, /* Two options: client or server */
                        0,      /* Not using SSL cache */
                        2000,   /* initial inBuf size: Can grow */
                        2000);  /* outBuf size: Fixed */


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

   /* It is very important to seed the SharkSSL RNG generator */
   sharkssl_entropy(baGetUnixTime() ^ (U32)&sharkSsl);
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
