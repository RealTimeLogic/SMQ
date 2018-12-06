
Porting layer for lwIP Raw/TCP : http://lwip.wikia.com/wiki/Raw/TCP

Designed specifically for event driven non RTOS (bare metal) systems.

Start by reading:
https://realtimelogic.com/ba/doc/en/C/shark/group__BareMetal.html


## lwIP Example:


------------------- Example 1: original lwIP poll loop --------------------
  for(;;) {
     sys_check_timeouts();
     .
     .
---------------------------------------------------------------------------



------------------- Example 2: modified lwIP poll loop --------------------
  static char stackbuf[512]; 
  static SeCtx ctx; /* Context Manager (magic stack handler) */
  SeCtx_constructor(&ctx, mainTask, stackbuf, sizeof(stackbuf)); 
  for(;;) {
     sys_check_timeouts(); /* 1: run lwIP events */
     SeCtx_run(&ctx); /* 2: run sequential code */
     .
     .

