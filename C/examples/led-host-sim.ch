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
 *   $Id$
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2021
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
 *               http://sharkssl.com
 ****************************************************************************
 */

 
/*

Implements a simulated host environment for the functions in ledctrl.h
See this header file for detailed explanation.

 */

#ifdef _WIN32
// For calculating unique ID
#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")
#endif

#include <stdio.h>


/*
  The following is used by the logic managing the simulated temperature.
 */
#define KEY_UP_ARROW 1000
#define KEY_DOWN_ARROW 1001
static int currentTemperature=200; /* simulated value */

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
   See function setLed below for details on the 'leds' array.
 */
int
getLedState(int ledId)
{
   baAssert(ledId >= 1 && ledId <= sizeof(ledInfo)/sizeof(ledInfo[1]));
   return leds[ledId-1];
} 


/* Command sent by UI client to turn LED with ID on or off.
   This function must set the LED to on if 'on' is TRUE and off if
   'on' is FALSE.  The parameter 'LedId' is the value of the field
   'id' in the corresponding 'LedInfo' structure.  Code might be
   required to match the 'id' value to the physical LED and entry in
   the LED array.  In this example, 'id' field is numbered from 1
   through 4, with 'id' 1 referring to the first entry in the array of
   LedInfo structures. The expression (LedId - 1), results in the
   correct index.
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

#if __APPLE__
#define HAVE_GETIFADDRS
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#elif __linux__
#define HAVE_SIOCGIFHWADDR
#endif

/* For backward comp. */
#define setConioTerminalMode()

static int
xkbhit()
{
   struct timeval tv = { 0L, 0L };
   fd_set fds;
   struct termios orgTs;
   struct termios asyncTs;
   int set;
   tcgetattr(0, &orgTs);
   memcpy(&asyncTs, &orgTs, sizeof(asyncTs));
   cfmakeraw(&asyncTs);
   asyncTs.c_oflag=orgTs.c_oflag;
   tcsetattr(0, TCSANOW, &asyncTs);
   FD_ZERO(&fds);
   FD_SET(STDIN_FILENO, &fds);
   set = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
   tcsetattr(0, TCSANOW, &orgTs);
   return set;
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
      if(read(0, &x, sizeof(x)) > 0)
      {
         switch(x)
         {
            case 0x415B: return KEY_UP_ARROW;
            case 0x425B: return KEY_DOWN_ARROW;
         }
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
#if defined(HAVE_SIOCGIFHWADDR)
   char buf[8192] = {0};
   struct ifconf ifc = {0};
   struct ifreq *ifr = NULL;
   int sck = 0;
   int nInterfaces = 0;
   int i = 0;
   struct ifreq *item=0;
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
      die("Cannot find a MAC address\n");
   memcpy(macaddr, item->ifr_hwaddr.sa_data, 6);
#elif defined(HAVE_GETIFADDRS)
   struct ifaddrs* iflist;
   BaBool found = FALSE;
   if (getifaddrs(&iflist) == 0)
   {
      struct ifaddrs* cur;
      for (cur = iflist; cur; cur = cur->ifa_next)
      {
         if ((cur->ifa_addr->sa_family == AF_LINK) &&
             (strcmp(cur->ifa_name, "lo0") == 0) &&
             cur->ifa_addr)
         {
            struct sockaddr_dl* sdl;
            sdl = (struct sockaddr_dl*)cur->ifa_addr;
            memcpy(macaddr, LLADDR(sdl), sdl->sdl_alen);
            found = TRUE;
            break;
         }
      }
      freeifaddrs(iflist);
   }
   if( ! found )
      die("Cannot find a MAC address\n");
#else
#error No MAC address on this platform?
#endif



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
   const LedInfo* linfo = getLedInfo(&ledLen);
   if(xkbhit())
   {
      int base,i;
      int c = xgetch();
      if(c == KEY_UP_ARROW)
      {
         currentTemperature += 8; /* increment by 0.8 celcius */
         if(currentTemperature > 1000)
            currentTemperature=1000;
         return 0;
      }
      if(c == KEY_DOWN_ARROW)
      {
         currentTemperature -= 8; /* decrement by 0.8 celcius */
         if(currentTemperature < 0)
            currentTemperature=0;
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
         if(linfo[i].id == c)
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
         printf("%c ",'A'+linfo[i].id);
      printf("\n");
   }

   { /* Print out usage info at startup */
      static int oneshot=0;
      if( ! oneshot )
      {
         oneshot = 1;
         xprintf(
            ("\nled-host-sim.ch:\n"
             "Set LED from keyboard. Uppercase = ON, lowercase = OFF.\n\n"));
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
#ifndef NO_MAIN
   ptr = devInfo + strlen(devInfo);
   gethostname(ptr, 100 - (ptr - devInfo));
#endif
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

/* Use MAC address as unqique SMQ device ID.
 */
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
