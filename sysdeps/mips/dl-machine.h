/* Machine-dependent ELF dynamic relocation inline functions.  MIPS version.
   Copyright (C) 1996, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
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
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/*  FIXME: Profiling of shared libraries is not implemented yet.  */
#ifndef dl_machine_h
#define dl_machine_h

#define ELF_MACHINE_NAME "MIPS"

#define ELF_MACHINE_NO_PLT

#include <entry.h>

#ifndef ENTRY_POINT
#error ENTRY_POINT needs to be defined for MIPS.
#endif

/* The offset of gp from GOT might be system-dependent.  It's set by
   ld.  The same value is also */
#define OFFSET_GP_GOT 0x7ff0

#ifndef _RTLD_PROLOGUE
# define _RTLD_PROLOGUE(entry)						\
	".globl\t" __STRING(entry) "\n\t"				\
	".ent\t" __STRING(entry) "\n\t"					\
	".type\t" __STRING(entry) ", @function\n"			\
	__STRING(entry) ":\n\t"
#endif

#ifndef _RTLD_EPILOGUE
# define _RTLD_EPILOGUE(entry)						\
	".end\t" __STRING(entry) "\n\t"					\
	".size\t" __STRING(entry) ", . - " __STRING(entry) "\n\t"
#endif

/* A reloc type used for ld.so cmdline arg lookups to reject PLT entries.
   This makes no sense on MIPS but we have to define this to R_MIPS_REL32
   to avoid the asserts in dl-lookup.c from blowing.  */
#define ELF_MACHINE_JMP_SLOT			R_MIPS_REL32
#define elf_machine_lookup_noplt_p(type)	(1)
#define elf_machine_lookup_noexec_p(type)	(0)

/* Translate a processor specific dynamic tag to the index
   in l_info array.  */
#define DT_MIPS(x) (DT_MIPS_##x - DT_LOPROC + DT_NUM)

/*
 * MIPS libraries are usually linked to a non-zero base address.  We
 * subtract the base address from the address where we map the object
 * to.  This results in more efficient address space usage.
 *
 * FIXME: By the time when MAP_BASE_ADDR is called we don't have the
 * DYNAMIC section read.  Until this is fixed make the assumption that
 * libraries have their base address at 0x5ffe0000.  This needs to be
 * fixed before we can safely get rid of this MIPSism.
 */
#if 0
#define MAP_BASE_ADDR(l) ((l)->l_info[DT_MIPS(BASE_ADDRESS)] ? \
			  (l)->l_info[DT_MIPS(BASE_ADDRESS)]->d_un.d_ptr : 0)
#else
#define MAP_BASE_ADDR(l) 0x5ffe0000
#endif

/* If there is a DT_MIPS_RLD_MAP entry in the dynamic section, fill it in
   with the run-time address of the r_debug structure  */
#define ELF_MACHINE_DEBUG_SETUP(l,r) \
do { if ((l)->l_info[DT_MIPS (RLD_MAP)]) \
       *(ElfW(Addr) *)((l)->l_info[DT_MIPS (RLD_MAP)]->d_un.d_ptr) = \
       (ElfW(Addr)) (r); \
   } while (0)

/* Return nonzero iff ELF header is compatible with the running host.  */
static inline int __attribute__ ((unused))
elf_machine_matches_host (const ElfW(Ehdr) *ehdr)
{
  switch (ehdr->e_machine)
    {
    case EM_MIPS:
    case EM_MIPS_RS3_LE:
      return 1;
    default:
      return 0;
    }
}

static inline ElfW(Addr) *
elf_mips_got_from_gpreg (ElfW(Addr) gpreg)
{
  /* FIXME: the offset of gp from GOT may be system-dependent. */
  return (ElfW(Addr) *) (gpreg - OFFSET_GP_GOT);
}

/* Return the link-time address of _DYNAMIC.  Conveniently, this is the
   first element of the GOT.  This must be inlined in a function which
   uses global data.  */
static inline ElfW(Addr)
elf_machine_dynamic (void)
{
  register ElfW(Addr) gp __asm__ ("$28");
  return *elf_mips_got_from_gpreg (gp);
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
       :	"=r" (addr)
       :	/* No inputs */
       :	"$31");
  return addr;
}

