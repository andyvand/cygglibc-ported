/* Assembler macros for HP/PA.
   Copyright (C) 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper, <drepper@cygnus.com>, August 1999.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <sysdeps/generic/sysdep.h>
#include <sys/syscall.h>

#ifdef	__ASSEMBLER__

/* Syntactic details of assembler.  */

#define ALIGNARG(log2) log2


/* Define an entry point visible from C.

   There is currently a bug in gdb which prevents us from specifying
   incomplete stabs information.  Fake some entries here which specify
   the current source file.  */
#define	ENTRY(name)							      \
  .SPACE $TEXT$;							      \
  .SUBSPA $CODE$,QUAD=0,ALIGN=8,ACCESS=44,CODE_ONLY;			      \
  .align ALIGNARG(4);							      \
  .NSUBSPA $CODE$,QUAD=0,ALIGN=8,ACCESS=44,CODE_ONLY;			      \
  .EXPORT C_SYMBOL_NAME(name),ENTRY,PRIV_LEV=3,ARGW0=GR,RTNVAL=GR;	      \
  C_LABEL(name)								      \

  CALL_MCOUNT

#undef	END
#define END(name)							      \
  .PROCEND


/* If compiled for profiling, call `mcount' at the start of each function.  */
#ifdef	PROF
/* The mcount code relies on a normal frame pointer being on the stack
   to locate our caller, so push one just for its benefit.  */
#define CALL_MCOUNT \
  XXX
#else
#define CALL_MCOUNT		/* Do nothing.  */
#endif

#define	PSEUDO(name, syscall_name, args)				      \
  ENTRY (name)								      \
  DO_CALL (syscall_name, args)

#undef	PSEUDO_END
#define	PSEUDO_END(name)						      \
  END (name)

#define JUMPTARGET(name)	name
#define SYSCALL_PIC_SETUP	/* Nothing.  */

/* Local label name for asm code. */
#ifndef L
#define L(name)		name
#endif

#endif	/* __ASSEMBLER__ */
