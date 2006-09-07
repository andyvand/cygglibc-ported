/* Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Carlos O'Donell <carlos@baldric.uwo.ca>, 2005.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <stdint.h>
#include <sysdep.h>
#include <abort-instr.h>
#include <kernel-features.h>

/* We need EFAULT, ENONSYS, and EAGAIN */
#if !defined EFAULT && !defined ENOSYS && !defined EAGAIN
#undef EFAULT
#undef ENOSYS
#undef EAGAIN
#define EFAULT	14
#define ENOSYS	251
#define EAGAIN	11
#endif

#ifndef _BITS_ATOMIC_H
#define _BITS_ATOMIC_H	1

typedef int8_t atomic8_t;
typedef uint8_t uatomic8_t;
typedef int_fast8_t atomic_fast8_t;
typedef uint_fast8_t uatomic_fast8_t;

typedef int32_t atomic32_t;
typedef uint32_t uatomic32_t;
typedef int_fast32_t atomic_fast32_t;
typedef uint_fast32_t uatomic_fast32_t;

typedef intptr_t atomicptr_t;
typedef uintptr_t uatomicptr_t;
typedef intmax_t atomic_max_t;
typedef uintmax_t uatomic_max_t;

/* prev = *addr;
   if (prev == old)
     *addr = new;
   return prev; */

/* Use the kernel atomic light weight syscalls on hppa */ 
#define LWS "0xb0"
#define LWS_CAS 0x0
/* Note r31 is the link register */
#define LWS_CLOBBER "r1", "r26", "r25", "r24", "r23", "r22", "r21", "r20", "r28", "r31", "memory"
#define ASM_EAGAIN -EAGAIN

#if __ASSUME_LWS_CAS
/* The only basic operation needed is compare and exchange.  */
# define atomic_compare_and_exchange_val_acq(mem, newval, oldval) 	\
  ({									\
     volatile int lws_errno = EFAULT;					\
     volatile int lws_ret = 0xdeadbeef;					\
     asm volatile(							\
	"0:					\n\t"			\
	"copy	%3, %%r26			\n\t"			\
	"copy	%4, %%r25			\n\t"			\
	"copy	%5, %%r24			\n\t"			\
	"ble	" LWS "(%%sr2, %%r0)		\n\t"			\
	"ldi	0, %%r20			\n\t"			\
	"cmpib,=,n " ASM_EAGAIN ",%%r21,0	\n\t"			\
	"nop					\n\t"			\
	"stw	%%r28, %0			\n\t"			\
        "sub	%%r0, %%r21, %%r21		\n\t"			\
	"stw	%%r21, %1			\n\t"			\
	: "=m" (lws_ret), "=m" (lws_errno), "=m" (*mem)			\
        : "r" (mem), "r" (oldval), "r" (newval)				\
	: LWS_CLOBBER							\
     );									\
    									\
     if(lws_errno == EFAULT || lws_errno == ENOSYS)			\
     	ABORT_INSTRUCTION;						\
    									\
     lws_ret;								\
   })

# define atomic_compare_and_exchange_bool_acq(mem, newval, oldval) 	\
  ({									\
     int ret;								\
     ret = atomic_compare_and_exchange_val_acq(mem, newval, oldval);	\
     /* Return 1 if it was already acquired */				\
     (ret != oldval);							\
   })
#else
/* Non-atomic primitives. */
# define atomic_compare_and_exchange_val_acq(mem, newval, oldval) \
  ({ __typeof (mem) __gmemp = (mem);				      \
     __typeof (*mem) __gret = *__gmemp;				      \
     __typeof (*mem) __gnewval = (newval);			      \
								      \
     if (__gret == (oldval))					      \
       *__gmemp = __gnewval;					      \
     __gret; })

# define atomic_compare_and_exchange_bool_acq(mem, newval, oldval) \
  ({ __typeof (mem) __gmemp = (mem);				      \
     __typeof (*mem) __gnewval = (newval);			      \
								      \
     *__gmemp == (oldval) ? (*__gmemp = __gnewval, 0) : 1; })
#endif

#endif	/* bits/atomic.h */