/* The MSB of got[1] of a gnu object is set to identify gnu objects.  */
#define ELF_MIPS_GNU_GOT1_MASK	0x80000000

/* We can't rely on elf_machine_got_rel because _dl_object_relocation_scope
   fiddles with global data.  */
#define ELF_MACHINE_BEFORE_RTLD_RELOC(dynamic_info)			\
do {									\
  struct link_map *map = &bootstrap_map;				\
  ElfW(Sym) *sym;							\
  ElfW(Addr) *got;							\
  int i, n;								\
									\
  got = (ElfW(Addr) *) D_PTR (map, l_info[DT_PLTGOT]);			\
									\
  if (__builtin_expect (map->l_addr == 0, 1))				\
    goto done;								\
									\
  /* got[0] is reserved. got[1] is also reserved for the dynamic object	\
     generated by gnu ld. Skip these reserved entries from		\
     relocation.  */							\
  i = (got[1] & ELF_MIPS_GNU_GOT1_MASK)? 2 : 1;				\
  n = map->l_info[DT_MIPS (LOCAL_GOTNO)]->d_un.d_val;			\
									\
  /* Add the run-time display to all local got entries. */		\
  while (i < n)								\
    got[i++] += map->l_addr;						\
									\
  /* Handle global got entries. */					\
  got += n;								\
  sym = (ElfW(Sym) *) D_PTR(map, l_info[DT_SYMTAB])			\
       + map->l_info[DT_MIPS (GOTSYM)]->d_un.d_val;			\
  i = (map->l_info[DT_MIPS (SYMTABNO)]->d_un.d_val			\
       - map->l_info[DT_MIPS (GOTSYM)]->d_un.d_val);			\
									\
  while (i--)								\
    {									\
      if (sym->st_shndx == SHN_UNDEF || sym->st_shndx == SHN_COMMON)	\
	*got = map->l_addr + sym->st_value;				\
      else if (ELFW(ST_TYPE) (sym->st_info) == STT_FUNC			\
	       && *got != sym->st_value)				\
	*got += map->l_addr;						\
      else if (ELFW(ST_TYPE) (sym->st_info) == STT_SECTION)		\
	{								\
	  if (sym->st_other == 0)					\
	    *got += map->l_addr;					\
	}								\
      else								\
	*got = map->l_addr + sym->st_value;				\
									\
      got++;								\
      sym++;								\
    }									\
done:									\
} while(0)


/* Get link map for callers object containing STUB_PC.  */
static inline struct link_map *
elf_machine_runtime_link_map (ElfW(Addr) gpreg, ElfW(Addr) stub_pc)
{
  extern int _dl_mips_gnu_objects;

  /* got[1] is reserved to keep its link map address for the shared
     object generated by the gnu linker.  If all are such objects, we
     can find the link map from current GPREG simply.  If not so, get
     the link map for caller's object containing STUB_PC.  */

  if (_dl_mips_gnu_objects)
    {
      ElfW(Addr) *got = elf_mips_got_from_gpreg (gpreg);
      ElfW(Word) g1;

      g1 = ((ElfW(Word) *) got)[1];

      if ((g1 & ELF_MIPS_GNU_GOT1_MASK) != 0)
	{
	  struct link_map *l =
	    (struct link_map *) (g1 & ~ELF_MIPS_GNU_GOT1_MASK);
	  ElfW(Addr) base, limit;
	  const ElfW(Phdr) *p = l->l_phdr;
	  ElfW(Half) this, nent = l->l_phnum;

	  /* For the common case of a stub being called from the containing
	     object, STUB_PC will point to somewhere within the object that
	     is described by the link map fetched via got[1].  Otherwise we
	     have to scan all maps.  */
	  for (this = 0; this < nent; this++)
	    {
	      if (p[this].p_type == PT_LOAD)
		{
		  base = p[this].p_vaddr + l->l_addr;
		  limit = base + p[this].p_memsz;
		  if (stub_pc >= base && stub_pc < limit)
		    return l;
		}
	    }
	}
    }

    {
      struct link_map *l = _dl_loaded;

      while (l)
	{
	  ElfW(Addr) base, limit;
	  const ElfW(Phdr) *p = l->l_phdr;
	  ElfW(Half) this, nent = l->l_phnum;

	  for (this = 0; this < nent; ++this)
	    {
	      if (p[this].p_type == PT_LOAD)
		{
		  base = p[this].p_vaddr + l->l_addr;
		  limit = base + p[this].p_memsz;
		  if (stub_pc >= base && stub_pc < limit)
		    return l;
		}
	    }
	  l = l->l_next;
	}
    }

  _dl_signal_error (0, NULL, "cannot find runtime link map");
  return NULL;
}

