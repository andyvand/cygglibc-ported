/* Machine-dependent ELF dynamic relocation inline functions.  mips version.
Copyright (C) 1996 Free Software Foundation, Inc.
This file is part of the GNU C Library.
Contributed by Kazumoto Kojima <kkojima@info.kanagawa-u.ac.jp>.

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

#define ELF_MACHINE_NAME "MIPS"

#include <assert.h>
#include <string.h>
#include <link.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

/* DT_MIPS macro ranslate a processor specific dynamic tag to the index
   in l_info array.  */
#define DT_MIPS(x) (DT_MIPS_##x - DT_LOPROC + DT_NUM)

#if 1
/* XXX If FLAGS has the MAP_ALIGN bit, we need 64k alignement. */
#ifndef MAP_ALIGN
#define MAP_ALIGN 0x1000
#endif
#define ELF_MACHINE_ALIGN_MASK(flags) ((flags & MAP_ALIGN) ? 0xffff : 0)
#endif

/* If there is a DT_MIPS_RLD_MAP entry in the dynamic section, fill it in
   with the run-time address of the r_debug structure  */
#define ELF_MACHINE_SET_DEBUG(l,r) \
do { if ((l)->l_info[DT_MIPS (RLD_MAP)]) \
       *(ElfW(Addr) *)((l)->l_info[DT_MIPS (RLD_MAP)]->d_un.d_ptr) = \
       (ElfW(Addr)) (r); \
   } while (0)

/* Return nonzero iff E_MACHINE is compatible with the running host.  */
static inline int
elf_machine_matches_host (ElfW(Half) e_machine)
{
  switch (e_machine)
    {
    case EM_MIPS:
    case EM_MIPS_RS4_BE:
      return 1;
    default:
      return 0;
    }
}

static inline ElfW(Addr) *
elf_mips_got_from_gpreg (ElfW(Addr) gpreg)
{
  /* FIXME: the offset of gp from GOT may be system-dependent. */
  return (ElfW(Addr) *) (gpreg - 0x7ff0);
}

/* Return the run-time address of the _GLOBAL_OFFSET_TABLE_.
   Must be inlined in a function which uses global data.  */
static inline ElfW(Addr) *
elf_machine_got (void)
{
  register ElfW(Addr) gpreg asm ("$28");
  return elf_mips_got_from_gpreg (gpreg);
}


/* Return the run-time load address of the shared object.  */
static inline ElfW(Addr)
elf_machine_load_address (void)
{
  ElfW(Addr) addr;
  asm ("	.set noreorder\n"
       "	la %0, here\n"
       "	bltzal $0, here\n"
       "	nop\n"
       "here:	subu %0, $31, %0\n"
       "	.set reorder\n"
       : "=r" (addr));
  return addr;
}

/* Perform the relocation specified by RELOC and SYM (which is fully resolved).
   MAP is the object containing the reloc.  */

static inline void
elf_machine_rel (struct link_map *map,
		 const ElfW(Rel) *reloc, const ElfW(Sym) *sym,
		 ElfW(Addr) (*resolve) (const ElfW(Sym) **ref,
					ElfW(Addr) reloc_addr,
					int noplt))
{
  ElfW(Addr) *const reloc_addr = (void *) (map->l_addr + reloc->r_offset);
  ElfW(Addr) loadbase, undo;

  switch (ELFW(R_TYPE) (reloc->r_info))
    {
    case R_MIPS_REL32:
      if (ELFW(ST_BIND) (sym->st_info) == STB_LOCAL
	  && (ELFW(ST_TYPE) (sym->st_info) == STT_SECTION
	      || ELFW(ST_TYPE) (sym->st_info) == STT_NOTYPE))
	*reloc_addr += map->l_addr;
      else
	{
	  if (resolve && map == &_dl_rtld_map)
	    /* Undo the relocation done here during bootstrapping.  Now we will
	       relocate it anew, possibly using a binding found in the user
	       program or a loaded library rather than the dynamic linker's
	       built-in definitions used while loading those libraries.  */
	    undo = map->l_addr + sym->st_value;
	  else
	    undo = 0;
	  loadbase = (resolve ? (*resolve) (&sym, (ElfW(Addr)) reloc_addr, 0) :
		      /* RESOLVE is null during bootstrap relocation.  */
		      map->l_addr);
	  *reloc_addr += (sym ? (loadbase + sym->st_value) : 0) - undo;
	}
      break;
    case R_MIPS_NONE:		/* Alright, Wilbur.  */
      break;
    default:
      assert (! "unexpected dynamic reloc type");
      break;
    }
}

