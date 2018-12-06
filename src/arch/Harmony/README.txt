
Porting layer for Microchip Harmony TCP/IP stack :
http://ww1.microchip.com/downloads/en/DeviceDoc/MPLAB%20Harmony%20TCP-IP%20Stack%20Libraries_v110.pdf

Designed specifically for event driven non RTOS (bare metal) systems.

Start by reading:
https://realtimelogic.com/ba/doc/en/C/shark/group__BareMetal.html

Compiler include path:
examples\arch\Harmony
examples
inc
inc\arch\BareMetal

Set the macro UMM_MALLOC. See inc\arch\BareMetal\TargConfig.h for details
Include umm_malloc.c in your build


Limitations:

The porting layer is currently limited to client socket connections.

The limitation mentioned in the documentation, that a socket cannot be
declared on the task stack, does not apply to the Harmony porting
layer. You may declare sockets on the task stack.

SharkSSL requires an allocator. You can use any allocator or the
included 'umm' allocator in examples/malloc/. See
inc/arch/BareMetal/TargConfig.h for details.