/* Define mips specific runtime resolver. The function __dl_runtime_resolve
   is called from assembler function _dl_runtime_resolve which converts
   special argument registers t7 ($15) and t8 ($24):
     t7  address to return to the caller of the function
     t8  index for this function symbol in .dynsym
   to usual c arguments.

   Other architectures call fixup from dl-runtime.c in
   _dl_runtime_resolve.  MIPS instead calls __dl_runtime_resolve.  We
   have to use our own version because of the way the got section is
   treated on MIPS (we've also got ELF_MACHINE_PLT defined).  */

#define ELF_MACHINE_RUNTIME_TRAMPOLINE					      \
/* The flag _dl_mips_gnu_objects is set if all dynamic objects are	      \
   generated by the gnu linker. */					      \
int _dl_mips_gnu_objects = 1;						      \
									      \
/* This is called from assembly stubs below which the compiler can't see.  */ \
static ElfW(Addr)							      \
__dl_runtime_resolve (ElfW(Word), ElfW(Word), ElfW(Addr), ElfW(Addr))	      \
		  __attribute__ ((unused));				      \
									      \
static ElfW(Addr)							      \
__dl_runtime_resolve (ElfW(Word) sym_index,				      \
		      ElfW(Word) return_address,			      \
		      ElfW(Addr) old_gpreg,				      \
		      ElfW(Addr) stub_pc)				      \
{									      \
  struct link_map *l = elf_machine_runtime_link_map (old_gpreg, stub_pc);     \
  const ElfW(Sym) *const symtab						      \
    = (const void *) D_PTR (l, l_info[DT_SYMTAB]);			      \
  const char *strtab							      \
    = (const void *) D_PTR (l, l_info[DT_STRTAB]);			      \
  const ElfW(Addr) *got							      \
    = (const ElfW(Addr) *) D_PTR (l, l_info[DT_PLTGOT]);		      \
  const ElfW(Word) local_gotno						      \
    = (const ElfW(Word)) l->l_info[DT_MIPS (LOCAL_GOTNO)]->d_un.d_val;	      \
  const ElfW(Word) gotsym						      \
    = (const ElfW(Word)) l->l_info[DT_MIPS (GOTSYM)]->d_un.d_val;	      \
  const ElfW(Sym) *sym = &symtab[sym_index];				      \
  ElfW(Addr) value;							      \
									      \
  /* FIXME: The symbol versioning stuff is not tested yet.  */		      \
  if (__builtin_expect (ELFW(ST_VISIBILITY) (sym->st_other), 0) == 0)	      \
    {									      \
      switch (l->l_info[VERSYMIDX (DT_VERSYM)] != NULL)			      \
	{								      \
	default:							      \
	  {								      \
	    const ElfW(Half) *vernum =					      \
	      (const void *) D_PTR (l, l_info[VERSYMIDX (DT_VERSYM)]);	      \
	    ElfW(Half) ndx = vernum[sym_index];				      \
	    const struct r_found_version *version = &l->l_versions[ndx];      \
									      \
	    if (version->hash != 0)					      \
	      {								      \
		value = _dl_lookup_versioned_symbol(strtab + sym->st_name, l, \
						    &sym, l->l_scope, version,\
						    R_MIPS_REL32, 0);	      \
		break;							      \
	      }								      \
	    /* Fall through.  */					      \
	  }								      \
	case 0:								      \
	  value = _dl_lookup_symbol (strtab + sym->st_name, l, &sym,	      \
				     l->l_scope, R_MIPS_REL32, 0);	      \
	}								      \
									      \
      /* Currently value contains the base load address of the object	      \
	 that defines sym.  Now add in the symbol offset.  */		      \
      value = (sym ? value + sym->st_value : 0);			      \
    }									      \
  else									      \
    /* We already found the symbol.  The module (and therefore its load	      \
       address) is also known.  */					      \
    value = l->l_addr + sym->st_value;					      \
									      \
  /* Apply the relocation with that value.  */				      \
  *(got + local_gotno + sym_index - gotsym) = value;			      \
									      \
  return value;								      \
}									      \
									      \
