# CPU Cache-Friendliness Audit

This is a static assessment of K's data layout and access patterns, not a
hardware-counter profile. Cache behavior is workload-dependent, so the ratings
below are intended to identify likely strengths and scaling limits rather than
provide measured miss rates.

Source references use `filename:symbol` rather than line numbers so that they
remain useful as the files evolve.

## Summary

K's main read, search, and display paths are fairly cache-friendly. The doubly
linked lists are inherently unfriendly to traversal, but most of them are too
small or too cold to be important. The more consequential scaling risks are:

1. edited-line payload fragmentation;
2. whole-line rebuilding and allocation during ordinary typing;
3. shifting the flat line table during structural edits; and
4. the potentially very large linked list of search highlights.

| Workload | Assessment | Main reason |
|---|---:|---|
| Reading/searching an unedited file | Very good | Contiguous metadata and contiguous file image |
| Screen redraw | Good | Dirty-line bitset and linear row processing |
| Normal typing on short lines | Acceptable | Extra copying/allocation, but a small working set |
| Typing on extremely long lines | Poor | Rebuilds and reallocates the whole line |
| Newline insertion near the top of a huge file | Poor | Moves the trailing line-metadata array |
| Large search-result highlight sets | Poor | One separately allocated linked node per highlight |

## Core text representation: strong

The strongest part of the design is the representation of file content.
`LineInfo` records are held in the contiguous `FBUF::d_paLineInfo` array, while
the original file is read into one `FBUF::d_pOrigFileImage` buffer. Unmodified
`LineInfo` records borrow slices of that image rather than owning separate line
allocations.

Relevant symbols:

- `ed_core.h:LineInfo`
- `ed_core.h:FBUF`
- `fbuf_edit.cpp:FBUF::ReadDiskFileFailed`
- `fbuf_edit.cpp:FBUF::LineInfoReserve`
- `ed_core.h:FBUF::PeekRawLine`

In the inspected 64-bit build, `LineInfo` is 16 bytes, so four records fit in a
conventional 64-byte cache line. A sequential scan of an unedited file follows
two predictable streams:

- the contiguous `LineInfo` array; and
- nearly contiguous bytes in the original file image.

The metadata-to-content pointer load is dependent, but after the first line the
content addresses advance through the same file allocation. Hardware
prefetchers should handle this well.

Search code reinforces this advantage by operating directly on referenced line
content instead of copying every line into a temporary buffer.

Relevant symbols:

- `search.cpp:FileSearcherFast::VFindMatches_`
- `search.cpp:FileSearcher::VFindMatches_`
- `search.cpp:strnstr`
- `search.cpp:strnstri`

## Doubly linked lists: locally unfriendly, usually harmless

Traversal of any linked list is a chain of dependent pointer loads. The CPU
cannot reliably prefetch a node until the preceding node supplies its address,
and separately allocated nodes may be scattered throughout the heap. A doubly
linked node also consumes one more pointer than a singly linked node.

K's `DLink` implementation nevertheless has an important advantage over
`std::list`: it is intrusive. The link fields live in the object, avoiding a
separate wrapper node and its additional object-pointer indirection.

Relevant symbols:

- `dlink.h:DLinkHead`
- `dlink.h:DLinkEntry`
- `dlink.h:DLINKC_FIRST_TO_LASTA`
- `dlink.h:DLINKC_LAST_TO_FIRST`

The current object layouts are also reasonably favorable:

- `View` is 192 bytes, with its two list links followed immediately by its
  `Win` and `FBUF` pointers. The traversal link and commonly needed ownership
  pointers are therefore in the first cache line.
- `FBUF` is 280 bytes, with its filename metadata and global-list link in its
  first cache line.
- `NamedPoint` is 32 bytes; its link, point, and name pointer form a compact
  record, although the name itself is another allocation.
- `HiLiteRec` is 40 bytes before allocator overhead. A heap allocation of this
  size will commonly occupy approximately one allocator/cache-line-sized chunk.

Most DLink lists are normally small:

- views belonging to a window or FBUF;
- display add-ins;
- named marks;
- regex replacement pieces;
- filename-substitution pieces;
- performance counters; and
- process/job queues.

For these uses, constant-time insertion and removal are likely worth more than
improved traversal locality. Replacing them wholesale with `std::list` would be
a regression, not an optimization.

### Large exception: highlight records

