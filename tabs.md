# Introduction

K is based on Microsoft's M.  M (and its child, PWB) offered a number of
switches to control tab-handling, all of which are present (though some with
different names) in K.  However K's internal implementation of tab-handling is
imperfect, and that's at least partially because the implementation ramifications
of these switches has not been clearly documented.  The goal of this document is
to close this gap.  Further, as a stretch goal, I'd really like to reduce the
number of tab-handling switches while delivering an intuitive conceptual model
that meets the spectrum of user needs.

NOTE: this issue has festered primarily because I personally militantly eschew
all possible uses of HTABs in favor of spaces.  Only since I started owning build
aspects of projects which (by my choice) use GNU Make, have I been unable to
entirely avoid (editing) text files containing tabs.  And now (late 2014) my "day
job" comes close to being a "full-time Linux system operator", which takes me
into a realm where a sizeable percentage of actors (at least: people providing,
at great distance, the FOSS which I am using) hold a position which is in polar
opposition to mine ("tabs are wonderful and should be used whenever possible!");
not only that, but HTABs are often (as in GNU make makefiles) used as
_syntax-elements_(!!!).  Thus I must confront this festering issue ASAP.

NOTE: Below I excerpt the PWB 2.0 manual, which is part of the MASM 6.x
documentation which is readily available on the internet in .DOC and .PDF format.
In order to use terminology consistent with the K source code (and avoid charges
of racism), I have changed references to "white space" to "blanks".