static inline void
elf_machine_lazy_rel (struct link_map *map, const ElfW(Rel) *reloc)
{
  /* Do nothing.  */
}

/* The MSB of got[1] of a gnu object is set to identify gnu objects. */
#define ELF_MIPS_GNU_GOT1_MASK 0x80000000

/* Relocate GOT. */
static void
elf_machine_got_rel (struct link_map *map)
{
  ElfW(Addr) *got;
  ElfW(Sym) *sym;
  int i, n;
  struct link_map **scope;
  const char *strtab
    = ((void *) map->l_addr + map->l_info[DT_STRTAB]->d_un.d_ptr);

  ElfW(Addr) resolve (const ElfW(Sym) *sym)
    {
      ElfW(Sym) *ref = sym;
      ElfW(Addr) sym_loadaddr;
      sym_loadaddr = _dl_lookup_symbol (strtab + sym->st_name, &ref, scope,
					map->l_name, 0, 1);
      return (ref)? sym_loadaddr + ref->st_value: 0;
    }

  got = (ElfW(Addr) *) ((void *) map->l_addr
			+ map->l_info[DT_PLTGOT]->d_un.d_ptr);

  /* got[0] is reserved. got[1] is also reserved for the dynamic object
     generated by gnu ld. Skip these reserved entries from relocation.  */
  i = (got[1] & ELF_MIPS_GNU_GOT1_MASK)? 2: 1;
  n = map->l_info[DT_MIPS (LOCAL_GOTNO)]->d_un.d_val;
  /* Add the run-time display to all local got entries. */
  while (i < n)
    got[i++] += map->l_addr;

  /* Set scope.  */
  scope = _dl_object_relocation_scope (map);

  /* Handle global got entries. */
  got += n;
  sym = (ElfW(Sym) *) ((void *) map->l_addr
		       + map->l_info[DT_SYMTAB]->d_un.d_ptr);
  sym += map->l_info[DT_MIPS (GOTSYM)]->d_un.d_val;
  i = (map->l_info[DT_MIPS (SYMTABNO)]->d_un.d_val
       - map->l_info[DT_MIPS (GOTSYM)]->d_un.d_val);

  while (i--)
    {
      if (sym->st_shndx == SHN_UNDEF)
	{
	  if (ELFW(ST_TYPE) (sym->st_info) == STT_FUNC)
	    {
	      if (sym->st_value /* && maybe_stub (sym->st_value) */)
		*got = sym->st_value + map->l_addr;
	      else
		*got = resolve (sym);
	    }
	  else /* if (*got == 0 || *got == QS) */
	    *got = resolve (sym);
	}
      else if (sym->st_shndx == SHN_COMMON)
	*got = resolve (sym);
      else if (ELFW(ST_TYPE) (sym->st_info) == STT_FUNC
	       && *got != sym->st_value
	       /* && maybe_stub (*got) */)
	*got += map->l_addr;
      else if (ELFW(ST_TYPE) (sym->st_info) == STT_SECTION
	       && ELFW(ST_BIND) (sym->st_info) == STB_GLOBAL)
	{
	  if (sym->st_other == 0 && sym->st_shndx == SHN_ABS)
	    *got = sym->st_value + map->l_addr; /* only for _gp_disp  */
	  /* else SGI stuff ignored */
	}
      else
	*got = resolve (sym);

      got++;
      sym++;
    }

  *_dl_global_scope_end = NULL;

  return;
}

/* The MIPS never uses Elfxx_Rela relocations.  */
#define ELF_MACHINE_NO_RELA 1

/* Set up the loaded object described by L so its stub function
   will jump to the on-demand fixup code in dl-runtime.c.  */