`ViewHiLites` can create one separately allocated `HiLiteRec` per search match.
It maintains a contiguous speed table to find a starting node, but display then
continues by chasing the linked list.

Relevant symbols:

- `display.cpp:HiLiteRec`
- `display.cpp:ViewHiLites`
- `display.cpp:ViewHiLites::vhInsHiLiteBox`
- `display.cpp:ViewHiLites::FirstHiLiteAtOrAfter`
- `display.cpp:ViewHiLites::InsertHiLitesOfLineSeg`

A search-all operation producing hundreds of thousands of highlights is the
most likely place for `DLink` traversal, allocator overhead, and heap
fragmentation to become material. This is the first DLink user worth changing
or pool-allocating.

### Undo records have the same locality problem

The undo subsystem implements its own doubly linked chain of separately
allocated polymorphic `EditRec` objects. It is not based on `DLink`, but has the
same traversal behavior.

Relevant symbols:

- `fbuf_undo.cpp:EditRec`
- `fbuf_undo.cpp:FBUF::Undo_AddNewEditOpToListHead`
- `fbuf_undo.cpp:FBUF::Undo_RmvOneEdOp_fNextIsBoundary`
- `fbuf_undo.cpp:FBUF::Undo_UserStep_UndoOrRedo`
- `ed_vars.h:g_iMaxUndo`

Normal consecutive character entry on one line is coalesced, so this list is
not traversed for every character. Deep undo/redo, old-record cleanup, and undo
diagnostics are cache-unfriendly, however. The default allows 100,000 major undo
steps, with potentially multiple records per step, so its retained footprint can
also displace more useful data from the cache.

## Character editing: sequential but allocation-heavy

Ordinary character insertion performs roughly this sequence:

1. expand/copy the existing line into a temporary `std::string`;
2. open an insertion hole and write the character;
3. optionally entab the result;
4. allocate new exact-sized owned storage for the line; and
5. copy the complete result into that allocation.

Relevant symbols:

- `fbuf_edit.cpp:FBOP::PutChar_`
- `fbuf_edit.cpp:FBUF::DupLineForInsert`
- `fbuf_edit.cpp:FBUF::PutLineEntab`
- `fbuf_edit.cpp:FBUF::PutLineRaw`
- `fbuf_edit.cpp:FBUF::UndoIns_EditLine`
- `fbuf_edit.cpp:LineInfo::PutContent`

These are mostly linear memory passes, so the access pattern is cache-friendly
in the narrow sense. The implementation nevertheless creates allocator churn
and more memory traffic than the logical one-character edit requires. For
ordinary short source lines the data stays in L1 and the cost is likely small.
For long minified lines, generated data, or logs, each keystroke becomes
O(line length) and can touch far more data than fits in cache.

Once a line is modified, it owns a separate allocation rather than borrowing
from the original file image. The metadata array remains contiguous, but a
whole-file scan containing many modified lines must jump among their heap
allocations. Cache locality therefore gradually degrades as a buffer becomes
heavily edited.

## Structural line edits: bandwidth-friendly but algorithmically expensive

Line insertion and deletion retain the flat `LineInfo` array and use `memmove`
to shift the trailing records.

Relevant symbols:

- `fbuf_edit.cpp:FBUF::InsertLines__`
- `fbuf_edit.cpp:FBUF::DeleteLines__`
- `ed_mem.h:MoveArray_`

This is a very cache-friendly streaming operation, but it is O(number of lines
after the edit). With 16-byte `LineInfo` records, inserting one newline near the
start of a one-million-line buffer moves about 16 MB of metadata. Repeated
structural edits can therefore become memory-bandwidth-bound even though their
cache access pattern is optimal for the chosen representation.

The flat table is an excellent tradeoff for random line lookup, rendering, and
search. A chunked table, B-tree, rope, or piece-table-like structure would
improve structural edits at the cost of more complicated lookup and less
predictable scanning. Such a change is justified only if huge-file editing is a
measured workload bottleneck.

## Display path: generally good

Dirty screen rows are represented by a compact contiguous `BitVector`. Redraw
tests each screen row, rebuilds only marked rows, and performs linear writes to
a reused string buffer and a fixed color array.

Relevant symbols:

- `BitVector.h:BitVector`
- `BitVector.h:BitVector::IsAnyBitSet`
- `display.cpp:RedrawScreen`
- `display.cpp:LineColorvals`
- `display.cpp:View::GetLineForDisplay`
- `display.cpp:conVidWrStrColors`
- `win32_conout.cpp:TConsoleOutputControl`

`LineColorvals` is 514 bytes and is filled for each dirty row. That means roughly
eight conventional cache lines of color data are written even for a narrow
terminal. This is contiguous and predictable; terminal/ncurses calls are likely
more expensive. It is a possible memory-traffic cleanup, but not a likely first
order bottleneck.

The Windows output path stores screen cells and per-line control records in
vectors, which is also favorable for locality.

### Long-line horizontal-scroll pathology

Display formatting begins at the start of a source line even when the visible
segment begins far to the right, because tab expansion requires knowing the
current display column.

Relevant symbols:

- `fbuf_edit.cpp:PrettifyWriter`
- `fbuf_edit.cpp:PrettifyMemcpy`
- `display.cpp:View::GetLineForDisplay`

This is a sequential, cache-friendly scan but can be O(horizontal offset) for
every redraw. It is another case where algorithmic work, rather than random
cache misses, is the concern.

## Other subsystems

### Command dispatch

Keyboard command dispatch is a direct indexed load from a contiguous pointer
table, not a tree or linked-list lookup.

Relevant symbols:

- `cmdidx.cpp:s_Key2CmdTbl`
- `cmdidx.cpp:CmdFromKbdForExec`

Name-based command lookup uses a red-black tree, but occurs for macros,
configuration, and interactive command-name execution rather than for every
ordinary keypress. Its pointer-heavy layout is therefore unlikely to affect the
normal typing path.

### FBUF registry

The global FBUF registry is currently a linear DLink list when `FBUF_TREE` is
disabled. Name lookup traverses separately allocated, fairly large `FBUF`
objects and compares filename strings.

Relevant symbols:

- `ed_core.h:FBUF_TREE`
- `fbuf.cpp:FindFBufByName`
- `fbuf.cpp:FBUF::AddFBuf`

This is reasonable for dozens of open buffers and may become noticeable with
thousands. The compiled-out red-black-tree alternative reduces comparisons but
is still pointer-heavy. If large buffer sets matter, a hash index alongside the
existing ordered list would likely be a better targeted improvement.

### Instruction cache

The program contains a large command and feature set, so the complete text
section cannot reside in instruction cache. That fact alone is not concerning:
the normal keyboard/edit/redraw loop uses a much smaller hot subset. The release
configuration uses LTO, `-O3`, and `-march=native` (`GNUmakefile:GCC_OPTZ`),
which should allow aggressive inlining and dead-code optimization. Profile-guided
code layout would be more relevant than changing the small DLink macros if
instruction-cache misses are eventually measured.

## Recommended optimization order

1. **Measure representative workloads first.** Separate opening/searching an
   unedited huge file, typing on short and extremely long lines, repeated
   newline insertion near the beginning of a huge file, and search-all with a
   very large match set.
2. **Change or pool-allocate `ViewHiLites` records.** A reserved vector of
   compact records plus speed-table indices is attractive because the heavy
   search-all case usually adds highlights in increasing order. A slab allocator
   is a lower-risk alternative that preserves stable nodes and list behavior.
3. **Use a mutable representation for the actively edited line.** A line-local
   gap buffer or retained scratch string, committed at an undo boundary or when
   leaving the line, could eliminate most per-character full-line allocations.
4. **Address the flat line table only if huge structural edits are measured as
   a problem.** A chunked vector or tree would trade away some of the current
   representation's excellent scan and lookup behavior.
5. **Add an FBUF name index only for genuinely large buffer sets.** Preserve the
   existing list for ordering/MRU semantics and use a hash table solely for
   lookup.
6. **Do not replace DLink globally.** Most uses are appropriately small, and a
   conventional non-intrusive linked list would add overhead without fixing the
   underlying pointer-chasing behavior.

## Bottom line

The application is more cache-friendly than unfriendly. Its central flat line
table and borrowed slices into a contiguous file image are strong architectural
choices for reading, rendering, and searching. DLink traversal is a localized
problem—most notably for large highlight sets—not the dominant systemic issue.
For interactive editing, whole-line rebuilding and heap allocation are more
likely to matter; for huge-file structural editing, shifting the line table is
the primary scaling limit.
