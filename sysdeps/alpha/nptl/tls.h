/* Definition for thread-local data handling.  NPTL/Alpha version.
   Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#ifndef _TLS_H
#define _TLS_H	1

# include <dl-sysdep.h>

#ifndef __ASSEMBLER__
# include <stddef.h>
# include <stdint.h>

/* Type for the dtv.  */
typedef union dtv
{
  size_t counter;
  void *pointer;
} dtv_t;

#else /* __ASSEMBLER__ */
# include <tcb-offsets.h>
#endif /* __ASSEMBLER__ */


/* We require TLS support in the tools.  */
#ifndef HAVE_TLS_SUPPORT
# error "TLS support is required."
#endif

/* Signal that TLS support is available.  */
# define USE_TLS	1

#ifndef __ASSEMBLER__

/* Get system call information.  */
# include <sysdep.h>

/* The TP points to the start of the thread blocks.  */
# define TLS_DTV_AT_TP	1

/* Get the thread descriptor definition.  */
# include <nptl/descr.h>

/* This layout is actually wholly private and not affected by the ABI.
   Nor does it overlap the pthread data structure, so we need nothing
   extra here at all.  */
typedef struct
{
  dtv_t *dtv;
} tcbhead_t;

/* This is the size of the initial TCB.  */
# define TLS_INIT_TCB_SIZE	0

/* Alignment requirements for the initial TCB.  */
# define TLS_INIT_TCB_ALIGN	__alignof__ (struct pthread)

/* This is the size of the TCB.  */
# define TLS_TCB_SIZE		0

/* Alignment requirements for the TCB.  */
# define TLS_TCB_ALIGN		__alignof__ (struct pthread)

/* This is the size we need before TCB.  */
# define TLS_PRE_TCB_SIZE \
  (sizeof (struct pthread)						      \
   + ((sizeof (tcbhead_t) + TLS_TCB_ALIGN - 1) & ~(TLS_TCB_ALIGN - 1)))

/* The following assumes that TP (R2 or R13) points to the end of the
   TCB + 0x7000 (per the ABI).  This implies that TCB address is
   TP - 0x7000.  As we define TLS_DTV_AT_TP we can
   assume that the pthread struct is allocated immediately ahead of the
   TCB.  This implies that the pthread_descr address is
   TP - (TLS_PRE_TCB_SIZE + 0x7000).  */
/* ??? PPC uses offset 0x7000; seems like a good idea for alpha too,
   but binutils not yet changed to match.  */
# define TLS_TCB_OFFSET	0

/* Install the dtv pointer.  The pointer passed is to the element with
   index -1 which contain the length.  */
# define INSTALL_DTV(tcbp, dtvp) \
  ((tcbhead_t *) (tcbp))[-1].dtv = dtvp + 1

/* Install new dtv for current thread.  */
# define INSTALL_NEW_DTV(dtv) (THREAD_DTV() = (dtv))

/* Return dtv of given thread descriptor.  */
# define GET_DTV(tcbp)	(((tcbhead_t *) (tcbp))[-1].dtv)

/* Code to initially initialize the thread pointer.  This might need
   special attention since 'errno' is not yet available and if the
   operation can cause a failure 'errno' must not be touched.  */
# define TLS_INIT_TP(tcbp, secondcall) \
  (__builtin_set_thread_pointer ((void *) (tcbp) + TLS_TCB_OFFSET), NULL)

/* Return the address of the dtv for the current thread.  */
# define THREAD_DTV() \
     (((tcbhead_t *) (__builtin_thread_pointer () - TLS_TCB_OFFSET))[-1].dtv)

/* Return the thread descriptor for the current thread.  */
# define THREAD_SELF \
    ((struct pthread *) (__builtin_thread_pointer () \
			 - TLS_TCB_OFFSET - TLS_PRE_TCB_SIZE))

/* Magic for libthread_db to know how to do THREAD_SELF.  */
# define DB_THREAD_SELF \
  REGISTER (64, 32 * 8, - TLS_TCB_OFFSET - TLS_PRE_TCB_SIZE)

/* Identifier for the current thread.  THREAD_SELF is usable but
   sometimes more expensive than necessary as in this case.  */
# define THREAD_ID (__builtin_thread_pointer ())

/* Read member of the thread descriptor directly.  */
# define THREAD_GETMEM(descr, member) ((void)(descr), (THREAD_SELF)->member)

/* Same as THREAD_GETMEM, but the member offset can be non-constant.  */
# define THREAD_GETMEM_NC(descr, member, idx) \
    ((void)(descr), (THREAD_SELF)->member[idx])

/* Set member of the thread descriptor directly.  */
# define THREAD_SETMEM(descr, member, value) \
    ((void)(descr), (THREAD_SELF)->member = (value))

/* Same as THREAD_SETMEM, but the member offset can be non-constant.  */
# define THREAD_SETMEM_NC(descr, member, idx, value) \
    ((void)(descr), (THREAD_SELF)->member[idx] = (value))

/* l_tls_offset == 0 is perfectly valid on PPC, so we have to use some
   different value to mean unset l_tls_offset.  */
# define NO_TLS_OFFSET		-1

#endif /* __ASSEMBLER__ */

#endif	/* tls.h */
