/* Copyright (C) 1993 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#include <ansidecl.h>

#ifndef	__GNUC__
  #error This file uses GNU C extensions; you must compile with GCC.
#endif

/* Get the definition of `struct sigcontext'.  */
#define	KERNEL
#define	sigvec		sun_sigvec
#define	sigstack	sun_sigstack
#define	sigcontext	sun_sigcontext
#include "/usr/include/sys/signal.h"
#undef	sigvec
#undef	sigstack
#undef	sigcontext
#undef	NSIG
#undef	SIGABRT
#undef	SIGCLD
#undef	SV_ONSTACK
#undef	SV_RESETHAND
#undef	SV_INTERRUPT
#undef	SA_ONSTACK
#undef	SA_NOCLDSTOP
#undef	SIG_ERR
#undef	SIG_DFL
#undef	SIG_IGN
#undef	sigmask
#undef	SIG_BLOCK
#undef	SIG_UNBLOCK
#undef	SIG_SETMASK

#include <signal.h>
#include <stddef.h>
#include <errno.h>

/* Defined in __sigvec.S.  */
extern int EXFUN(__raw_sigvec, (int sig, CONST struct sigvec *vec,
				struct sigvec *ovec));

/* User-specified signal handlers.  */
#define mytramp 1
#ifdef mytramp
static __sighandler_t handlers[NSIG];
#else
#define handlers _sigfunc
extern __sighandler_t _sigfunc[];
#endif

#if mytramp

/* Handler for all signals that are handled by a user-specified function.
   Saves and restores the general regs %g2-%g7, the %y register, and
   all the FPU regs (including %fsr), around calling the user's handler.  */
static void
DEFUN(trampoline, (sig, code, context, addr),
      int sig AND int code AND struct sigcontext *context AND PTR addr)
{
  register int a0 asm("%a0");
  register int a1 asm("%a1");
  register int d0 asm("%d0");
  register int d1 asm("%d1");

  int savea[2], saved[2];

  double fpsave[16];
  int fsr;
  int savefpu;

  /* Save the call-clobbered registers.  */
  savea[0] = a0;
  savea[1] = a1;
  saved[0] = d0;
  saved[1] = d1;

#if 0
  /* Save the FPU regs if the FPU enable bit is set in the PSR,
     and the signal isn't an FP exception.  */
  savefpu = (context->sc_psr & 0x1000) && sig != SIGFPE;
  if (savefpu)
#endif

  /* Call the user's handler.  */
  (*((void EXFUN((*), (int sig, int code, struct sigcontext *context,
		       PTR addr))) handlers[sig]))
    (sig, code, context, addr);

  /* Restore the call-clobbered registers.  */
  a0 = savea[0];
  a1 = savea[1];
  d0 = saved[0];
  d1 = saved[1];

#if 0
  if (savefpu)
    ;
#endif
}

#endif

int
DEFUN(__sigvec, (sig, vec, ovec),
      int sig AND CONST struct sigvec *vec AND struct sigvec *ovec)
{
#ifndef	mytramp
  extern void _sigtramp (int);
#define	trampoline	_sigtramp
#endif
  struct sigvec myvec;
  int mask;
  __sighandler_t ohandler;

  if (sig <= 0 || sig >= NSIG)
    {
      errno = EINVAL;
      return -1;
    }

  mask = __sigblock(sigmask(sig));

  ohandler = handlers[sig];

  if (vec != NULL &&
      vec->sv_handler != SIG_IGN && vec->sv_handler != SIG_DFL)
    {
      handlers[sig] = vec->sv_handler;
      myvec = *vec;
      myvec.sv_handler = trampoline;
      vec = &myvec;
    }

  if (__raw_sigvec(sig, vec, ovec) < 0)
    {
      int save = errno;
      (void) __sigsetmask(mask);
      errno = save;
      return -1;
    }

  if (ovec != NULL && ovec->sv_handler == trampoline)
    ovec->sv_handler = ohandler;

  (void) __sigsetmask(mask);

  return 0;
}