static inline void
elf_machine_runtime_setup (struct link_map *l, int lazy)
{
  ElfW(Addr) *got;
  extern void _dl_runtime_resolve (ElfW(Word));

  if (lazy)
    {
      /* The GOT entries for functions have not yet been filled in.
	 Their initial contents will arrange when called to put an
	 offset into the .dynsym section in t8, the return address
	 in t7 and then jump to _GLOBAL_OFFSET_TABLE[0].  */
      got = (ElfW(Addr) *) ((void *) l->l_addr
			    + l->l_info[DT_PLTGOT]->d_un.d_ptr);

      /* This function will get called to fix up the GOT entry indicated by
	 the register t8, and then jump to the resolved address.  */
      got[0] = (ElfW(Addr)) &_dl_runtime_resolve;

      /* Store l to _GLOBAL_OFFSET_TABLE[1] for gnu object. The MSB
	 of got[1] of a gnu object is set to identify gnu objects.
	 Where we can store l for non gnu objects? XXX  */
      if ((got[1] & ELF_MIPS_GNU_GOT1_MASK) != 0)
	got[1] = (ElfW(Addr)) ((unsigned) l | ELF_MIPS_GNU_GOT1_MASK);
      else
	; /* Do nothing. */
    }

  /* Relocate global offset table.  */
  elf_machine_got_rel (l);
}

/* Get link_map for this object.  */
static inline struct link_map *
elf_machine_runtime_link_map (ElfW(Addr) gpreg)
{
  ElfW(Addr) *got = elf_mips_got_from_gpreg (gpreg);
  ElfW(Word) g1;

  g1 = ((ElfW(Word) *) got)[1];

  /* got[1] is reserved to keep its link map address for the shared
     object generated by gnu linker. If not so, we must search GOT
     in object list slowly. XXX  */
  if ((g1 & ELF_MIPS_GNU_GOT1_MASK) != 0)
    return (struct link_map *) (g1 & ~ELF_MIPS_GNU_GOT1_MASK);
  else
    {
      struct link_map *l = _dl_loaded;
      while (l)
	{
	  if (got == (ElfW(Addr) *) ((void *) l->l_addr
				     + l->l_info[DT_PLTGOT]->d_un.d_ptr))
	    return l;
	  l = l->l_next;
	}
    }
  _dl_signal_error (0, NULL, "cannot find runtime link map");
}

/* Mips has no PLT but define elf_machine_relplt to be elf_machine_rel. */
#define elf_machine_relplt elf_machine_rel

/* Define mips specific runtime resolver. The function __dl_runtime_resolve
   is called from assembler function _dl_runtime_resolve which converts
   special argument registers t7 ($15) and t8 ($24):
     t7  address to return to the caller of the function
     t8  index for this function symbol in .dynsym
   to usual c arguments.  */

#define ELF_MACHINE_RUNTIME_TRAMPOLINE \
static ElfW(Addr) \
__dl_runtime_resolve (ElfW(Word) sym_index,\
		      ElfW(Word) return_address,\
		      ElfW(Addr) old_gpreg)\
{\
  struct link_map *l = elf_machine_runtime_link_map (old_gpreg);\
  const ElfW(Sym) *const symtab\
    = (const ElfW(Sym) *) (l->l_addr + l->l_info[DT_SYMTAB]->d_un.d_ptr);\
  const char *strtab\
    = (void *) (l->l_addr + l->l_info[DT_STRTAB]->d_un.d_ptr);\
  const ElfW(Addr) *got\
    = (const ElfW(Addr) *) (l->l_addr + l->l_info[DT_PLTGOT]->d_un.d_ptr);\
  const ElfW(Word) local_gotno\
    = (const ElfW(Word)) l->l_info[DT_MIPS (LOCAL_GOTNO)]->d_un.d_val;\
  const ElfW(Word) gotsym\
    = (const ElfW(Word)) l->l_info[DT_MIPS (GOTSYM)]->d_un.d_val;\
  const ElfW(Sym) *definer;\
  ElfW(Addr) loadbase;\
  ElfW(Addr) funcaddr;\
  struct link_map **scope;\
\
  /* Look up the symbol's run-time value.  */\
  scope = _dl_object_relocation_scope (l);\
  definer = &symtab[sym_index];\
\
  loadbase = _dl_lookup_symbol (strtab + definer->st_name, &definer,\
				scope, l->l_name, 0, 1);\
\
  *_dl_global_scope_end = NULL;\
\
  /* Apply the relocation with that value.  */\
  funcaddr = loadbase + definer->st_value;\
  *(got + local_gotno + sym_index - gotsym) = funcaddr;\
\
  return funcaddr;\
}\
\
asm ("\n\
	.text\n\
	.align	2\n\
	.globl	_dl_runtime_resolve\n\
	.type	_dl_runtime_resolve,@function\n\
	.ent	_dl_runtime_resolve\n\
_dl_runtime_resolve:\n\
	.set noreorder\n\
	# Save old GP to $3.\n\
	move	$3,$28\n\
	# Modify t9 ($25) so as to point .cpload instruction.\n\
	addu	$25,8\n\
	# Compute GP.\n\
	.cpload $25\n\
	.set reorder\n\
	# Save arguments and sp value in stack.\n\
	subu	$29, 40\n\
	.cprestore 32\n\
	sw	$15, 36($29)\n\
	sw	$4, 12($29)\n\
	sw	$5, 16($29)\n\
	sw	$6, 20($29)\n\
	sw	$7, 24($29)\n\
	sw	$16, 28($29)\n\
	move	$16, $29\n\
	move	$4, $24\n\
	move	$5, $15\n\
	move	$6, $3\n\
	jal	__dl_runtime_resolve\n\
	move	$29, $16\n\
	lw	$31, 36($29)\n\
	lw	$4, 12($29)\n\
	lw	$5, 16($29)\n\
	lw	$6, 20($29)\n\
	lw	$7, 24($29)\n\
	lw	$16, 28($29)\n\
	addu	$29, 40\n\
	move	$25, $2\n\
	jr	$25\n\
	.end	_dl_runtime_resolve\n\
");