# Terminology

 * tab (a.k.a. horizontal tab or HTAB) character: an ASCII (or Unicode)
   character having decimal value 9.  Any character immediately following an
   HTAB will be (for print/display purposes) horizontally positioned at the next
   tabstop.  IOW any character immediately following an HTAB will
   printed/displayed at the column of the next tabstop.  Therefore the occurrence
   of a HTAB will, depending on its columnar position, necessitate the insertion
   of [1..tabwidth) "tabspring" characters.

   [Wikipedia](https://en.wikipedia.org/wiki/Tab_key#Tab_characters) covers
   this ground well:

      "...fixed tab stops, de facto standardized at every multiple of 8
      characters horizontally...  Tab characters simply became a form of data
      compression."

 * tabstop: the set of horizontal/columnar positions which are evenly divisible
   by the tabwidth value (traditionally=8), + 1.  The first tabstop is (at)
   column 9.

 * tabspring character: a term of my own invention to describe the "virtual"
   (not present in source content) characters occupying display column(s) (if
   any) between (to the immediate right of) a HTAB and the next tabstop of a
   displayed/printed representation of the source content.  Tabspring characters
   are not present in the HTAB-containing content, but are added to a depiction
   of that content, as a consequence of the presence of HTAB characters.

   In the vast majority of text display applications (text viewers (e.g.  less,
   more) and editors (e.g. nano, notepad), both HTAB and tabspring characters
   are depicted as space (ASCII 8) characters, however because space characters
   are also used to depict both actual space characters as well as columns past
   the right end of a line's content (as well as HTAB and tabspring characters),
   such depiction cause great loss of information, and that loss can sometimes
   cause problems.

   Therefore M/PWB/K offer a user-alterable switch (tabdisp) that specifies the
   ASCII code of the character used to depict HTAB and tabspring characters.  K
   uniquely (on Microsoft platforms) takes this another step: it forces use of
   [Code Page 437](https://en.wikipedia.org/wiki/Code_page_437)

      "...the character set of the original IBM PC (personal computer), or
      MS-DOS." "...this character set was not conceived as a code page; it was
      simply the graphical glyph repertoire available in the original IBM PC.
      This character set remains the primary font in the core of any EGA and
      VGA-compatible graphics card.  Text shown when a PC reboots, before any
      other font can be loaded from a storage medium, typically is rendered with
      this "Code Page".

   iff tabdisp:249 (which K source code calls BIG_BULLET character), HTAB is
   displayed as BIG_BULLET and tabspring chars are displayed as SMALL_BULLET
   (decimal value 250).  This makes the presence and display effect of HTAB
   characters very clear.

   Further, M/PWB/K offer another switch, traildisp, which is used to depict
   spaces which follow the last non-space character on a line.  traildisp:249 is
   a common setting.

   Aside: when viewing files containing all-HTAB indenting, and/or many trailing
   spaces, the tabdisp/traildisp:249 setting can be annoying.  Therefore K
   contains some code which disables these switches under many conditions (EX:
   when viewing non-dirty files; as soon as a buffer is modified,
   tabdisp/traildisp:249 is set so the user is confronted by HTABs and trailin
   spaces in their full glory).


# M/PWB/K Tab-handling switches

NB: In the interest of better user comprehension I have renamed a large percentage of these in K.

 * filetab (K:tabwidth): [1..8] distance between tabstops; tabwidth=8 is "traditional" (and default)
 * entab   (K:tabconv):  enum [0..2/3] controlling how spaces in a modified line being written to an editor buffer are converted to tabs (or not).
 * tabalign: controls whether cursor is allowed to be positioned into display-locations occupied by tabspring characters.
 * realtabs: a mode control switch which controls when other switches take effect.

 Note: PWB 2.0 added a "tabstops" switch (offering arbitrarily-located tabstops!!!) which will never be supported by K.

## tabwidth: (MS: filetab) switch

Default value: 8

From the PWB 2.0 manual:

   "The Filetab switch determines the width of a tab field for displaying tab
   (ASCII 9) characters in the file.  The width of a tab field determines how
   blanks are translated when the Realtabs switch is set to no.  The Filetab
   switch does not affect the cursor-movement functions `tab` and `backtab`."

## tabconv: (MS: entab) switch

Default value: 8

From the PWB 2.0 manual:

   "The entab switch controls how PWB converts blanks on modified lines.
   PWB converts blanks only on the lines that you modify.

   "When the Realtabs switch is set to yes, tab characters are converted.  When
   set to no, tab characters are not converted.

   "The entab switch can have the following values:

   "0: Convert all blanks to space (ASCII 32) characters.

   "1: Convert blanks outside quoted strings to tabs.

      A quoted string is any span of characters enclosed by a pair of single
      quotation marks or a pair of double quotation marks.  PWB does not
      recognize escape sequences because they are language-specific.

      For well-behaved conversions with this setting, make sure that you use a
      numeric escape sequence to encode quotation marks in strings or character
      literals.

   "2  Convert blanks to tabs.

   "With settings 1 and 2, if the blanks being considered for conversion to a
   tab character occupies an entire tab field or ends at the boundary of a tab field,
   it is converted to a tab (ASCII 9) character. The width of a tab field is specified
   by the Filetab switch."

K modifies this by inserting a new meaning for tabconv value 1: "convert leading
blanks to tabs"; M/PWB entab values [1..2] become tabconv values [2..3] in K.

## tabalign switch

Default value: no

From the PWB 2.0 manual:

   The Tabalign switch determines the positioning of the cursor when it enters a
   tab field.  A tab field is the area of the screen representing a tab character
   (ASCII 9) in the file.  The width of a tab field is specified by the Filetab
   switch.

   The Tabalign switch takes effect only when the Realtabs switch is set to yes.

   yes
      PWB aligns the cursor to the beginning of the tab field when the cursor
      enters the tab field.  The cursor is placed on the actual tab character in
      the file.

   no
      PWB does not align the cursor within the tab field.  You can place the
      cursor on any column in the tab field.  When you type a character at this
      position, PWB inserts enough leading blanks to ensure that the character
      appears in the same column.

## realtabs switch

Default value: 8

From the PWB 2.0 manual:

   The Realtabs switch determines if PWB preserves tab (ASCII 9) characters or
   translates blanks according to the Entab switch when a line is modified.
   Realtabs also determines if the Tabalign switch is in effect.

   yes
      Preserve tab characters when editing a line.
   no
      Translate tab characters when editing a line.

# Tab-handling in K

OK with that delightful mountain of minutiae absorbed, time to discuss how this all fits together.

tabalign: no further discussion needed (and in fact, the K source-code ramifications of tabalign are miniscule).

tabdisp, traildisp: these are only used when generating "display strings".  This
mostly means: strings used in screen redraw, but this can include some unlikely
things like strings being edited by the user on the dialog-line.  Like tabalign,
the effects of these switches is well encapsulated.

realtabs, tabconv, tabwidth: tabwidth is a parameter to tabconv and tabdisp,
and, while frequently referenced in K source code, its involvement in the
following policy discussion is implicit.

## Reading buffer lines

K buffers (FBUFs) contain copies of file content, arranges as lines of text.
This (as read from filesystem) content is stored in the FBUF as read from the
filesystem; it there are HTABs in this source file, these (and all other source
file content) are present in FBUF (line) content.

All low-level FBUF access to buffer content is done on a per-line basis.

   Aside: these low-level FBUF line-reading methods all hide the actual content
   of end-of-line (EoL) character sequences: a line consists of all characters
   from the previous EoL to the next EoL.  Therefore all editing code is ignorant
   of (and therefore generic with respect to) the actual EoL's found in the
   source file.  Only code that (a) parses source file content (in order to
   create the LineInfo array index) and (b) writes FBUF content to filesystem
   files, pays any attention to EoLs.

In the interest of efficiency (avoidance of unnecessary heap-allocations and
related copying of line content), FBUF::PeekRawLine*() methods are offered which
return stref's to the actual (const) FBUF line content of a given line and should
be used whenver possible due to their zero-copy (and no-heap-alloc) nature.

 * FBUF::PeekRawLine() returns the entire line.

 * FBUF::PeekRawLineSeg() returns a stref to the substring of the line enclosed by a pair of columns (x-ordinates).

Perhaps the entire dilemma around tab-handling can be captured by considering
what FBUF::PeekRawLineSeg() returns (rls) (as of 20150103_111809) when
xMinIncl==column of a tabspring char; answer: rls[0] == HTAB.

Now, this is (by some interpretations) obviously incorrect since there is not an
HTAB at that column location.  Nor is their a space.  There is actually NOTHING
in the source string at the column!  So an argument could be made that this HTAB
character should be removed from rls, that rls[0] should reference the character
in the source string AFTER the HTAB.

But the user might object.  Because (for example, when executing the new
"rawline" function) what the user selects (BOXARG, STREAMARG) is the _displayed_
representation of the source string, not the source string itself: they may have
included the tabspring in their selection, intending to encompass leading
blank(s).  And, depending on the tabdisp setting, they might not have the
slightest inkling that what they were selecting was not spaces, but tabsprings!
When the subsequently invoked function acts as if no leading blanks were present
in the arg provided, well, that's cause for user consternation (which is
obviously NOT our goal!).

So, you might say: "hey, just set 'tabalign:yes'!" That way, the user CANNOT
move the cursor into a tabspring region, so they cannot select a tabspring
(without also selecting the parent HTAB).  Not quite: what if a multiline BOXARG
selection is formed over a line-range whose top & bottom lines contain spaces,
and whose middle lines contain tabs(prings), in the horizontal selection boundary
zone (sides of the BOXARG selection).  There are ways the user could move the
cursor to avoid it passing thru any tabspring regions (which would cause the
cursor to be evicted from the tabspring region to the left to the HTAB which
created the tabspring region), leading to the selection box edges (on some lines)
passing thru a tabspring region.  What to do?

How about: when selecting, transiently force tabdisp:BIG_BULLET and
traildisp:SMALL_BULLET?  That's a good idea (removes the user's blinders), except
(as it stands today) that may not work on Linux (where this problem is most
likely to occur; see note near top) because I've not yet figured out (indeed,
haven't even _pondered_) how to obtain the effect of using Code Page 437 on a
Linux host (is it possible without adding support for some form of Unicode?  If
not, then what?) in order to be able to _display_ BIG_BULLET and SMALL_BULLET.

Down the rathole we go...
