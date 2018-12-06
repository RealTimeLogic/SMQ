

        Socket Example library (selib.c) porting layers

selib.h includes selibplat.h, which can be found in the following
directories:

- Non event based porting layers (RTOS):
FreeRTOS-TCP    https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/
Harmony         Microchip Harmony TCP/IP
MDK             Keil MDK
MQX             MQX and RTCS from Freescale
NetX            ThreadX and NetX from Express Logic
Posix           POSIX including Linux, Mac, VxWorks, QNX
Windows         Windows
lwIP            lwIP Netconn API for RTOS enabled systems

- Bare metal (no RTOS) event based porting layers:
lwIP-raw        lwIP raw TCP API port: http://lwip.wikia.com/wiki/Raw/TCP
uip             uIP TCP/IP port: http://sourceforge.net/projects/uip-stack/

Note: bare metal (event based) requires the context manager SeCtx.c
