
# Global ctors:


Ref0: From gcc source file: `/gcc/target-def.h`:
```cpp
#if !defined(TARGET_ASM_CONSTRUCTOR) && !defined(USE_COLLECT2)
# ifdef CTORS_SECTION_ASM_OP
#  define TARGET_ASM_CONSTRUCTOR default_ctor_section_asm_out_constructor
# else
#  ifdef TARGET_ASM_NAMED_SECTION
#   define TARGET_ASM_CONSTRUCTOR default_named_section_asm_out_constructor
#  else
#   define TARGET_ASM_CONSTRUCTOR default_asm_out_constructor
#  endif
# endif
#endif

#if !defined(TARGET_ASM_DESTRUCTOR) && !defined(USE_COLLECT2)
# ifdef DTORS_SECTION_ASM_OP
#  define TARGET_ASM_DESTRUCTOR default_dtor_section_asm_out_destructor
# else
#  ifdef TARGET_ASM_NAMED_SECTION
#   define TARGET_ASM_DESTRUCTOR default_named_section_asm_out_destructor
#  else
#   define TARGET_ASM_DESTRUCTOR default_asm_out_destructor
#  endif
# endif
#endif

#if !defined(TARGET_HAVE_CTORS_DTORS)
# if defined(TARGET_ASM_CONSTRUCTOR) && defined(TARGET_ASM_DESTRUCTOR)
# define TARGET_HAVE_CTORS_DTORS true
# endif
#endif
```

So there seem to be 5 main methods for doing the global constructors:
1. The preferred method seems to be COLLECT2.
If COLLECT2 is not available, then the target is expected to define its own implementation-specific backend for `TARGET_ASM_CONSTRUCTOR`. If the target hasn't defined its own backend for `TARGET_ASM_CONSTRUCTOR`, then GCC has its own internal generic methods. They are, in order of GCC's preference:
2. `default_ctor_section_asm_out_constructor`. According to Cursor, this is supposed to be auto-executed by the dynamic linker on program or library load.
3. This is SysVr4 styled (so says Cursor): `default_named_section_asm_out_constructor`, which places function pointers into a .init_array/.fini_array section. This is supposed to be auto-executed by the dynamic linker on program or library load.
4. And finally `default_asm_out_constructor` as the final fallthrough method.

Below I've listed the options supported for the ELF format, on selected architectures, ignoring whether or not COLLECT2 is available on those arch+ELF combinations:
  * On Aarch64-ELF: There's support for `default_ctor_section_asm_out_constructor` because `CTORS_SECTION_ASM_OP` is defined [Ref2].
  * On Ia32: Couldn't trivially deterine, but it's likely that IA32 uses the #defines in sysv4.h, which means it supports `default_ctor_section_asm_out_constructor`.
5. Then there is the .init_array/.fini_array which is "more modern", according to Cursor AI. The function that implements this is `default_elf_init_array_asm_out_constructor`. (Which ABI is this conformant to?)


A related macro, `TARGET_HAVE_CTORS_DTORS`, is defined based solely on the results of the calculations in Ref0. See how it's defined in Ref1.

Ref1: From gcc source file: `/gcc/target-def.h`:
```cpp
#if !defined(TARGET_HAVE_CTORS_DTORS)
# if defined(TARGET_ASM_CONSTRUCTOR) && defined(TARGET_ASM_DESTRUCTOR)
# define TARGET_HAVE_CTORS_DTORS true
# endif
#endif
```

Ref2: From file `gcc/config/aarch64/aarch64-elf.h`:
```cpp
#define CTORS_SECTION_ASM_OP "\t.section\t.init_array,\"aw\",%init_array"
#define DTORS_SECTION_ASM_OP "\t.section\t.fini_array,\"aw\",%fini_array"
```

Ref3: From file `gcc/config/aarch64/aarch64-elf.h`:
```cpp
#undef INIT_SECTION_ASM_OP
#undef FINI_SECTION_ASM_OP
#define INIT_ARRAY_SECTION_ASM_OP CTORS_SECTION_ASM_OP
#define FINI_ARRAY_SECTION_ASM_OP DTORS_SECTION_ASM_OP
```

