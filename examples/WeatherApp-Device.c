/* 
   Command line based simulated native-UI for:
   https://simplemq.com/WeatherApp/
   Documentation:
   https://simplemq.com/WeatherApp/doc/

   Change the domain/url if you are running your own broker
   Note: you can also set the domain at the command prompt when in
   simulation mode.
 */
#define SIMPLEMQ_DOMAIN "http://simplemq.com"


/* The path to the SMQ LSP page entry (where the connection is
 * upgraded from HTTP to SMQ)
 */
#define SIMPLEMQ_PATH "/WeatherApp/smq.lsp?client=device"

/* The complete URL. We must set the query string client=device. This
 * is used by the server side weather app (implemented in Lua) and
 * indicates that this connection is from a device and not a browser.
 */
#define SIMPLEMQ_URL SIMPLEMQ_DOMAIN SIMPLEMQ_PATH


/* Sub topic numbers hard coded by weather app server code. Do not
   change unless you also change the codes in the server's Lua code.
 */
#define MSG_SERVER_HELLO 1
#define MSG_GET_FORECAST 2
#define MSG_WEATHER_FORECAST 3
#define MSG_WEATHER_UPDATE 4
#define MSG_GET_TIMEZONE 5
#define MSG_SET_TIMEZONE 6
#define MSG_SET_THERMOSTAT 7
#define MSG_DEVCLIENTS 8
#define MSG_DEVICE_DISCONNECT 9

#define INVALID_TZOFFS ((S32)((~((U32)0))/2-1))


#include <time.h> /* gmtime */

#define XTYPES
#include <JVal.h>
#include <JEncoder.h>
#include <SMQ.h>

/* See the end of this file for info on using a static allocator with JSON.
   JParser allocates one buffer only.
 */
#define MAX_STRING_LEN 2048

typedef struct
{
   AllocatorIntf super;
   U8 buf[MAX_STRING_LEN];
} JParserAllocator;

typedef struct
{
   AllocatorIntf super;
   U8* buf;
   size_t bufsize; 
   size_t cursor;
} JPValFactAllocator;

void JParserAllocator_constructor(JParserAllocator* o);
#define JPValFactAllocator_reset(o) (o)->cursor=0

void JPValFactAllocator_constructor(
   JPValFactAllocator* o, void* b, size_t bsize);



/****************************************************************************
 **************************-----------------------***************************
 **************************| BOARD SPECIFIC CODE |***************************
 **************************-----------------------***************************
 ****************************************************************************/

/* The following code is for the simulated weather app client. The
   device code must implement the following functions:

   getUniqueId() or #define GETUNIQUEID
   setSystemTime()
   weatherForecastMsg()
   currentWeatherMsg()
   setThermostatTemp()
   setDevClientsConnected()

   See the simulated versions below for implementation notes.
*/


#if HOST_PLATFORM

static char* simpleMqUrl; /* defaults to SIMPLEMQ_DOMAIN */

#ifdef _WIN32
// For calculating unique ID
#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

/*
  The following is used by the logic managing the simulated temperature.
 */
#define KEY_UP_ARROW 1000
#define KEY_DOWN_ARROW 1001


