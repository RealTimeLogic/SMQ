
Porting layer for uIP, the two kb TCP/IP stack.

Start by reading:
https://realtimelogic.com/ba/doc/en/C/shark/group__BareMetal.html

The uIP porting layer has been tested on the uIP stack that comes with
the Texas Instrument's extremely low cost Cortex M4 development board
EK-TM4C1294XL: http://www.ti.com/tool/ek-tm4c1294xl

uIP version 1.0 can also be downloaded from:
http://sourceforge.net/projects/uip-stack/

The porting layer is currently limited to client socket
connections. It is specifically designed for low cost IoT (M2M)
solutions. The porting layer has been tested with SharkMQ, the LED
demos, and the SMTP lib. These examples require DHCP for easy
configuration and hostname lookup (DNS). DHCP and DNS are provided in
uIP as applications and can be found in the uIP's application sub
directory.

Compiler include path:
examples\arch\uip
examples
inc
inc\arch\BareMetal


-----------------------------------------------------------
***** Required uIP modifications
-----------------------------------------------------------

uIP is optimized for use with one application. Using more
than one application requires careful configuration; at a
minimum, three applications will be required: one SharkSSL example, DHCP, and
DNS.




-----------------------------------------------------------
***** Step 1: Modify uIP for multi application use.
-----------------------------------------------------------

resolv.h: comment out:
typedef int uip_udp_appstate_t;
#define UIP_UDP_APPCALL resolv_appcall

dhcp.h: comment out:
typedef struct dhcpc_state uip_udp_appstate_t;
#define UIP_UDP_APPCALL dhcpc_appcall

** Add the following in uip-conf.h:
#include "dhcpc/dhcpc.h"
#include "resolv/resolv.h"
#ifdef RESOLV_C
typedef int uip_udp_appstate_t;
#else
typedef union{
  int resolve;
  struct dhcpc_state dhcp;
}uip_udp_appstate_t;
#endif

#define UIP_UDP_APPCALL() do { \
  if(uip_hostaddr[0]) resolv_appcall(); else dhcpc_appcall(); } while(0)

#define resolv_found se_resolv_found

** Modify the following code:

from:
#define UIP_APPCALL     httpd_appcall
to:
#define UIP_APPCALL() se_appcall()

** Modify the following code:

from:
typedef struct httpd_state uip_tcp_appstate_t
to:
struct SOCKET;
typedef struct
{
    struct SOCKET* sock;
} uip_tcp_appstate_t;

#define GET_SOCKET() uip_conn->appstate.sock
#define SET_SOCKET(conn,s) conn->appstate.sock=s

Make sure this is set:
#define UIP_BUFSIZE     1500 /* We have also tested with 700 */
#define UIP_REASSEMBLY 1


-----------------------------------------------------------
***** Step 2: Include the additional uIP apps required.
-----------------------------------------------------------

Make sure dhcpc/dhcpc.c and resolv/resolv.c are included in your build.

Add the following in resolve.c, just above the line: #include "resolv.h"
#define RESOLV_C

Make sure resolv_init and resolv_conf are called. Function resolv_conf
can be called when DHCP is configured as follows:
resolv_conf((unsigned short*)s->default_router);

-----------------------------------------------------------
***** Step 3: Implement timer function.
-----------------------------------------------------------

Implement function u32_t sys_now(void);

The timer function must return system time in milliseconds. The
function is typically implemented as follows:

u32_t systemTime; /* Global variable updated by timer tick interrupt function */
u32_t sys_now(void)
{
	return systemTime;
}


-----------------------------------------------------------
***** Step 4: Include an allocator.
-----------------------------------------------------------

SharkSSL requires an allocator. You can use any allocator or the
included 'umm' allocator. You must compile the code with the following
macro if you intend to use 'umm'.

UMM_MALLOC


-----------------------------------------------------------
***** Step 5: Call SeCtx_run from uIP main loop.
-----------------------------------------------------------

static char stackbuf[512]; 
static SeCtx ctx; /* Context Manager (magic stack handler) */
SeCtx_constructor(&ctx, mainTask, stackbuf, sizeof(stackbuf));
while(true)
{
   ;
   ;
   if(BUF->type == htons(UIP_ETHTYPE_IP))
   {
      uip_arp_ipin();
      uip_input();
      SeCtx_run(&ctx);
      ;
      ;
   }
   else // main loop is idle
   {   
      SeCtx_run(&ctx); // Check timers
   }

The above code snippet is from the TM4C1294XL port. The uIP function
uip_input is called when a new Ethernet interrupt has
occurred. function SeCtx_run must be called immediately after
uip_input. We are also calling SeCtx_run when the main loop is idle.

Function 'SeCtx_run', found in seuip.c, is internally using the
Context Manager (SeCtx.c).  See the following link for more info on SeCtx.c.
https://realtimelogic.com/ba/doc/en/C/shark/group__BareMetal.html

------------------------
Errors:
------------------------

We got a compile error in uIP when compiled with IAR. If you get a
compile error in resolv_conf, change the function to:

void
resolv_conf(u16_t *dnsserver)
{
  union {
    u16_t *dns;
    uip_ipaddr_t* addr;
  } u;
  if(resolv_conn != NULL) {
    uip_udp_remove(resolv_conn);
  }
  u.dns = dnsserver;
  resolv_conn = uip_udp_new(u.addr, HTONS(53));
}

------------------------
Optimizing SharkSSL for minimum size:
------------------------

You probably have severe memory limitations if you use uIP. The
following macro makes the SharkSSL code base less than 20Kb.

SHARKSSL_TINY

See the user config file inc/SharkSSL_opts.h for details.
