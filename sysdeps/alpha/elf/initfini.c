/* Special .init and .fini section support for Alpha.
   Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.
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

/* This file is compiled into assembly code which is then munged by a sed
   script into two files: crti.s and crtn.s.

   * crti.s puts a function prologue at the beginning of the .init and .fini
   sections and defines global symbols for those addresses, so they can be
   called as functions.

   * crtn.s puts the corresponding function epilogues in the .init and .fini
   sections.

   This differs from what would be generated by the generic code in that
   we save and restore the GP within the function.  In order for linker
   relaxation to work, the value in the GP register on exit from a function
   must be valid for the function entry point.  Normally, a function is
   contained within one object file and this is not an issue, provided
   that the function reloads the gp after making any function calls.
   However, _init and _fini are constructed from pieces of many object
   files, all of which may have different GP values.  So we must reload
   the GP value from crti.o in crtn.o.  */

__asm__ ("						\n\
#include \"defs.h\"					\n\
							\n\
/*@HEADER_ENDS*/					\n\
							\n\
/*@_init_PROLOG_BEGINS*/				\n\
	.section .init, \"ax\", @progbits		\n\
	.globl	_init					\n\
	.type	_init, @function			\n\
	.usepv	_init, std				\n\
_init:							\n\
	ldgp	$29, 0($27)				\n\
	subq	$30, 16, $30				\n\
	lda	$27, __gmon_start__			\n\
	stq	$26, 0($30)				\n\
	stq	$29, 8($30)				\n\
	beq	$27, 1f					\n\
	jsr	$26, ($27), __gmon_start__		\n\
	ldq	$29, 8($30)				\n\
	.align 3					\n\
1:							\n\
/*@_init_PROLOG_ENDS*/					\n\
							\n\
/*@_init_EPILOG_BEGINS*/				\n\
	.section .init, \"ax\", @progbits		\n\
	ldq	$26, 0($30)				\n\
	ldq	$29, 8($30)				\n\
	addq	$30, 16, $30				\n\
	ret						\n\
/*@_init_EPILOG_ENDS*/					\n\
							\n\
/*@_fini_PROLOG_BEGINS*/				\n\
	.section .fini, \"ax\", @progbits		\n\
	.globl	_fini					\n\
	.type	_fini,@function				\n\
	.usepv	_fini,std				\n\
_fini:							\n\
	ldgp	$29, 0($27)				\n\
	subq	$30, 16, $30				\n\
	stq	$26, 0($30)				\n\
	stq	$29, 8($30)				\n\
	.align 3					\n\
/*@_fini_PROLOG_ENDS*/					\n\
							\n\
/*@_fini_EPILOG_BEGINS*/				\n\
	.section .fini, \"ax\", @progbits		\n\
	ldq	$26, 0($30)				\n\
	ldq	$29, 8($30)				\n\
	addq	$30, 16, $30				\n\
	ret						\n\
/*@_fini_EPILOG_ENDS*/					\n\
							\n\
/*@TRAILER_BEGINS*/					\n\
");
