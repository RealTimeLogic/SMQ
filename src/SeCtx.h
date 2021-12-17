/*
 *     ____             _________                __                _     
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__  
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/  
 *                                                       /____/          
 *
 ****************************************************************************
 *            HEADER
 *
 *   $Id: SeCtx.h 4914 2021-12-01 18:24:30Z wini $
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
 *               http://sharkssl.com
 ****************************************************************************
 *
 */

#define SE_CTX


/** @defgroup SeCtx Context Manager
    @ingroup BareMetal

    The SeCtx library makes it possible to use sequential functions in
    an event driven system such as a bare metal (non RTOS) system. It
    does so by implementing a method for saving and restoring the
    stack by using the standard C library functions setjmp/longjmp.

    \if SharkSslDoc
    See [Bare Metal Systems](@ref BareMetal) for an introduction.
    \else
    See [Bare Metal Systems](../../shark/group__BareMetal.html) for an introduction.
    \endif

@{
*/




#include <setjmp.h>
#include <TargConfig.h>

struct SeCtx;

/** The task/thread entry point */
typedef void (*SeCtxTask)(struct SeCtx* ctx);

/** SeCtx structure: See [Context Manager](@ref SeCtx) and
    [Bare Metal Systems](@ref BareMetal) for details.
 */
typedef struct SeCtx
{
   jmp_buf savedContext;
#ifndef NDEBUG
   U32 magic1;
#endif
   jmp_buf startContext;
#ifndef NDEBUG
   U32 magic2;
#endif
   SeCtxTask task;
   U8* stackTop;
   void* stackBuf;
#ifndef NDEBUG
   U8* currentStack;
#endif
   U32 timeout;
   U32 startTime;
   U16 stackBufLen;
   U8 hasContext;
   U8 ready;
#ifdef SECTX_EX
   SECTX_EX;
#endif
} SeCtx;

#define SeCtx_setStackTop(o, stackMark) \
  (*(stackMark)=1,(o)->stackTop=stackMark,setjmp((o)->startContext))

#ifdef DYNAMIC_SE_CTX
void SeCtx_constructor(SeCtx* o,SeCtxTask t);
void SeCtx_destructor(SeCtx* o);
#else


/** Create a Context Manager instance.
    \param o uninitialized data of size sizeof(SeCtx).
    \param t the task/thread to call
    \param buf buffer used for storing the stack when switching from
    sequential mode to event driven mode.
    \param bufLen buffer length.
    \sa SeCtx_panic
 */
void SeCtx_constructor(SeCtx* o, SeCtxTask t, void* buf, int bufLen);
#define SeCtx_destructor(o)
#endif

void SeCtx_save(SeCtx* o); 

void SeCtx_restore(SeCtx* o);

/** Function you must implement. This function is called if the buffer
    provided in SeCtx_constructor is too small.
*/
void SeCtx_panic(SeCtx* o, U32 size);

/** @} */ /* end group SeCtx */