asm ("\n								      \
	.text\n								      \
	.align	2\n							      \
	.globl	_dl_runtime_resolve\n					      \
	.type	_dl_runtime_resolve,@function\n				      \
	.ent	_dl_runtime_resolve\n					      \
_dl_runtime_resolve:\n							      \
	.frame	$29, 40, $31\n						      \
	.set noreorder\n						      \
	# Save GP.\n							      \
	move	$3, $28\n						      \
	# Modify t9 ($25) so as to point .cpload instruction.\n		      \
	addu	$25, 8\n						      \
	# Compute GP.\n							      \
	.cpload $25\n							      \
	.set reorder\n							      \
	# Save slot call pc.\n						      \
	move	$2, $31\n						      \
	# Save arguments and sp value in stack.\n			      \
	subu	$29, 40\n						      \
	.cprestore 32\n							      \
	sw	$15, 36($29)\n						      \
	sw	$4, 16($29)\n						      \
	sw	$5, 20($29)\n						      \
	sw	$6, 24($29)\n						      \
	sw	$7, 28($29)\n						      \
	move	$4, $24\n						      \
	move	$5, $15\n						      \
	move	$6, $3\n						      \
	move	$7, $2\n						      \
	jal	__dl_runtime_resolve\n					      \
	lw	$31, 36($29)\n						      \
	lw	$4, 16($29)\n						      \
	lw	$5, 20($29)\n						      \
	lw	$6, 24($29)\n						      \
	lw	$7, 28($29)\n						      \
	addu	$29, 40\n						      \
	move	$25, $2\n						      \
	jr	$25\n							      \
	.end	_dl_runtime_resolve\n					      \
	.previous\n							      \
");

/* Mask identifying addresses reserved for the user program,
   where the dynamic linker should not map anything.  */
#define ELF_MACHINE_USER_ADDRESS_MASK	0x80000000UL



/* Initial entry point code for the dynamic linker.
   The C function `_dl_start' is the real entry point;
   its return value is the user program's entry point.
   Note how we have to be careful about two things:

   1) That we allocate a minimal stack of 24 bytes for
      every function call, the MIPS ABI states that even
      if all arguments are passed in registers the procedure
      called can use the 16 byte area pointed to by $sp
      when it is called to store away the arguments passed
      to it.

   2) That under Linux the entry is named __start
      and not just plain _start.  */

#define RTLD_START asm (\
	".text\n"\
	_RTLD_PROLOGUE(ENTRY_POINT)\
	".set noreorder\n\
	.set noreorder\n\
	bltzal $0, 0f\n\
	nop\n\
0:	.cpload $31\n\
	.set reorder\n\
	# i386 ABI book says that the first entry of GOT holds\n\
	# the address of the dynamic structure. Though MIPS ABI\n\
	# doesn't say nothing about this, I emulate this here.\n\
	la $4, _DYNAMIC\n\
	# Subtract OFFSET_GP_GOT\n\
	sw $4, -0x7ff0($28)\n\
	move $4, $29\n\
	subu $29, 16\n\
	\n\
	la $8, coff\n\
	bltzal $8, coff\n\
coff:	subu $8, $31, $8\n\
	\n\
	la $25, _dl_start\n\
	addu $25, $8\n\
	jalr $25\n\
	\n\
	addiu $29, 16\n\
	# Get the value of label '_dl_start_user' in t9 ($25).\n\
	la $25, _dl_start_user\n\
	.globl _dl_start_user\n\
_dl_start_user:\n\
	.set noreorder\n\
	.cpload $25\n\
	.set reorder\n\
	move $16, $28\n\
	# Save the user entry point address in a saved register.\n\
	move $17, $2\n\
	# Store the highest stack address\n\
	sw $29, __libc_stack_end\n\
	# See if we were run as a command with the executable file\n\
	# name as an extra leading argument.\n\
	lw $2, _dl_skip_args\n\
	beq $2, $0, 1f\n\
	# Load the original argument count.\n\
	lw $4, 0($29)\n\
	# Subtract _dl_skip_args from it.\n\
	subu $4, $2\n\
	# Adjust the stack pointer to skip _dl_skip_args words.\n\
	sll $2, 2\n\
	addu $29, $2\n\
	# Save back the modified argument count.\n\
	sw $4, 0($29)\n\
1:	# Call _dl_init (struct link_map *main_map, int argc, char **argv, char **env) \n\
	lw $4, _dl_loaded\n\
	lw $5, 0($29)\n\
	la $6, 4($29)\n\
	sll $7, $5, 2\n\
	addu $7, $7, $6\n\
	addu $7, $7, 4\n\
	subu $29, 16\n\
	# Call the function to run the initializers.\n\
	jal _dl_init
	addiu $29, 16\n\
	# Pass our finalizer function to the user in $2 as per ELF ABI.\n\
	la $2, _dl_fini\n\
	# Jump to the user entry point.\n\
	move $25, $17\n\
	jr $25\n\t"\
	_RTLD_EPILOGUE(ENTRY_POINT)\
	".previous"\
);

