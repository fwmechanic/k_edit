## WUC = "Word Under Cursor" display highlighting

This is a K facility that (unbidden, as part of the display (highlight)
subsystem) highlights _other_ instances of the WUC.

WUC-determination is done using the `wordchars` switch setting (plus evolutions; see below).

This feature _does not highlight_ the actual WUC; I find this too distracting
(highlighting gyrations atop the cursor make it hard to follow the cursor
itself).  Highlight changes _away from the cursor_ are less distracting (and are
the _entire_ point of WUC highlighting!).

The WUC is used (by default) for tag lookups (NOARG `tags`).  This of course has
some interesting/unforeseen implications (the effect of "simple changes" is
seldom simple)...

This feature started out simple, and evolved to become more complex.  The
evolution is ongoing...

### Evolution: variant-WUC highlighting

In my career, I encountered identifiers which were semantically identical, but
syntactically different.  Examples:

1. __bash__ variables: assigned with `var=val`, referenced (read) with `$var` or `${var}`.

2. __make__ variables; similar to __bash__; assigned with `VAR=val`, referenced with `$(VAR)`

3. Hexadecimal values: some tools inconsistently provide or omit "0x" prefix (in the same file) on hex values.

Variant-WUC highlighting will highlight-match any variant.  These capabilities
are hard-coded in K's C++ code ; see `HiliteAddin_WordUnderCursor::SetNewWuc()`
(and are sufficiently non-intersecting or benign that they are enabled always.

### Evolution: class/struct name hierarchial-join

Experimental feature: as I get more involved in comprehending Python code which tends to be coded as

```python
def __init__(self, deduct)
   self.deduct = deduct
```

in such code, existing K WUC highlighting will not distinguish between
`self.deduct` and `deduct`; all instances will be highlighted.  There is more
value in highlighting only instances of `self.deduct` or only instances of
`deduct` (the coincidence of variable and member names cannot be relied upon the
confer an actual relationship (worth highlighting) between the two).  To
accomplish this is a (mostly-)language-independent manner, a switch `hljoinchars`
was created, and when looking for the left end of the WUC, the set of chars from
the union of hljoinchars and wordchars is used to define the set chars that
comprise the WUC.

Implementation defect: some languages use a multicharacter "hierarchial-join
phrase" (`->`) which does not align correctly with a simpleminded character-set
implementation. `hljoinchars: .->` might be set for such languages, and would
see all of `yourval-myval` as a WUC when a char of `myval` is under the cursor
(instead of `myval` alone).  So far this has not bit me, as in most code I would
write `yourval - myval` (spaces surrounding `-`).  But for code I didn't write,
this will likely be a problem (and demand switching the WUC parsing
implementation `GetWordUnderPoint()` from charset-based to regex-based).

More heuristic gaming.  I'm finding that `self.deduct` is an interesting WUC
whereas `self.deduct()` is less so (especially if `deduct()` is a mutator).
Meaning I might be more interested in _all_ occurrences of `deduct()` being
called rather than only calls to `deduct()` on `self`.  But there isn't a clear
winner here; it's very situational.

The interesting part of the implementation is that `GetWordUnderPoint()`
searches forward __and backward__ from the cursor location.  I'm unsure whether
searching backward is something Regex supports (my gut feel is "no").  I think I
would have to find all matches on a line, and then find if the cursor falls
within any of them.  This seems to not scale well for very long lines, unless the
result for the line could be cached until the cursor line changes (by the cursor
changing lines or the cursor line content changing).
