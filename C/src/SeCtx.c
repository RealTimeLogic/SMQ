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
 *   $Id: SeCtx.c 4914 2021-12-01 18:24:30Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2017
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
 ****************************************************************************




SeCtx ( Socket Example Library Context Manager) for bare metal systems.

Documentation:
https://realtimelogic.com/ba/doc/en/C/shark/group__BareMetal.html




*/

#include "selib.h"


#ifdef DYNAMIC_SE_CTX
void
SeCtx_destructor(SeCtx* o)
{
   if(o->stackBuf)
      baFree(o->stackBuf);
   o->stackBuf=0;
}
#endif

static void
SeCtx_setStackBuf(SeCtx* o, U32 size)
{
#ifdef DYNAMIC_SE_CTX
   if(size > 40*1024)
      SeCtx_panic(0, size);
   if(o->stackBuf)
      baFree(o->stackBuf);
   o->stackBufLen = (U16)size;
   o->stackBuf = baMalloc(o->stackBufLen);
   if( ! o->stackBuf )
#endif
      SeCtx_panic(o, size);
}

static int
SeCtx_saveStackOrSwitchContext(SeCtx* o, U8 switchContext)
{
  auto U8 stackmark;
  U8* currentStack;
  stackmark = 1; /* Make sure compiler keeps this variable. */
  currentStack = &stackmark;
  if(switchContext) /* Restore original context */
  {
     baAssert(o->currentStack == currentStack);
     memcpy(currentStack, o->stackBuf, o->stackTop-currentStack);
     return FALSE;
  }
  else /* Save context i.e. save stack. */
  {
#ifndef NDEBUG
     o->currentStack = currentStack;
#endif
     if((o->stackTop - currentStack) > o->stackBufLen)
        SeCtx_setStackBuf(o, (o->stackTop - currentStack) + 100);
     memcpy(o->stackBuf, currentStack, o->stackTop-currentStack);
     return TRUE;
  }
}


#ifdef DYNAMIC_SE_CTX
void
SeCtx_constructor(SeCtx* o, SeCtxTask* t)
{
   baAssert(sizeof(int) >= sizeof(void*));
   memset(o, 0, sizeof(SeCtx));
   o->task=t;
#ifndef NDEBUG
   o->magic1 = o->magic2 = 0xAFA5A5F5;
#endif
   SeCtx_setStackBuf(o, 1000);
}
#else
void
SeCtx_constructor(SeCtx* o, SeCtxTask t, void* buf, int bufLen)
{
   baAssert(sizeof(int) >= sizeof(void*));
   memset(o, 0, sizeof(SeCtx));
#ifndef NDEBUG
   o->magic1 = o->magic2 = 0xAFA5A5F5;
#endif
   o->task=t;
   o->stackBuf = buf;
   o->stackBufLen = (U16)bufLen;
}
#endif


void
SeCtx_save(SeCtx* o)
{
   int val;
   baAssert( ! o->hasContext );
   baAssert(o->ready == FALSE);
   baAssert(o->magic1 == 0xAFA5A5F5 && o->magic2== 0xAFA5A5F5);  
   val = setjmp(o->savedContext);
   if(val)
   {
      o = (SeCtx*)val;
      SeCtx_saveStackOrSwitchContext(o, TRUE);
      baAssert(o->magic1 == 0xAFA5A5F5 && o->magic2== 0xAFA5A5F5);  
   }
   else
   {
      if(SeCtx_saveStackOrSwitchContext(o, FALSE))
      {
         o->hasContext = TRUE;
         longjmp(o->startContext, 1);
      }
   }
}


void
SeCtx_restore(SeCtx* o)
{
  baAssert(o->hasContext);
  o->hasContext = FALSE;
  o->ready = FALSE;
  longjmp(o->savedContext, (int)o);
}