Ref4: from file `gcc/config/aarch64/aarch64-elf.h`:
```cpp
/* Since we use .init_array/.fini_array we don't need the markers at
   the start and end of the ctors/dtors arrays.  */
#define CTOR_LIST_BEGIN asm (CTORS_SECTION_ASM_OP)
#define CTOR_LIST_END		/* empty */
#define DTOR_LIST_BEGIN asm (DTORS_SECTION_ASM_OP)
#define DTOR_LIST_END		/* empty */
```

Ref5: from file `gcc/config/aarch64/aarch64-elf.h`:
```cpp
#undef TARGET_ASM_CONSTRUCTOR
#define TARGET_ASM_CONSTRUCTOR aarch64_elf_asm_constructor

#undef TARGET_ASM_DESTRUCTOR
#define TARGET_ASM_DESTRUCTOR aarch64_elf_asm_destructor
```

## The implementation of `default_asm_out_constructor`:

It's just an error message lol.

Ref9: From file: `/gcc/targhooks.cc`:
```cpp
/* Record an element in the table of global constructors.  SYMBOL is
   a SYMBOL_REF of the function to be called; PRIORITY is a number
   between 0 and MAX_INIT_PRIORITY.  */

void
default_asm_out_constructor (rtx symbol ATTRIBUTE_UNUSED,
			     int priority ATTRIBUTE_UNUSED)
{
  sorry ("global constructors not supported on this target");
}
```

## The implementation of `default_stabs_asm_out_constructor`:

It doesn't exist! It's a zombie function reference!

## The implementation of `default_[named|ctor]_section_asm_out_constructor`:

### Generically in varasm.cc:

By default, default_named_section will try to use .ctors and handle priorities with
priority-name-postfixed sections.
The difference between named_section and ctor_section is that ctor_section only
emits .ctors while named_section will emit both .ctors and .ctors.PRIORITY-postfix
sections.

Ref8: From file `/gcc/varasm.cc`:
```cpp
void
default_named_section_asm_out_constructor (rtx symbol, int priority)
{
  section *sec;

  if (priority != DEFAULT_INIT_PRIORITY)
    sec = get_cdtor_priority_section (priority,
				      /*constructor_p=*/true);
  else
    sec = get_section (".ctors", SECTION_WRITE, NULL);

  assemble_addr_to_section (symbol, sec);
}

#ifdef CTORS_SECTION_ASM_OP
void
default_ctor_section_asm_out_constructor (rtx symbol,
					  int priority ATTRIBUTE_UNUSED)
{
  assemble_addr_to_section (symbol, ctors_section);
}
```

### On all ARM platforms generically by default:

ARM will, by default, put prioritized constructors into .init_array and then
put non-prioritized constructors into the section defined in `ctors_section`:

```cpp
static void
arm_elf_asm_cdtor (rtx symbol, int priority, bool is_ctor)
{
  section *s;

  if (!TARGET_AAPCS_BASED)
    {
      (is_ctor ?
       default_named_section_asm_out_constructor
       : default_named_section_asm_out_destructor) (symbol, priority);
      return;
    }

  /* Put these in the .init_array section, using a special relocation.  */
  if (priority != DEFAULT_INIT_PRIORITY)
    {
      char buf[18];
      sprintf (buf, "%s.%.5u",
	       is_ctor ? ".init_array" : ".fini_array",
	       priority);
      s = get_section (buf, SECTION_WRITE | SECTION_NOTYPE, NULL_TREE);
    }
  else if (is_ctor)
    s = ctors_section;
  else
    s = dtors_section;

  switch_to_section (s);
  assemble_align (POINTER_SIZE);
  fputs ("\t.word\t", asm_out_file);
  output_addr_const (asm_out_file, symbol);
  fputs ("(target1)\n", asm_out_file);
}
```

### On Aarch64:

Aarch64 is interesting because it seems to mix together the use of
both .init_array and .ctors. It puts all priority constructors into
.init_array.PRIORITY-postfix sections, and then it puts non-prioritized
constructors into .ctors:

```cpp
static void
aarch64_elf_asm_constructor (rtx symbol, int priority)
{
  if (priority == DEFAULT_INIT_PRIORITY)
    default_ctor_section_asm_out_constructor (symbol, priority);
  else
    {
      section *s;
      /* While priority is known to be in range [0, 65535], so 18 bytes
         would be enough, the compiler might not know that.  To avoid
         -Wformat-truncation false positive, use a larger size.  */
      char buf[23];
      snprintf (buf, sizeof (buf), ".init_array.%.5u", priority);
      s = get_section (buf, SECTION_WRITE | SECTION_NOTYPE, NULL);
      switch_to_section (s);
      assemble_align (POINTER_SIZE);
      assemble_aligned_integer (POINTER_BYTES, symbol);
    }
}
```

## Correctly calling constructors within a .ctors section:

This code for correctly calling the funcptrs in a .ctors section is pasted below in Ref6. Basically the .init section is always NULL terminated. The first funcptr may be the number of items. Or it may be -1. Or it may be the first valid funcptr.

Ref6: From file `libgcc/gbl-ctors.h`:
```cpp
/* Define a macro with the code which needs to be executed at program
   start-up time.  This macro is used in two places in crtstuff.c (for
   systems which support a .init section) and in one place in libgcc2.c
   (for those system which do *not* support a .init section).  For all
   three places where this code might appear, it must be identical, so
   we define it once here as a macro to avoid various instances getting
   out-of-sync with one another.  */

/* Some systems place the number of pointers
   in the first word of the table.
   On other systems, that word is -1.
   In all cases, the table is null-terminated.
   If the length is not recorded, count up to the null.  */

#ifndef DO_GLOBAL_CTORS_BODY
#define DO_GLOBAL_CTORS_BODY		.				\
do {									\
  __SIZE_TYPE__ nptrs = (__SIZE_TYPE__) __CTOR_LIST__[0];		\
  unsigned i;								\
  if (nptrs == (__SIZE_TYPE__)-1)				        \
    for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; nptrs++);		\
  for (i = nptrs; i >= 1; i--)						\
    __CTOR_LIST__[i] ();						\
} while (0)
#endif
```

## Correctly calling constructors in a .init/.fini section:

Basically it does some extra setup and then just invokes the macro that was provided by `libgcc/gbl-ctors.h`; but it also adds `__do_global_dtors` to the atexit function list.

Ref7: From file `libgcc/crtstuff.c`:
```cpp
/* A routine to invoke all of the global constructors upon entry to the
   program.  We put this into the .init section (for systems that have
   such a thing) so that we can properly perform the construction of
   file-scope static-storage C++ objects within shared libraries.  */

static void __attribute__((used))
__do_global_ctors_aux (void)	/* prologue goes in .init section */
{
  FORCE_CODE_SECTION_ALIGN	/* explicit align before switch to .text */
  asm (__LIBGCC_TEXT_SECTION_ASM_OP__);	/* don't put epilogue and body in .init */
  DO_GLOBAL_CTORS_BODY; /* Notice that it just calls the code in libgcc/gbl-ctors.h */
  atexit (__do_global_dtors);
}
```

## Correctly calling constructors in a .init_array/.fini_array section:

This is done "somehow" by the dynamic linker. I assume that's literally Linux's ld-linux.so.

## Final questions:

1. It's important to figure out if there's a way to harmoniously do this where we don't write any new code, but rather we just properly invoke the compiler's own mechanisms to link in the right code into our kernel binary, and then we just invoke that code. Idk whether that requires us to pass specific flags (-nostdlib, -nostartfiles, -nodefaultlibs are prolly related here?). What's the proper, compiler-harmonious way to do things?
2. How can we, if necessary, write a portable implementation of crt*.[S/c]?
3. Why does i586-elf demand a crt0.o, yet AFAICT the correct files should be crti, crtbegin, crtend, crtn? How can I know in advance which crt*.o files a particular xcompiler will demand?
4. None of these methods we've explored so far explain why my i586-elf xcompiler worked by simply emitting the code into 3 sections: .init.prologue, .init, .init.epilogue. Where is this behaviour specified?

# References:

* Useful article: https://maskray.me/blog/2021-11-07-init-ctors-init-array

