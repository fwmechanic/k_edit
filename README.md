K is my personal Win32 programmer's text editor, whose design is derived
from Microsoft's "M" editor which was itself derived from the ["Z"](http://www.texteditors.org/cgi-bin/wiki.pl?Z) [editor](http://www.applios.com/z/z.html).

[TOC]

# Features

 * **Z**: "Reverse-polish" function-execution mode wherein the user creates the function-argument ("arg") using various selection or data-entry modes or argtypes, before the function is invoked; the function's execution behavior adapts to the actual argtype it receives.
 * **Z**: Can switch between line and box (column) selection mode simply by varying the shape of the selection.
 * Copy and run.  No installation necessary.  Can be run from removable storage.
 * Infinite undo/redo.
 * "syntax highlighting" (of comments, literal strings and characters *only*)
 * highlights C/C++ preprocessor conditional regions
 * File/Source-code navigation: K is integrated with [Exuberant Ctags](http://ctags.sourceforge.net/), and can perform multi-file-greps and -replaces targeting sets of files enumerated in any editor buffer.  K supports powerful recursive (tree) directory scanning with output to an editor buffer, so, when combined with file-filtering `function`s such as grep, strip, etc. it's easy to quickly construct a buffer containing only
the names of all of the files of interest to you, and have the multi-file-aware functions reference this buffer.  And since this is based on current filesystem content, it's more likely to be complete and correct than a
"project file" which must be independently maintained (and thus can easily fall out of sync with workspace reality).

# Limitations

 * K is a Win32 Console app with no mouse support (except the "scroll wheel" (or trackpad gestures which mimic scroll-wheel behaviors).  The UI is fairly minimal: there are no "pulldown menus" though primitive "pop-up menus" are used on a per-function basis.
 * K has no "virtual memory" mechanism (as M did); edited files are loaded in
toto into RAM; K WILL CRASH if you attempt to open a file that is larger than
the biggest malloc'able block available to the K process.  I get hit by this
maybe once a year, and it's easy enough to head/tail/grep to chop a huge
(usually some sort of log) file into manageable pieces.  Also, the 
64-bit version raises this ceiling considerably.
 * K operates on ASCII files; is has no support for Unicode/MBCS/etc. content (it's a text-editor, not a word processor).  Lately (very rarely) I get hit with problems related to non-ASCII filenames: when I download music, the names of file or dirs in the received package occasionally contain characters which have to be TRANSLATED
into the charset that K uses.  If I then construct a cmdline to rename said
file/dir (a task I often perform with K), the command will fail because the filename (containing the translated
character instead of the original character) will not exist.  As with the "lack
of VM" limitation, this is rarely annoying, and will only be resolved (a large
undertaking) if it becomes much more annoying to me (which seems unlikely).

# Building

Prerequisite: I use the [nuwen.net distribution](http://nuwen.net/mingw.html) of MinGW. 

The nuwen.net MinGW distro downloads are self-extracting-GUI 7z archives which contain bat files (I use `set_distro_paths.bat` below) which add the appropriate environment variable values sufficient to use gcc from the cmdline.  

I use the following 1-line bat files (stored outside the K repo) to setup MinGW for building K (or any other C/C++ project):

 * `mingw.bat` (x64): `c:\_tools\mingw\64\mingw\set_distro_paths.bat`
 * `mingw32.bat` (i386): `c:\_tools\mingw\32\mingw\set_distro_paths.bat`

To build:

    cd K-repo-root
    mingw.bat   & rem run once to put MinGW exes in shell's PATH
    make clean  & rem unnecessary first time
    make -j

To clean a repo sufficient to switch between 32-bit and 64-bit toolchains:

    make zap    & rem clean plus nuke all lua.exe related

Note that [MinGW gcc non-optionally dyn-links to MSVCRT.DLL](http://mingw-users.1079350.n2.nabble.com/2-Question-on-Mingw-td7578166.html)
which it assumes is already present on any Windows PC.

## Stability notes

The last nuwen.net MinGW release (w/GCC 4.8.1) that builds 32-bit targets is 10.4, released 2013/08/01, and this version is no longer available from the nuwen.net site.  So, while I continue to build K as both 32- and 64- bit .exe's (and can supply a copy of the nuwen.net MinGW 10.4 release upon request), the future of K on the Win32 platform is clearly x64 only. 

The 64-bit build of K is relatively recent (first release 2014/02/09) but it's *mostly* working fine so far (updt: on Win7 (targeting a WQXGA (2560x1600) monitor), I get an assertion failure related to console reads (these never occur with the 32-bit K); also these never occur with the x64 K running in Win 8.x (but targeting HD+ (1600x900) resolution); the only time I use Win7 is at work (I am one of seemingly few people who can look past the "Metro" UI of Win 8.x and find a core OS that is superior to Win7).

# Debug/Development

I use [DebugView](http://technet.microsoft.com/en-us/sysinternals/bb896647.aspx) to capture the output from
the DBG macros which are sprinkled liberally throughout the source code.

Prior to release 11.6, nuwen.net MinGW *purposely* DID NOT include `gdb`; the newest (64-bit-only) MinGW distros now include `gdb`, and I have used it a couple of times.  I generally only use a debugger to debug crashes, so if `gdb` is unavailable I use [DrMinGW](https://github.com/jrfonseca/drmingw) as a minimalist way of obtaining a useful stack-trace when a crash occurs.  It is necessary to build K w/full debug information in order to use either DrMinGW or `gdb`: open GNUmakefile, search for "DBG_BUILD" for instructions on how to modify that file to build K most suitably for DrMinGW and `gdb`).

# Release Fileset

A release is the minimum fileset needed to run the editor

run `krls outputdirname`  (this is currently broken)

1. outputdirname must already exist.  It may be relative to the cwd.
1. the output of this process also includes a self-extracting "installer"
  exe file (all the installer does is self-extract, so calling it an
  "installer" is a major stretch.

# Deployment (a.k.a. "installation")

K was designed to be "copy and run" (a "release") anywhere.  I have
successfully run it from network shares and "thumb drives".

# Persistent Footprint (a.k.a. spoor)

Editor state

 *  files edited (including window/cursor position)
 *  search history
 *  function usage accuulation

is stored in files in `%APPDATA%\Kevins Editor\*`

# Tutorial

## Command line invocation

 * to edit the previously edited file, run `k`
 * to edit file `filename`, run `k filename`
 * run `k -h` to display cmdline invocation help. 

## Essential Functions

The editor implements a large number of functions, all of which the user can invoke. Every key has one function bound to it (and the user is completely free to change these bindings), and functions can also be invoked within macros and via the `execute` function.  Following are some of the most commonly used functions:

 * `exit` (`alt+F4`) exits the editor; the user is prompted to save any dirty files (one by one, or all remaining).
 * `arg` is assigned to `goto` (numeric keypad 5 key with numlock off (the state I always use).  `arg` is used to introduce arguments to other editor functions. `arg` can be invoked multiple times prior to `function`; this can serve to modify the behavior of `function` (EX: `setfile`)
 * `alt+h` opens a buffer named <CMD-SWI-Keys> containing the runtime settings of the editor:
    * switches with current values (and comments regarding effect).
    * functions with current key assignment (and comments regarding effect).
    * macros with current definition
 * `setfile` (`F2`) function is very powerful:
    * `setfile` (w/o `arg`) switches between two most recently viewed files/buffers.
    * `arg setfile` opens the "openable thing" (see below) whose name starts at the cursor.
    * `arg arg setfile` saves the current buffer (if dirty) to its corresponding disk file (if one exists)
    * `arg arg arg setfile` saves all dirty buffers to disk
    * `arg` "name of thing to open" `setfile` opens the "thing"; an "openable thing" is either a filename, a pseudofile name (pseudofile is another name for temporary editor buffer; these typically have <names> containing characters which cannot legally be present in filenames), or a URL (latter is opened in dflt browser).
    * `arg` "text containing wildcard" `setfile` will open a new "wildcard buffer" containing the names of all files matching the wildcard pattern.  If the "text containing wildcard" ends with a '|' character, the wildcard expansion is recursive.  EX: `arg "*.cpp|" setfile` opens a new buffer containing the names of all the .cpp files found in the cwd and its child trees.
 * `alt+F2` opens file history buffer; its contents reflect a stack of filenames, current on top.  Use `arg setfile` to switch among them.
 * `tags` (`alt+u`): looks up the identifier under the cursor (or arg-provided if any) and "hyperlinks" to it.  If >1 definition is found, a menu of the choices is offered.
    * `ctags.exe` [Exuberant Ctags](http://ctags.sourceforge.net/) is invoked to rebuild the "tags database" at the close of each successful build of K.
    * the set of tags navigated to are added to a linklist which is traversed via `alt+left` and `alt+right`.  Locations hyperlinked from are also added to this list, allowing easy return.
    * those functions appearing in the "Intrinsic Functions" section of <CMD-SWI-Keys> are methods of `ARG::` and can be tags-looked up (providing the best possible documentation to the user: the source code!).
 * `psearch` (`F3`) and `msearch` (`F4`) (referred to as `xsearch` in the following text) are forward and backward text search functions respectively.
    * `xsearch` (w/o arg) searches for the next occurrence of the current search key, in the particular direction.
    * `arg xsearch` changes the current search key to the word in the buffer starting at the cursor and searches for the next occurrence of it, in the particular direction.
    * `arg` "searchkey" `xsearch` changes the current search key to "searchkey" and searches for the next occurrence of it, in the particular direction.
    * `alt+F3` opens a buffer containing previous search keys.
 * `grep` (`ctrl+F3`) creates a new buffer containing one line for each line matching the search key.  `gotofileline` (`alt+g`) comprehends this file format, allowing you to hyperlink back to the match in the grepped file.
 * `mfgrep` (`shift+F4`) creates a new buffer containing one line for each line, from a set of files, matching the search key.  The "set of files" is initialized the first time the user invokes the tags function (there are other ways of course).
 * Regular-expression search is supported.
 * text-replace functions (note: these functions take three arguments: region to perform the replace, search-key, replace string, and the latter two arguments are required to be entered interactively by the user)
     * noarg `replace` (`ctrl+L`) performs a unconditional (noninteractive) replace from the cursor position to the bottom of the buffer.
     * noarg `qreplace` (`ctrl+\`) performs a query-driven (i.e. interactive) replace from the cursor position to the bottom of the buffer.
     * if a selection arg (line, box, stream) is prefixed to `replace` or `qreplace`, only the content of that selection region is subject to the replace operation.
     * `mfreplace` (`F11`) performs a query-driven (i.e. interactive) replace operation across multiple files.
     * Regular-expression replacement is not (yet) supported.
 * the cursor keys (alone and chorded with shift, ctrl and alt keys) should all work as expected, and serve to move the cursor (and extend the arg selection if one is active).
 * `resize` (`alt+w`) allows you to interactively resize the screen and change the console font using the numpad cursor keys and those nearby.
 * `ctrl+c` and `ctrl+v` xfr text between the Windows Clipboard and the editor's <clipboard> buffer in (hopefully) intuitive ways.
 * `+` (copy selection into <clipboard>), `-` (cut selection into <clipboard>) and `ins` keys on the numpad are used to move text between locations in buffers.
 * `execute` (`ctrl+x`):
    * `arg` "editor command string" `execute` executes an editor command (a.k.a. macro) string.
    * `arg arg` "CMD.exe shell command string" `execute` executes an editor command (a.k.a. macro) string.
 * `sort` (`alt+9`) sort contiguous range of lines.  Sort key is either (user provides BOXARG) substring of each line, or (user provides LINEARG) entire line.  After `sort` is invoked, a series of menu prompts allow the user to choose ascending/descending, case (in)sensitive, keep/discard duplicates).

## Argtypes

Legend: `function` is the editor function (embodied in the editor C++ source code as `ARG::function()`) consuming the xxxARG.  

Different `ARG::function()`s (and therefore `function`s) are specified as accepting different argtypes, and the editor command invocation processing code (see `buildexecute.cpp`) which calls `ARG::function()`s will present the user's Arg values to `ARG::function()`s differently depending on these specifications.  The association of `function` name to `ARG::function()`, its acceptable argtypes, and its help-text is sourced from `cmdtbl.dat` which is preprocessed by `cmdtbl.lua` into `cmdtbl.h` at build time:

 * `NOARG`: no arg prefix was active when the function was invoked.  Only the cursor position is passed to `ARG::function()`.
 * `NULLARG`: when the function is invoked with an `arg` prefix but without intervening cursor movement or entry of literal characters.  Depending on other argtype qualifiers, the actual arg seen by `ARG::function()` can vary, but always includes the cursor position and cArg, containing a count, the number of times `arg` was invoked prior:
     * if the `function`s argtype is qualified by `NULLEOW` or `NULLEOL` (these can only apply to `NULLARG`); `ARG::function()` is invoked receiving a `TEXTARG` (string value) containing the string read from buffer text content:
        * `NULLEOL` from the cursor position and extending to the end of the line.  
             * EX: `arg setfile` opens (switches to) the file or URL beginning at the cursor position.  Note that `ARG::setfile()` contains code which further parses the `TEXTARG` value, truncating it at the first whitespace character or in other "magical" ways (using `FBUF::GetLineIsolateFilename()`).
        * `NULLEOW` from the cursor position and including all contiguous "word characters" up to the end of that line (if the cursor is positioned in the middle of a word, `NULLEOW` passes only the trailing substring of the word to `ARG::function()`). 
             * EX: `arg psearch` (likewise `msearch`, `grep`, `mfgrep`) searches for the word beginning at the cursor position. 
 * `TEXTARG`: a string value is passed to `ARG::function()`.  Generated when 
      * a literal string arg entered: `arg` <user types characters to create the string text> `function`
      * a segment of a single line is selected by `arg` followed by horizontal cursor movement followed by `function`.  Internally, if `ARG::function()` is specified as consuming `TEXTARG` qualified with `BOXSTR`, this selected text is transformed into a string value (common with all `TEXTARG` invocations) which is passed to `ARG::function()`.  The `TEXTARG` + `BOXSTR` argtype + qualifier combination prevents single-line `BOXARG`s from being passed to `ARG::function()` (since these are transformed into `TEXTARG`).
 * `BOXARG`: if `function` is specified as accepting `BOXARG`, the user (with the editor in boxmode, the default) to provide this arg type, invokes `arg`, moves the cursor to a different column, either on the same (note `BOXSTR` behaviors) or a different line.  A pair of Point coordinates (ulc, lrc) are passed to `ARG::function()`.
 * `LINEARG`: if `function` is specified as accepting `LINEARG` the user (with the editor in boxmode, the default), the user invokes `arg`, moves the cursor to a different line (while not moving the cursor to a different column) and invokes `function`.  A pair line numbers (yMin, yMax) are passed to `ARG::function()`.
 * `STREAMARG`: this argtype is seldom used and should be considered "under development."

# Historical Notes

K is heavily based upon Microsoft's M editor (released as M.EXE for DOS, and
MEP.EXE for OS/2 and Windows NT), which was first released, and which I first started using in 1988.  [According to a member of the 1990 Windows "NT OS/2" development team](http://blogs.msdn.com/b/larryosterman/archive/2009/08/21/nineteen-years-ago-today-1990.aspx):

> Programming editor -- what editor will we have?  Need better than a simple
> system editor (Better than VI!) [They ended up with "M", the "Microsoft
> Editor" which was a derivative of the ["Z"](http://www.texteditors.org/cgi-bin/wiki.pl?Z) [editor](http://www.applios.com/z/z.html)].

K development started (in spirit) in 1988 when I started writing extensions
for MS's DOS "M" programmer's text editor which was included with Microsoft
(not _Visual_) C 5.1.  In the next MSC releases (6.0, 6.0a, 7.x) MS soon
bloated-up M into PWB (v1.0, 1.1, 2.0; see MSDN.News.200107.PWB.Article.pdf)
then replaced it with the GUI "Visual Studio" IDE when Windows replaced DOS.  I
preferred the simpler yet tremendously powerful M, so starting around 1991 I
wrote my own version, K, with many enhancements.  True to its DOS heritage, K
is a Win32 Console App (with no substantive mouse support aside from the
scroll-wheel) because I have no interest in mice or GUIs.  The current (since
2005) extension language is Lua 5.1.  A full source distro of Lua, plus a few
of its key modules, is included herein, and lua.exe, built herein, is used in
an early build step.

## Toolchain notes

Until 2012/06, I compiled K using MSVC 7.1 (Free 32-bit command line build
toolset offered by MS in 2003, since withdrawn, replaced by Visual Studio
Express Edition).  While I used these MS build tools, I used WinDbg, part of a
free (as of 2011/07/13) "Debugging Tools for Windows" from MS, to debug crashes.

Anyway, I have no fondness for Visual Studio, nor for installers, so when I
finally found [a reliable way to obtain MinGW](http://news.ycombinator.com/item?id=4112374)
and didn't have to pay a significant code-size price for doing so (updt: K.exe's disk footprint has grown significantly since then, mostly at the hands of GCC, though adopting `std::string` and other STL bits has doubtless contributed greatly...), I was thrilled!  Since then I have extensively modified the code to take great
advantage of the major generic features of C++11.  As a result, K no longer
compiles with the MSVC 7.1 compiler.

Per
[Visual-Studio-Why-is-there-No-64-bit-Version](http://blogs.msdn.com/b/ricom/archive/2009/06/10/visual-studio-why-is-there-no-64-bit-version.aspx)
the 32-bit version may be the better (more efficient) one (unless your use case includes
editing > 2GB files), but given STL's removal of support for 32-bit MinGW, we will "follow suit."

[README Markdown Syntax Reference](https://confluence.atlassian.com/display/STASH/Markdown+syntax+guide)