int getUniqueId(const char** id)
{
#ifdef _WIN32
   UUID winid;
   static char uuid[8];
   if(RPC_S_OK == UuidCreateSequential(&winid))
      memcpy(uuid,winid.Data4,8);
   else
      memset(uuid,0,8);
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


/* Platform specific (Windows) function for starting the browser.
 */
static void startBrowser(void)
{
   char buf[200];
   char* ptr;
   const char* smqUniqueId;
   int smqUniqueIdLen = getUniqueId(&smqUniqueId);
   strcpy(buf, "start ");
   strcat(buf, simpleMqUrl);
   ptr = strrchr(buf, '/');
   if(ptr)
   {
      xprintf(("\nStarting browser!\n>>>>>>>>>   Device ID: "));
      strcpy(ptr, "/?di=");
      ptr+=5;
      for(int i=0; i < smqUniqueIdLen ; i++)
      {
         unsigned int x = (unsigned int)*((unsigned char*)smqUniqueId);
	      sprintf(ptr,"%02X",x);
         xprintf(("%c%c", ptr[1], ptr[0]));
         ptr+=2;
	      smqUniqueId++;
      }
      xprintf(("   <<<<<<<<<\n\n"));
      system(buf);
   }
}

#else

#define startBrowser(x)

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


static void printUniqueID(const char* uuid, int uuidLen)
{
   int i;
   printf("UUID: ");
   for(i = 0 ; i < uuidLen ; i++)
      printf("%X ", (unsigned int)((U8)uuid[i]));
   printf("\n");
}


int main(int argc, char* argv[])
{
#ifdef _WIN32
   WSADATA wsaData;
   /* Windows specific: Start winsock library */
    WSAStartup(MAKEWORD(1,1), &wsaData);
#endif

   if(argc > 1)
   {
      simpleMqUrl=malloc(sizeof(SIMPLEMQ_PATH)+strlen(argv[1])+10);
      sprintf(simpleMqUrl,"http://%s%s", argv[1],SIMPLEMQ_PATH);
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



/**************************  Simulated Weather App **************************/


static const char*
wday2String(S32 wday)
{
   switch(wday)
   {
      case 0: return "Sun";
      case 1: return "Mon";
      case 2: return "Tue";
      case 3: return "Wed";
      case 4: return "Thu";
      case 5: return "Fri";
      case 6: return "Sat";
   }
   return "??";
}


static const char*
month2String(S32 month)
{
   switch(month)
   {
      case 0: return "January";
      case 1: return "February";
      case 2: return "March";
      case 3: return "April";
      case 4: return "May";
      case 5: return "June";
      case 6: return "July";
      case 7: return "August";
      case 8: return "September";
      case 9: return "October";
      case 10: return "November";
      case 11: return "December";
   }
   return "??";
}


/* 
   The server fetched correct time from the atomic time server
   time.nist.gov, adjusts the time for the device's time zone, and
   sends the time to the device at regular intervals. The device must
   store this value and increment the value every second. Data can be
   translated to human readable values by using function gmtime (See
   example below). This data must be updated and displayed on the UI
   every second.
  
   Unix time info: https://en.wikipedia.org/wiki/Unix_time
 */
static void
setSystemTime(time_t secondsSinceJan1970)
{
   /* Time is timezone adjusted thus gmtime returns local time. */
   struct tm* t = gmtime(&secondsSinceJan1970);
   xprintf(("%d:%d:%d, %s %s %d %d\n",
            t->tm_hour,
            t->tm_min,
            t->tm_sec,
            wday2String(t->tm_wday),
            month2String(t->tm_mon),
            t->tm_mday,
            1900+t->tm_year));
}


/*
  The server sends weather forecast data to the device when the device
  connects to the server. Data is also pushed at regular intervals,
  but not as frequent as data pushed to function
  currentWeatherMsg(). The forecast also includes additional
  information such current weather info and the correct time. Note
  that this function must extract time and call setSystemTime().

  Data is sent from the server to the device as JSON. The JSON data is
  provided as a JVal JSON tree. See the documentation for JVal for how
  to extract data and how to use the utility function JVal_get.
  https://realtimelogic.com/ba/doc/en/C/reference/html/structJVal.html 
 */
static int
weatherForecastMsg(JVal* rootVal, S32* tzOffs)
{
   const char* city;
   const char* desc=0;
   S32 temp;
   U32 humidity,weatherId;
   U32 dt; /* date & time */
   JVal* jForecast[7]; /* Array for 7 day forecast */
   JErr err;
   int i;
   JErr_constructor(&err);
   JVal_get(rootVal, &err, "{s}", "err",&city);
   if(JErr_noError(&err))
   {
      xprintf(("Server reported error: %s\n", city));
      return -1;
   }
   else
      JErr_reset(&err);
   JVal_get(rootVal,
            &err,
            "{ssddddAJ}",
            "city",&city,
            "desc",&desc,
            "temp",&temp,
            "id", &weatherId,
            "humidity",&humidity,
            "time", &dt,
            "list",jForecast,7);
   if(JErr_noError(&err))
   {
      if(*tzOffs == INVALID_TZOFFS) /* if not set i.e. first time */
      {
         JVal_get(rootVal,
                  &err,
                  "{d}","tzoffs",tzOffs);
         JErr_reset(&err); /* The data may not be part of the JSON response */
      }

      xprintf(("\n-------------------------------\n"));
      xprintf(("Weather for %s\n",city));
      if(*tzOffs != INVALID_TZOFFS)
         setSystemTime(dt+*tzOffs); /* Set time adjusted for local time zone */
      temp = temp*9/50 + 32; /* celcius x 10 to F */
      xprintf(("Temp: %d, humidity: %d, %s\n",
               temp,humidity,desc));

      xprintf(("***** Forecast:\n"));
      for(i=0 ; i < 7 && JErr_noError(&err) ; i++)
      {
         S32 wday;
         S32 tmax;
         S32 tmin;
         JVal_get(jForecast[i],
                  &err,
                  "{ddddds}",
                  "wday",&wday,
                  "tmax",&tmax,
                  "tmin",&tmin,
                  "humidity",&humidity,
                  "id", &weatherId,
                  "desc",&desc);
         tmax = tmax*9/50 + 32; /* celcius x 10 to F */
         tmin = tmin*9/50 + 32; /* celcius x 10 to F */
         xprintf(("-------------------------------\n"
                "%s: %d/%d, humidity=%d, %s\n",
                  wday2String(wday-1),
                tmax,tmin,
                humidity,
                desc));
      }
   }
   return JErr_isError(&err);
}


/*
  The server sends weather information to the device at regular
  intervals. This data is pushed every 20 minute. The data pushed to
  the device also includes additional information such the correct
  time. Note that this function must extract time and call
  setSystemTime().

  Data is sent from the server to the device as JSON. The JSON data is
  provided as a JVal JSON tree. See the documentation for JVal for how
  to extract data and how to use the utility function JVal_get.
  https://realtimelogic.com/ba/doc/en/C/reference/html/structJVal.html 
 */
static int
currentWeatherMsg(JVal* rootVal, S32 tzOffs)
{
   const char* desc;
   S32 temp;
   U32 humidity;
   U32 dt; /* date & time */
   JErr err;
   JErr_constructor(&err);
   JVal_get(rootVal,
            &err,
            "{sddd}",
            "desc",&desc,
            "temp",&temp,
            "humidity",&humidity,
            "time", &dt);
   if(JErr_noError(&err))
   {
      xprintf(("Weather update\n"));
      if(tzOffs != INVALID_TZOFFS)
         setSystemTime(dt+tzOffs); /* Set time adjusted for local time zone */
      temp = temp*9/50 + 32; /* celcius x 10 to F */
      xprintf(("Temp: %d, humidity: %d, %s\n",
               temp,humidity,desc));
   }
   return JErr_isError(&err);
}

/* Global variable used by setThermostatTemp() and pollThermostatTemp()
 */
static S32 thermostatTemp=70;

/* Data pushed from browser(s).
   Set thermostat temperature and update the UI.
*/
static void
setThermostatTemp(S32 temp)
{
   thermostatTemp = temp;
   xprintf(("Thermostat temperature: %d\n", thermostatTemp));
}

/* 
   This function is called every 50 millisecond. The function must
   return the thermostat temperature. The main function mainIoT()
   sends the thermostat temperature to the broker if the temperature
   has changed.
 */
static S32
pollThermostatTemp(S32 temp)
{
   if(xkbhit())
   {
      int c = xgetch();
      if(c == KEY_UP_ARROW)
         temp++;
      else if(c == KEY_DOWN_ARROW)
         temp--;
   }
   return temp;
}


/* 
   The server side weather app sends information about the number of
   connected browser(s) each time a browser connects or
   disconnects. This information can optionally be displayed on the
   UI.
 */
static void
setDevClientsConnected(S32 clients)
{
   xprintf(("Browsers connected: %d\n", clients));
}


#else

#define startBrowser(x)

#endif /* HOST_PLATFORM */




/****************************************************************************
 **************************----------------------****************************
 **************************| GENERIC CODE BELOW |****************************
 **************************----------------------****************************
 ****************************************************************************/



/*

  All messages sent over SMQ for the Weather App is encoded as
  JSON. This function parses the received message.

  Note that the SMQ buffer may not be sufficiently large enough for
  the complete JSON message. The function manages this condition by
  making sure to keep reading from SMQ until bytesRead = frameLen. See
  the SMQ documentation for details:
  https://realtimelogic.com/ba/doc/en/C/shark/structSharkMQ.html

  The data read from SMQ is fed into the parser. The parser returns 1
  when a complete JSON message has been received. We check that all
  conditions are met i.e. bytesRead = frameLen and parser returns 1.

  Parser doc:
  https://realtimelogic.com/ba/doc/en/C/reference/html/structJParser.html
 */
static int
parseJsonMsg(SharkMQ* smq, JParser* jParser, U8* msg, int len)
{
   int status;
   do {
      status = JParser_parse(jParser, msg, len);
   } while(smq->bytesRead < smq->frameLen &&
           status == 0 &&
           (len = SharkMQ_getMessage(smq, &msg)) > 0);
   if(len < 0)
   {
      xprintf(("Rec Error: %d\n",smq->status));
      return len; /* sock error */
   }
   if(smq->bytesRead != smq->frameLen || status != 1)
   {
      xprintf(("JSON parse error\n"));
      return -1;
   }
   return 0;
}


/* pubJson helper function. Designed as a BufPrint flush function that
   sends data using the chunk send mechanism in the SMQ protocol.
 */
static int
pubJsonFlushCB(BufPrint* bp, int sizeRequired)
{
   int retVal;
   (void)sizeRequired; /* Not used */
   retVal = SharkMQ_write(((SharkMQ*)bp->userData), bp->buf, bp->cursor);
   bp->cursor = 0; /* reset */
   return retVal; /* zero OK, negative value: error */
}


/* Publish JSON.
   This function combines the functionality of JEncoder::set
   and SharkMQ::publish into one convenient to use function.

   Data is sent as chunks using SharkMQ_write in pubJsonFlushCB when
   the 256 byte buffer used by BufPrint is full.

   smq	the SharkMQ instance.
   tid	the topic ID.
   subtid  sub-topic ID.
   fmt see JEncoder::set for details
*/
static int
pubJson(SharkMQ* smq, U32 tid, U32 subtid, const char* fmt, ...)
{
   BufPrint bp;
   JErr jerr;
   JEncoder jenc;
   va_list argList;
   int retv;
   char buf[256];

   JErr_constructor(&jerr);
   BufPrint_constructor(&bp, smq, pubJsonFlushCB);
   bp.buf = buf;
   bp.bufSize = sizeof(buf);
   JEncoder_constructor(&jenc, &jerr, &bp);

   va_start(argList, fmt); 
   retv=JEncoder_vSetJV(&jenc,&fmt,&argList) || JEncoder_commit(&jenc);
   va_end(argList);
   
   /* zero OK, negative value: error */
   return retv || SharkMQ_pubflush(smq, tid, subtid);
}


/*
  Decode the JSON data used by setDevClientsConnected()
 */
static int
devClientsMsg(JVal* rootVal)
{
   JErr err;
   S32 clients;
   JErr_constructor(&err);
   JVal_get(rootVal, &err, "{d}", "clients", &clients);
   if(JErr_isError(&err))
      return -1;
   setDevClientsConnected(clients);
   return 0;
}


/*
  Decode the JSON data sent when a browser changes the temperature.
 */
static S32
thermostatTempMsg(JVal* rootVal)
{
   JErr err;
   S32 temp;
   BaBool isCelsius;
   JErr_constructor(&err);
   JVal_get(rootVal, &err, "{db}", "temp", &temp, "celsius", &isCelsius);
   if(isCelsius)
      return temp*9/50 + 32; /* celcius x 10 to F */
   return temp; /* format is Fahrenheit */
}


/* Manage message MSG_SET_TIMEZONE
 */
static int
timezoneMsg(JVal* rootVal, S32* tzOffs)
{
   U32 time;
   JErr err;
   JErr_constructor(&err);
   JVal_get(rootVal,
            &err,
            "{dd}",
            "tzoffs",tzOffs,
            "time",&time);
   if(JErr_noError(&err))
   {
      xprintf(("Adjusting to local time by %d hour(s)\n",*tzOffs/(60*60)));
      setSystemTime(time+*tzOffs); /* Set time adjusted for local time zone */
      return 0;
   }
   return -1;
}

       
/* Send temperature to server.

   serverTid: Server's ephemeral TID
   temp: in Fahrenheit
*/
static int
sendTemperature(SharkMQ* smq, U32 serverTid, S32 temp)
{
   return pubJson(smq, serverTid, MSG_SET_THERMOSTAT,
                  "{db}", "temp", temp, "celsius", FALSE);
}


/*
  The IoT main function does not return unless the code cannot
  connect to the broker or the connection breaks. The return
  value 0 means that the caller can attempt to reconnect by calling
  this function again.
 */
static int
mainIoT(SharkMQ* smq, const char* smqUniqueId, int smqUniqueIdLen)
{
   char ipaddr[16];
   char memberName[32]; /* Max JSON member name len */
   U8 vBuf[3000]; /* Malloc buffer for JVal instances */
   U8 dBuf[3000]; /* Malloc buffer for JSON strings */
   JPValFactAllocator vAlloc; /* v=value -> JVal (JSON Node Value: fixed) */
   JPValFactAllocator dAlloc; /* d=dynamic length -> used for JSON strings */
   JParserAllocator jpAlloc;
   JParserValFact vFact; /* JVal factory used by JParser */
   JParser jParser;
   S32 tzOffs=INVALID_TZOFFS; /* Time offset from Greenwich Mean Time */
   U32 serverTid=0; /* The SMQ ID (ephemeral id) for the server weather app */
   int status;
   U8* msg;
   int len;
   static int callCounter = 0;

   pollThermostatTemp(thermostatTemp); /*  Fahrenheit */

/* We make it possible to override the URL at the command prompt when
 * in simulation mode.
 */
#if HOST_PLATFORM
   const char* str=simpleMqUrl;
#else
   const char* str=SIMPLEMQ_URL;
#endif

   JPValFactAllocator_constructor(&vAlloc,vBuf,sizeof(vBuf));
   JPValFactAllocator_constructor(&dAlloc,dBuf,sizeof(dBuf));
   JParserValFact_constructor(
      &vFact, (AllocatorIntf*)&vAlloc, (AllocatorIntf*)&dAlloc);
   JParserAllocator_constructor(&jpAlloc);
   JParser_constructor(&jParser, (JParserIntf*)&vFact, memberName,
                       sizeof(memberName), (AllocatorIntf*)&jpAlloc, 0);

   xprintf(("Connecting to %s\n", str));
   smq->timeout = 3000; /* Bail out if the connection takes this long */
   if(SharkMQ_init(smq, scon, str, 0) < 0)
   {
      xprintf(("Cannot establish connection, status: %d\n", smq->status));
      switch(smq->status)
      {
         case -1:
            str="Socket error!";
            break;
         case -2:
            str="Cannot resolve IP address for " SIMPLEMQ_DOMAIN ".";
            break;
         default:
            str="Cannot connect to " SIMPLEMQ_DOMAIN ".";
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
                      0, 0, 0))
   {
      xprintf(("Connect failed, status: %d\n", smq->status));
      return smq->status == SMQE_BUF_OVERFLOW || smq->status > 0;
   }

   /* State : Startup.
    *  The server weather app immediately sends a hello message to device etid.
    */
   if(SharkMQ_getMessage(smq, &msg) <= 0 || 
      smq->tid != smq->clientTid ||
      smq->subtid != MSG_SERVER_HELLO)
   {
      xprintf(("Error: expected a server hello message.\n"));
      return -1; /* Must exit */
   }
   serverTid = smq->ptid;

   /* Get forecast:
      Empty location JSON object makes server use IP address for geolocation.
      The following example shows how to fetch forecast for lat/lon
      pubJson(smq,serverTid,MSG_GET_FORECAST,"{ss}","lat","33.6","lon","66.6");
      Position is encoded as string above. You can also use floating
      point numbers: "{ff}","lat",33.6,"lon",66.6
   */
   pubJson(smq, serverTid, MSG_GET_FORECAST,"{}");

   if(callCounter++==0)
      startBrowser();

    /* Server needs a copy at startup */
   sendTemperature(smq, serverTid, thermostatTemp);
   smq->timeout=50; /* Poll so we can fetch data from device temp buttons. */
   for(;;)
   {
      len = SharkMQ_getMessage(smq, &msg);
      if(len < 0) /* We received a control message or an error code */
      {
         switch(len)
         {  
            /* Error codes */

            case SMQE_DISCONNECT: 
               xprintf(("Disconnect request from server\n"));
               return -1; /* Not allowed to reconnect */

            case SMQE_BUF_OVERFLOW:
               xprintf(("Increase buffer size\n"));
               return -1; /* Must exit */

            case SMQ_TIMEOUT:
            {
               S32 temp = pollThermostatTemp(thermostatTemp);
               if(temp != thermostatTemp)
               {
                  thermostatTemp=temp;
                  status=sendTemperature(smq, serverTid, thermostatTemp);
                  if(status)
                  {
                     xprintf(("Pub JSON failed: %d\n",status));
                     return 0;
                  }
               }
            }
            break;

            default:
               xprintf(("Rec Error: %d\n",smq->status));
               return 0; /* try again */
         }
      }
      else
      {
         /* This app only accepts messages sent to the device's ephemeral tid */
         if(smq->tid == smq->clientTid)
         {
            JVal* rootVal;
            /* All SMQ messages must be JSON encoded */
            if(parseJsonMsg(smq, &jParser, msg, len))
               return 0;
            rootVal=JParserValFact_getFirstVal(&vFact);
            switch(smq->subtid)
            {
               case MSG_WEATHER_FORECAST:
                  if(weatherForecastMsg(rootVal,&tzOffs))
                     return 1;
                  if(tzOffs == INVALID_TZOFFS) /* if not set i.e. first time */
                  {
                     /* We must specifically ask for time zone offset.
                        Empty location JSON object makes server use (stored)
                        geolocation from the latest MSG_GET_FORECAST message.
                        Server responds with: MSG_SET_TIMEZONE
                     */
                     pubJson(smq, serverTid, MSG_GET_TIMEZONE,"{}");
                  }
                  break;
               case MSG_WEATHER_UPDATE:
                  if(currentWeatherMsg(rootVal,tzOffs))
                     return 1;
                  break;

               case MSG_SET_TIMEZONE:
                  timezoneMsg(rootVal,&tzOffs);
                  break;

               case MSG_DEVCLIENTS:
                  if(devClientsMsg(rootVal))
                     return 1;
                  break;

               case MSG_SET_THERMOSTAT:
                  thermostatTemp = thermostatTempMsg(rootVal);
                  setThermostatTemp(thermostatTemp);
                  break;
               default:
                  xprintf(("Received invalid sub-tid %X\n", smq->subtid));
                  return 0;
            }
            /* Reset objects and make ready for next message */
            JParserValFact_termFirstVal(&vFact);
            JPValFactAllocator_reset(&vAlloc);
            JPValFactAllocator_reset(&dAlloc);
         }
         else
         {
            xprintf(("Received unexpected tid %X\n", smq->tid));
            return 0;
         }
      }
   }
}



void
mainTask(SeCtx* ctx)
{
   static SharkMQ smq;
   static U8 buf[127];

   const char* smqUniqueId;
   int smqUniqueIdLen;



   xprintf(("\n"
"                    ______   ____    ____   ___     \n"
"                  .' ____ \\ |_   \\  /   _|.'   `.   \n"
"                  | (___ \\_|  |   \\/   | /  .-.  \\  \n"
"                   _.____`.   | |\\  /| | | |   | |  \n"
"                  | \\____) | _| |_\\/_| |_\\  `-'  \\_ \n"
"                   \\______.'|_____||_____|`.___.\\__|\n"
"\n"
"                     Simulated Weather App Device\n"
"\n"
"          Documentation:\n"
"          https://realtimelogic.com/downloads/docs/SMQ-Weather-App.pdf\n"));

   smqUniqueIdLen = getUniqueId(&smqUniqueId);
   if(smqUniqueIdLen < 1)
   {
      xprintf(("Cannot get unique ID: aborting.\n"));
      return;
   }
   printUniqueID(smqUniqueId, smqUniqueIdLen);

   SharkMQ_constructor(&smq, buf, sizeof(buf));
   SharkMQ_setCtx(&smq, ctx);  /* Required for non RTOS env. */

   while(! mainIoT(&smq, smqUniqueId, smqUniqueIdLen) )
   {
      xprintf(("Closing connection; status: %d\n",smq.status));
      SharkMQ_disconnect(&smq);
   }
   SharkMQ_destructor(&smq);
}


/*************************************************************************
  The following code implements a static (not using malloc) allocator
  for the JSON lib.
**************************************************************************/



static void*
JParserAllocator_malloc(AllocatorIntf* super, size_t* size)
{
   JParserAllocator* o = (JParserAllocator*)super;
   return *size <= MAX_STRING_LEN ? o->buf : 0;
}


//Called when the one and only string buffer must grow.
static void*
JParserAllocator_realloc(AllocatorIntf* super, void* memblock, size_t* size)
{
   JParserAllocator* o = (JParserAllocator*)super;
   baAssert(memblock == o->buf);
   return *size <= MAX_STRING_LEN ? o->buf : 0;
}


//Called when JParser terminates
static void
JParserAllocator_free(AllocatorIntf* super, void* memblock)
{
   // Do nothing
   (void)super;
   (void)memblock;
}



void
JParserAllocator_constructor(JParserAllocator* o)
{
   AllocatorIntf_constructor((AllocatorIntf*)o,
                             JParserAllocator_malloc,
                             JParserAllocator_realloc,
                             JParserAllocator_free);
}


static void*
JPValFactAllocator_malloc(AllocatorIntf* super, size_t* size)
{
   JPValFactAllocator* o = (JPValFactAllocator*)super;
   if(*size + o->cursor < o->bufsize)
   {
      void* buf = o->buf+o->cursor;
      o->cursor += *size;
      return buf;
   }
   return 0; // Memory exhausted
}


//Not used by JParserValFact
static void* JPValFactAllocator_realloc(
   AllocatorIntf* super, void* memblock, size_t* size)
{
   (void)super;
   (void)memblock;
   (void)size;
   baAssert(0);
   return 0;
}


// Called when JParserValFact terminates or when
// JParserValFact_termFirstVal is called.
static void JPValFactAllocator_free(AllocatorIntf* super, void* memblock)
{
   // Do nothing
   (void)super;
   (void)memblock;
}


void
JPValFactAllocator_constructor(JPValFactAllocator* o, void* b, size_t bsize)
{
   AllocatorIntf_constructor((AllocatorIntf*)o,
                             JPValFactAllocator_malloc,
                             JPValFactAllocator_realloc,
                             JPValFactAllocator_free);
   o->buf =(U8*)b;
   o->bufsize = bsize;
   JPValFactAllocator_reset(o);
}
