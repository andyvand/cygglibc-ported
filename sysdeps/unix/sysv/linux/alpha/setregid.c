/* Copyright (C) 1998, 2000, 2003, 2004 Free Software Foundation, Inc.
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

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include <sysdep.h>
#include <sys/syscall.h>

#include <linux/posix_types.h>
#include "kernel-features.h"
#include <pthread-functions.h>


int
__setregid (gid_t rgid, gid_t egid)
{
  int result;

  result = INLINE_SYSCALL (setregid, 2, (signed int)rgid, (signed int)egid);

#if defined HAVE_PTR__NPTL_SETXID && !defined SINGLE_THREAD
  if (result == 0 && __libc_pthread_functions.ptr__nptl_setxid != NULL)
    {
      struct xid_command cmd;
      cmd.syscall_no = __NR_setregid;
      cmd.id[0] = rgid;
      cmd.id[1] = egid;
      __libc_pthread_functions.ptr__nptl_setxid (&cmd);
    }
#endif

  return result;
}
#ifndef __setregid
weak_alias (__setregid, setregid)
#endif