/* Mask identifying addresses reserved for the user program,
   where the dynamic linker should not map anything.  */
#define ELF_MACHINE_USER_ADDRESS_MASK	0x00000000UL



/* Initial entry point code for the dynamic linker.
   The C function `_dl_start' is the real entry point;
   its return value is the user program's entry point.  */

#define RTLD_START asm ("\
	.text\n\
	.globl _start\n\
	.globl _dl_start_user\n\
	.ent _start\n\
_start:\n\
	.set noreorder\n\
	bltzal $0, 0f\n\
	nop\n\
0:	.cpload $31\n\
	.set reorder\n\
	# i386 ABI book says that the first entry of GOT holds\n\
	# the address of the dynamic structure. Though MIPS ABI\n\
	# doesn't say nothing about this, I emulate this here.\n\
	la $4, _DYNAMIC\n\
	sw $4, -0x7ff0($28)\n\
	move $4, $29\n\
	jal _dl_start\n\
	# Get the value of label '_dl_start_user' in t9 ($25).\n\
	la $25, _dl_start_user\n\
_dl_start_user:\n\
	.set noreorder\n\
	.cpload $25\n\
	.set reorder\n\
	move $16, $28\n\
	# Save the user entry point address in saved register.\n\
	move $17, $2\n\
	# See if we were run as a command with the executable file\n\
	# name as an extra leading argument.\n\
	lw $2, _dl_skip_args\n\
	beq $2, $0, 1f\n\
	# Load the original argument count.\n\
	lw $4, 0($29)\n\
	# Subtract _dl_skip_args from it.\n\
	subu $4, $2\n\
	# Adjust the stack pointer to skip _dl_skip_args words.\n\
	sll $2,2\n\
	addu $29, $2\n\
	# Save back the modified argument count.\n\
	sw $4, 0($29)\n\
	# Get _dl_default_scope[2] as argument in _dl_init_next call below.\n\
1:	la $2, _dl_default_scope\n\
	lw $4, 8($2)\n\
	# Call _dl_init_next to return the address of an initializer\n\
	# function to run.\n\
	jal _dl_init_next\n\
	move $28, $16\n\
	# Check for zero return,  when out of initializers.\n\
	beq $2, $0, 2f\n\
	# Call the shared object initializer function.\n\
	move $25, $2\n\
	lw $4, 0($29)\n\
	lw $5, 4($29)\n\
	lw $6, 8($29)\n\
	lw $7, 12($29)\n\
	jalr $25\n\
	move $28, $16\n\
	# Loop to call _dl_init_next for the next initializer.\n\
	b 1b\n\
	# Pass our finalizer function to the user in ra.\n\
2:	la $31, _dl_fini\n\
	# Jump to the user entry point.\n\
	move $25, $17\n\
	lw $4, 0($29)\n\
	lw $5, 4($29)\n\
	lw $6, 8($29)\n\
	lw $7, 12($29)\n\
	jr $25\n\
	.end _start\n\
");

