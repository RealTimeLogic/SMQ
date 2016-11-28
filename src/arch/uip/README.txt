uIP is an extremely small event driven TCP/IP stack. The uIP porting
layer makes it possible to use the sequential socket library and the
sequential socket examples in an event driven system. An introduction
to bare metal (event driven) systems can be found here:

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

uIP is optimized for use with one application. Using more
than one application requires careful configuration; at a
minimum, three applications will be required: one SharkSSL example, DHCP, and
DNS.

------------------------
***** Step 1: Modify uIP for multi application use.
------------------------

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

#define UIP_UDP_APPCALL() do { if(uip_hostaddr[0]) resolv_appcall(); else dhcpc_appcall(); } while(0)

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


------------------------
***** Step 2: Include the additional uIP apps required.
------------------------

Make sure dhcpc/dhcpc.c and resolv/resolv.c are included in your build.

Add the following in resolve.c, just above the line: #include "resolv.h"
#define RESOLV_C

Make sure resolv_init and resolv_conf are called. Function resolv_conf
can be called when DHCP is configured as follows:
resolv_conf((unsigned short*)s->default_router);

------------------------
***** Step 3: Implement timer function.
------------------------

Implement function u32_t sys_now(void);

The timer function must return system time in milliseconds. The
function is typically implemented as follows:

u32_t systemTime; /* Global variable updated by timer tick interrupt function */
u32_t sys_now(void)
{
	return systemTime;
}


------------------------
***** Step 4: Include an allocator.
------------------------

SharkSSL requires an allocator. You can use any allocator or the
included 'umm' allocator. You must compile the code with the following
macro if you intend to use 'umm'.

UMM_MALLOC


------------------------
***** Step 5: Call SeCtx_run from uIP main loop.
------------------------

static char stackbuf[512]; 
static SeCtx ctx; /* Context Manager (magic stack handler) */
SeCtx_constructor(&ctx, stackbuf, sizeof(stackbuf));
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

You must also include function SeCtx_panic in your code as shown in
the following example. The Context Manager calls this function if the
provided stack buffer (stackbuf) is not large enough for saving
the stack used by the sequential functions. You can trim this buffer
so it's just big enough for your sequential code. BTW, you should try to
minimize the number of variables declared on the stack in the
sequential functions.

void SeCtx_panic(SeCtx* o, U32 size)
{
  printf("PANIC: stack size required: %u",size);
  for(;;); /* stop system */
}

It is possible to create several Context Managers (SeCtx instances)
and run several parallel sequential threads, but you will have to
study the code a bit to find out how this can be done.

SeCtx_run internally calls 'mainTask', which is the entry for all
socket examples.

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
following macros make the SharkSSL code base less than 20Kb. The
following settings remove the server functionality and disable all
ciphers except for ECC and chacha/poly.

SHARKSSL_ACCEPT_CLIENT_HELLO_2_0=0
SHARKSSL_AES_CIPHER_LOOP_UNROLL=0
SHARKSSL_BIGINT_EXP_SLIDING_WINDOW_K=1
SHARKSSL_BIGINT_WORDSIZE=32
SHARKSSL_DES_CIPHER_LOOP_UNROLL=0
SHARKSSL_ECC_USE_SECP192R1=0
SHARKSSL_ECC_USE_SECP224R1=0
SHARKSSL_ECC_USE_SECP384R1=0
SHARKSSL_ECC_USE_SECP521R1=0
SHARKSSL_ENABLE_AES_CCM=0
SHARKSSL_ENABLE_AES_CTR_MODE=0
SHARKSSL_ENABLE_AES_GCM=0
SHARKSSL_ENABLE_CERTSTORE_API=0
SHARKSSL_ENABLE_CERT_CHAIN=0
SHARKSSL_ENABLE_CLONE_CERTINFO=0
SHARKSSL_ENABLE_DHE_RSA=0
SHARKSSL_ENABLE_ECDHE_RSA=0
SHARKSSL_ENABLE_ECDH_ECDSA=0
SHARKSSL_ENABLE_ECDH_RSA=0
SHARKSSL_ENABLE_MD5_CIPHERSUITES=0
SHARKSSL_ENABLE_PEM_API=0
SHARKSSL_ENABLE_PSK=0
SHARKSSL_ENABLE_RSA=0
SHARKSSL_ENABLE_RSA_API=0
SHARKSSL_ENABLE_RSA_BLINDING=1
SHARKSSL_ENABLE_SELECT_CIPHERSUITE=0
SHARKSSL_ENABLE_SESSION_CACHE=0
SHARKSSL_ENABLE_SSL_3_0=0
SHARKSSL_ENABLE_TLS_1_1=0
SHARKSSL_MD5_SMALL_FOOTPRINT=1
SHARKSSL_SHA256_SMALL_FOOTPRINT=1
SHARKSSL_SSL_SERVER_CODE=0
SHARKSSL_UNALIGNED_ACCESS=1
SHARKSSL_USE_3DES=0
SHARKSSL_USE_AES_128=0
SHARKSSL_USE_AES_256=0
SHARKSSL_USE_ARC4=0
SHARKSSL_USE_DES=0
SHARKSSL_USE_ECC=1
SHARKSSL_USE_MD5=0
SHARKSSL_USE_NULL_CIPHER=0
SHARKSSL_USE_RNG_TINYMT=1
SHARKSSL_USE_SHA1=0
SHARKSSL_USE_SHA_256=0
SHARKSSL_USE_SHA_512=0
SHARKSSL_USE_SHA_384=0
SHARKSSL_OPTIMIZED_BIGINT_ASM=1
SHARKSSL_OPTIMIZED_CHACHA_ASM=1
SHARKSSL_OPTIMIZED_POLY1305_ASM=1
SHARKSSL_ENABLE_CLIENT_AUTH=0
NDEBUG



