
Porting layer for lwIP Raw/TCP : http://lwip.wikia.com/wiki/Raw/TCP

Designed specifically for event driven non RTOS (bare metal) systems.

The Socket Example library (selib) provides a sequential API designed
for blocking calls. The lwIP Raw porting layer makes the sequential
selib functions cooperate with any event driven system by utilizing the
Socket Example Library's Context Manager (SeCtx.c).

The following examples show (1) an unmodified lwIP event loop and
(2 & 3) how to integrate the Context Manager in the event loop. The
code is similar for interrupt driven systems, but the three variables
stackbuf, ctx, and startTime must be global and initialized at startup
(not in the interrupt function).



------------------- Example 1: original lwIP poll loop --------------------
  for(;;) {
     sys_check_timeouts();
     .
     .
---------------------------------------------------------------------------



------------------- Example 2: modified lwIP poll loop --------------------
  static char stackbuf[512]; 
  static SeCtx ctx; /* Context Manager (magic stack handler) */
  SeCtx_constructor(&ctx, stackbuf, sizeof(stackbuf)); 
  for(;;) {
     sys_check_timeouts(); /* 1: run lwIP events */
     SeCtx_run(&ctx); /* 2: run sequential code */
     .
     .

Function 'SeCtx_run', found in seLwIP.c, is internally using the
Context Manager (SeCtx.c). 

The socket library and all example programs are sequential and
blocking. However, the Context Manager makes sure the sequential code
works in an event driven system.

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
