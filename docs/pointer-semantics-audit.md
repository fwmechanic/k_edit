# Pointer-semantics audit

## Scope and constraints

This audit covers the editor's first-party C++ sources at branch point
`de491d1`. The bundled Lua 5.1 sources are third-party C and are out of scope.
The local compiler is GCC 13.3, and `GNUmakefile` selects C++23.

The conversion rules are deliberately conservative:

- A sole owner becomes `std::unique_ptr`.
- Storage obtained from the `malloc` family becomes `malloc_ptr`, a
  `std::unique_ptr` with a stateless `free` deleter.
- A contiguous non-owning range becomes `span` (the imported `std::span`) or
  the existing `stref` (`std::string_view`).
- Nullable, reseatable observers remain raw pointers. C++23 has no standard
  `observer_ptr`, and wrapping an observer would not improve its lifetime
  safety.
- Intrusive links and pointers required by C or operating-system APIs remain
  raw at those boundaries.
- `std::shared_ptr` is not introduced: none of the converted sites needs shared
  ownership, and its control block, atomics, and possible extra allocation
  violate the efficiency constraint.

## Converted opportunities

### C-family allocations

- `BitVector` now owns its zeroed allocation with pointer-sized
  `malloc_ptr<T[]>`. This also fixes the pre-existing undefined behavior of
  allocating with `calloc` and destroying with scalar `delete`.
- `TxtFileLineReader` keeps its growable `malloc`/`realloc` buffer in
  `malloc_ptr<char[]>`. Reallocation preserves the old owner on failure.
- `/proc/self/exe` read-link storage, mark names, diceable strings, undo
  `LineInfo` arrays, and CFX substitution storage now use `malloc_ptr`.
- PCRE code and match-data handles have separate stateless deleters. A failure
  to allocate match data now releases the already-compiled code
  automatically.

`ed_mem.h` has compile-time checks that both scalar and array `malloc_ptr`
specializations remain exactly one machine pointer wide.

### C++ object ownership

- Switch implementation factories, filename-generator factories, and searcher
  factories return `std::unique_ptr`; ownership is explicit at the return
  boundary.
- The global search-specifier owner, its compiled-regex member, CFX nested
  generators, tab-completion directory state, process information, Windows
  title-bar contributors, and Linux shell-job command lists now use
  `std::unique_ptr`.
- Shell-job entry points accept `std::unique_ptr<StringList>`. A rejected job
  start therefore cannot leak its command list.
- Existing `unique_ptr(new T)` and `delete owner.release()` idioms were replaced
  with `make_unique` and `reset`.
- Follow-up work gave each `FBUF` sole ownership of its internal shell-job
  executor. The worker is joinable, completed executors are reused without
  another allocation, and `FBUF` destruction stops and joins the worker before
  releasing line storage.
- `View` highlighting, Windows console-font arrays, the Windows screen
  controller and its screen-buffer handle, and transient screen-copy storage
  now have automatic ownership.
- Locally owned C `FILE` and pipe streams use pointer-sized unique owners while
  preserving raw `FILE *` at C API boundaries.

### Explicit mixed storage

- `LineInfo` now records whether its data is empty, borrowed from the original
  file image, or separately owned. Cleanup no longer infers ownership by
  relationally comparing unrelated pointers.

### Bounded non-owning views

- Switch enum metadata is stored as `span<const enum_nm>`.
- Switch display callbacks take `span<char>` instead of independent
  destination and byte-count parameters.
- Internal C++ call sites construct bounded views explicitly with
  `span{array}`. `BSOB(array)` retains its original pointer-and-byte-count
  expansion for C and OS APIs.
- Pseudofile name tables and the output array used to select interesting files
  are passed as `std::span`.
- Regex replacement literal segments use `stref` instead of a character
  pointer plus length.

These views occupy the same state as the pointer-and-extent pairs they replace
and do not allocate.

## Raw pointers intentionally retained

- `DLinkEntry`, `DLinkHead`, red-black-tree nodes, undo links, `FBUF`, `View`,
  and `Win` relationships are intrusive graph links or observers. Their
  lifetime is governed by container membership and editor-level protocols, not
  by the individual pointer field.
- Lua callbacks, allocator callbacks, PCRE call arguments, C stdio handles,
  Windows handles, ncurses values, and polymorphic command-function pointers
  must preserve the external ABI.
- Raw C/OS file, time, PCRE, hostname, and Win32 calls still receive separate
  pointer and byte-count arguments through `BSOB`. Internal C++ buffer
  interfaces use `span<std::byte>` or `span<char>`.

## Allocation and representation audit

The changes add no allocation sites:

- Every `make_unique` replaces an existing `new` of the same object.
- Every `malloc_ptr` adopts an existing `malloc`, `calloc`, `realloc`, or
  `Strdup` allocation.
- Stateless deleters are checked to use empty-base optimization and retain
  one-pointer representation.
- `span` and `stref` are non-owning views and never allocate.
- No `shared_ptr`, control block, container layer, or copied backing buffer was
  introduced.

The relevant verification commands are:

```sh
make -j2 k
make -j2 run_unittests
make sanitize
```

On the same `./k -?` startup/help path, Valgrind reports 150 total heap
allocations for both the original baseline and the current follow-up. The
current code increases completed frees from 121 to 124 and reports zero
definite, indirect, or possible leaks and zero memory errors.

The optimized (`DBG_BUILD=`) configuration has a pre-existing
`-Werror=unused-result` failure in `kitty_conin.cpp` on both `master` and this
branch. With the same temporary diagnostic override, both optimized binaries
link; the pointer changes introduce no additional heap allocations.

The Windows-only edits also require a MinGW build before merging on that
platform; no MinGW cross-compiler is installed in the audited environment.
