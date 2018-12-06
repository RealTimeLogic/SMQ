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
 *   $Id: selib.h 3407 2014-06-24 22:44:50Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2013 - 2015
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

#ifndef _ledctrl_h
#define _ledctrl_h

#include "selib.h"

/* Do not change the number sequence. Must match peer code. */
typedef enum
{
   LedColor_red=0,
   LedColor_yellow=1,
   LedColor_green=2,
   LedColor_blue=3
} LedColor;


/* Each LED is registered with the following information */
typedef struct {
   const char* name; /* LED name shown in the browser */
   LedColor color; /* The color of this particular LED */
   int id; /* A unique ID for the LED. ID range can be 0 to 15. */
} LedInfo;


typedef enum {
   ProgramStatus_Starting,
   ProgramStatus_Restarting,
   ProgramStatus_Connecting,
   ProgramStatus_SslHandshake,
   ProgramStatus_DeviceReady,
   ProgramStatus_CloseCommandReceived,

   ProgramStatus_SocketError,
   ProgramStatus_DnsError,
   ProgramStatus_ConnectionError,
   ProgramStatus_CertificateNotTrustedError,
   ProgramStatus_SslHandshakeError,
   ProgramStatus_WebServiceNotAvailError,
   ProgramStatus_PongResponseError,
   ProgramStatus_InvalidCommandError,
   ProgramStatus_MemoryError
} ProgramStatus;


#ifdef __cplusplus
extern "C" {
#endif

/*
  Return an array of LedInfo (struct). Each element in the array
  provides information for one LED. The 'len' argument must be set by
  function getLedInfo.  The out argument 'en' specifies the length of
  the returned array, that is, number of LEDs in the device.  Each LED
  has a name, color, and ID. The ID, which provides information about
  which LED to turn on/off, is used by control messages sent between
  device code and UI clients. The IDs for a four LED device can for
  example be 1,2,3,4.
*/
const LedInfo* getLedInfo(int* len);


/* Returns the name of this device. The name is presented by UI
   clients such as browsers.
 */
const char* getDevName(void);


/* Command sent by UI client to turn LED with ID on or off. This
   function must set the LED to on if 'on' is TRUE and off if 'on' is FALSE.
 */
int setLed(int ledId, int on);

/*
  An optional function that enables LEDs to be set directly by the
  device. This function is typically used by devices that include one
  or more buttons. A button click may for example turn on a specific
  LED. The function is called at intervals (polled) by the LED device
  code. The function may for example detect a button click and return
  the information to the caller. Arguments 'ledId' and 'on' are out
  arguments, where 'ledId' is set to the LED ID and 'on' is set to
  TRUE for on and FALSE for off. The function must return TRUE (a non
  zero value) if the LED is to be set on/off and zero on no
  change. Create an empty function returning zero if you do not plan
  on implementing this feature.
*/
int setLedFromDevice(int* ledId, int* on);

/* Returns the LED on/off state for led with ID 'ledId'.
*/
int getLedState(int ledId);

/*
The purpose with program status is to provide visible program
connection state information during startup. The function is typically
used to signal information via the LEDs. Simply create an empty
function if you do not want to set program status.
*/
void setProgramStatus(ProgramStatus s);

/* Required by SMQ examples.
   The unique ID is used when calling the SMQ constructor. The
   unique ID is typically set to the MAC address. See the SMQ
   documentation for details:
   https://realtimelogic.com/ba/doc/en/C/shark/structSharkMQ.html
*/
int getUniqueId(const char** id);

/* Optional function that can be implemented and used by the SMQ
   examples if the device includes a temperature sensor. The returned
   value must be in Celsius times 10 i.e. the temperature 20 Celsius
   must be returned as the value 200. You must also compile the code
   with the macro ENABLE_TEMP defined to enable the temperature
   logic. The simulated (host) version includes a simulated
   temperature and the temperature can be changed by using the up and
   down keyboard arrows. The temperature is displayed in the browser UI.
 */
int getTemp(void);


#ifdef __cplusplus
}
#endif

#endif