/* The MIPS never uses Elfxx_Rela relocations.  */
#define ELF_MACHINE_NO_RELA 1

#endif /* !dl_machine_h */

#ifdef RESOLVE

/* Perform the relocation specified by RELOC and SYM (which is fully resolved).
   MAP is the object containing the reloc.  */

static inline void
elf_machine_rel (struct link_map *map, const ElfW(Rel) *reloc,
		 const ElfW(Sym) *sym, const struct r_found_version *version,
		 ElfW(Addr) *const reloc_addr)
{
#ifndef RTLD_BOOTSTRAP
  /* This is defined in rtld.c, but nowhere in the static libc.a;
     make the reference weak so static programs can still link.  This
     declaration cannot be done when compiling rtld.c (i.e.  #ifdef
     RTLD_BOOTSTRAP) because rtld.c contains the common defn for
     _dl_rtld_map, which is incompatible with a weak decl in the same
     file.  */
  weak_extern (_dl_rtld_map);
#endif

  switch (ELFW(R_TYPE) (reloc->r_info))
    {
    case R_MIPS_REL32:
#ifndef RTLD_BOOTSTRAP
      if (map != &_dl_rtld_map)
#endif
	*reloc_addr += map->l_addr;
      break;
    case R_MIPS_NONE:		/* Alright, Wilbur.  */
      break;
    default:
      _dl_reloc_bad_type (map, ELFW(R_TYPE) (reloc->r_info), 0);
      break;
    }
}

static inline void
elf_machine_lazy_rel (struct link_map *map,
		      ElfW(Addr) l_addr, const ElfW(Rel) *reloc)
{
  /* Do nothing.  */
}

/* Relocate GOT. */
static inline void
elf_machine_got_rel (struct link_map *map, int lazy)
{
  ElfW(Addr) *got;
  ElfW(Sym) *sym;
  int i, n, symidx;
  /*  This function is loaded in dl-reloc as a nested function and can
      therefore access the variables scope and strtab from
      _dl_relocate_object.  */
#ifdef RTLD_BOOTSTRAP
# define RESOLVE_GOTSYM(sym,sym_index) 0
#else
# define RESOLVE_GOTSYM(sym,sym_index)					  \
    ({									  \
      const ElfW(Sym) *ref = sym;					  \
      ElfW(Addr) value;							  \
									  \
      switch (map->l_info[VERSYMIDX (DT_VERSYM)] != NULL)		  \
	{								  \
	default:							  \
	  {								  \
	    const ElfW(Half) *vernum =					  \
	      (const void *) D_PTR (map, l_info[VERSYMIDX (DT_VERSYM)]);  \
	    ElfW(Half) ndx = vernum[sym_index];				  \
	    const struct r_found_version *version = &l->l_versions[ndx];  \
									  \
	    if (version->hash != 0)					  \
	      {								  \
		value = _dl_lookup_versioned_symbol(strtab + sym->st_name,\
						    map,		  \
						    &ref, scope, version, \
						    R_MIPS_REL32, 0);	  \
		break;							  \
	      }								  \
	    /* Fall through.  */					  \
	  }								  \
	case 0:								  \
	  value = _dl_lookup_symbol (strtab + sym->st_name, map, &ref,	  \
				     scope, R_MIPS_REL32, 0);		  \
	}								  \
									  \
      (ref)? value + ref->st_value: 0;					  \
    })
#endif /* RTLD_BOOTSTRAP */

  got = (ElfW(Addr) *) D_PTR (map, l_info[DT_PLTGOT]);

  n = map->l_info[DT_MIPS (LOCAL_GOTNO)]->d_un.d_val;
  /* The dynamic linker's local got entries have already been relocated.  */
  if (map != &_dl_rtld_map)
    {
      /* got[0] is reserved. got[1] is also reserved for the dynamic object
	 generated by gnu ld. Skip these reserved entries from relocation.  */
      i = (got[1] & ELF_MIPS_GNU_GOT1_MASK)? 2 : 1;

      /* Add the run-time display to all local got entries if needed. */
      if (__builtin_expect (map->l_addr != 0, 0))
	{
	  while (i < n)
	    got[i++] += map->l_addr;
	}
    }

  /* Handle global got entries. */
  got += n;
  /* Keep track of the symbol index.  */
  symidx = map->l_info[DT_MIPS (GOTSYM)]->d_un.d_val;
  sym = (ElfW(Sym) *) D_PTR (map, l_info[DT_SYMTAB]) + symidx;
  i = (map->l_info[DT_MIPS (SYMTABNO)]->d_un.d_val
       - map->l_info[DT_MIPS (GOTSYM)]->d_un.d_val);

  /* This loop doesn't handle Quickstart.  */
  while (i--)
    {
      if (sym->st_shndx == SHN_UNDEF)
	{
	  if (ELFW(ST_TYPE) (sym->st_info) == STT_FUNC
	      && sym->st_value && lazy)
	    *got = sym->st_value + map->l_addr;
	  else
	    *got = RESOLVE_GOTSYM (sym, symidx);
	}
      else if (sym->st_shndx == SHN_COMMON)
	*got = RESOLVE_GOTSYM (sym, symidx);
      else if (ELFW(ST_TYPE) (sym->st_info) == STT_FUNC
	       && *got != sym->st_value
	       && lazy)
	*got += map->l_addr;
      else if (ELFW(ST_TYPE) (sym->st_info) == STT_SECTION)
	{
	  if (sym->st_other == 0)
	    *got += map->l_addr;
	}
      else
	*got = RESOLVE_GOTSYM (sym, symidx);

      ++got;
      ++sym;
      ++symidx;
    }

#undef RESOLVE_GOTSYM

  return;
}

/* Set up the loaded object described by L so its stub function
   will jump to the on-demand fixup code __dl_runtime_resolve.  */

static inline int
elf_machine_runtime_setup (struct link_map *l, int lazy, int profile)
{
# ifndef RTLD_BOOTSTRAP
  ElfW(Addr) *got;
  extern void _dl_runtime_resolve (ElfW(Word));
  extern int _dl_mips_gnu_objects;

  if (lazy)
    {
      /* The GOT entries for functions have not yet been filled in.
	 Their initial contents will arrange when called to put an
	 offset into the .dynsym section in t8, the return address
	 in t7 and then jump to _GLOBAL_OFFSET_TABLE[0].  */
      got = (ElfW(Addr) *) D_PTR (l, l_info[DT_PLTGOT]);

      /* This function will get called to fix up the GOT entry indicated by
	 the register t8, and then jump to the resolved address.  */
      got[0] = (ElfW(Addr)) &_dl_runtime_resolve;

      /* Store l to _GLOBAL_OFFSET_TABLE[1] for gnu object. The MSB
	 of got[1] of a gnu object is set to identify gnu objects.
	 Where we can store l for non gnu objects? XXX  */
      if ((got[1] & ELF_MIPS_GNU_GOT1_MASK) != 0)
	got[1] = (ElfW(Addr)) ((unsigned) l | ELF_MIPS_GNU_GOT1_MASK);
      else
	_dl_mips_gnu_objects = 0;
    }

  /* Relocate global offset table.  */
  elf_machine_got_rel (l, lazy);

# endif
  return lazy;
}

#endif /* RESOLVE */
