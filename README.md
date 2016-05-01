K is my personal programmer's text editor, whose design is derived from Microsoft's [M editor](http://www.texteditors.org/cgi-bin/wiki.pl?M) (a.k.a. "Microsoft Editor") which was itself derived from the [Z](http://www.texteditors.org/cgi-bin/wiki.pl?Z) [editor](http://www.applios.com/z/z.html).

K runs on Win32 (Console) and Linux (ncurses) platforms, in 32- and 64-bit form.  K is writen in C++ with Lua 5.1 embedded.

[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/5869.svg)](https://scan.coverity.com/projects/5869)

[unlike BitBucket, Github has no markdown TOC feature; Please fix this!](https://github.com/isaacs/github/issues/215)

# Features

 * **Z**: "Reverse-polish" function-execution mode wherein the user creates the function-argument ("xxxARG") using various selection or data-entry modes or argtypes, before the function is invoked; the function's execution behavior adapts to the actual argtype it receives.
     * This allows each editor function (one bound to each key) to potentially perform many different operations, minimizing consumption of the "keyboard namespace".  EX: `setfile` (described below).
 * **Z**: Can switch between line and box (column) selection mode simply by varying the shape of the selection.
 * **M**: No installation: copy and run, delete when done. Run from removable storage.
 * **M**: Easily accessible history of recent files visited, strings searched for and replaced, stored in files in user's homedir.
 * **M**: Edit undo/redo limited only by available memory (effectively infinite).
 * **M**: Default automatic backup of previous versions of all files edited.  Every time a dirty file is saved to disk, the previous incarnation of the file (being overwritten) is moved to `.kbackup\filename.yyyymmdd_hhmmss` where `.kbackup` is a directory created by K in the directory containing `filename`, and `yyyymmdd_hhmmss` is the mtime of the instance `filename` of being saved.  This feature was a lifesaver in the "dark decades" preceding the availability of free, multi-platform DVCS (git, Mercurial), and is much less important when DVCS is used; **use DVCS**!
 * **K**: highlighting
     * "word-under-cursor"
     * comments
     * literal strings/characters
     * conditional regions: C/C++ preprocessor, GNU make
 * **K**: Powerful file/source-code navigation
     * K is integrated with [Exuberant Ctags](http://ctags.sourceforge.net/), enabling a hypertext-linking experience navigating amongst tagged items in your programming project.
     * K can perform multi-file-greps and -replaces targeting sets of files enumerated in any editor buffer.
     * K supports powerful recursive (tree) directory scanning with output to an editor buffer, so, when combined with file-filtering functions such as grep, strip, etc.  it's easy to quickly construct a buffer containing only the names of all of the files of interest to you, and have the multi-file-aware functions reference this buffer.  And since this is based on current filesystem content, it's more likely to be complete and correct than a "project file" which must be independently maintained (and thus can easily fall out of sync with workspace reality).

# Licensing

K itself is released under the [GPLv3 license](http://opensource.org/licenses/GPL-3.0).  See file COPYING.

The K source code distro contains, and K uses, the following source code from external sources:

 * [Lua 5.1](http://www.lua.org/versions.html#5.1) from 2005, licensed under the [MIT License](http://opensource.org/licenses/mit-license.html)
 * [James S. Plank's Red-Black Tree library](http://web.eecs.utk.edu/~plank/plank/rbtree/rbtree.html) from 2000 (substantially modified), licensed under [LGPL](http://opensource.org/licenses/LGPL-2.1)

# Limitations

 * K is a Win32 Console or Linux ncurses app with no mouse support (aside from "scroll wheel" (or trackpad gestures which mimic scroll-wheel behaviors)).  The UI is fairly minimal: there are no "pulldown menus" though primitive "pop-up menus" are used on a per-function basis.
 * K has no "virtual memory" mechanism (as M did); files are loaded in toto into RAM; K WILL CRASH if you attempt to open a file that is larger than the biggest malloc'able block available to the K process (now that all OS's default-deploy their x64 variant (and K is buildable as an x64 app) this is practically never a concern).
 * K operates on ASCII files; is has no support for Unicode/UTF-8/MBCS/etc. content (it's a text-editor, not a word processor).  I am increasingly entertaining the possibility of adding UTF-8 support.

# Building

## External build dependencies

### Windows

* GCC from the [nuwen.net distribution of MinGW](http://nuwen.net/mingw.html).  The downloads are self-extracting-GUI 7zip archives which contain bat files (I use `set_distro_paths.bat` below) which add the appropriate environment variable values sufficient to use gcc from the cmdline.  I use the following 1-line bat files (stored outside the K repo because their content is dependent on where the MinGW packages are extracted) to setup MinGW for building K (or any other C/C++ project):
     * `mingw.bat` (x64): `c:\_tools\mingw\64\mingw\set_distro_paths.bat`
     * `mingw32.bat` (i386): `c:\_tools\mingw\32\mingw\set_distro_paths.bat`
 * `ctags.exe` ([Exuberant Ctags](http://ctags.sourceforge.net/)) is invoked to rebuild the "tags database" at the close of each successful build of K, and must be in `PATH`.
 * `7z.exe` ([7zip](http://www.7-zip.org/)) is invoked when building the `rls` `make` target, and must be in `PATH`.

### Linux (Ubuntu 14.04+ (tested on 14.04, 14.10, 15.10))

 * after cloning this repo, run `./install_build_tools_ubuntu.sh` to install the necessary packages.
    * I first built (and _still_ build 32-bit Windows) K with GCC using GCC 4.8; it probably will not build with any lesser GCC version.
    * Requires Boost 1.54 or newer (1.53's boost::string_ref contains (at least in CentOS 7.2.1511) a compile-breaking bug)

## To build

    make clean
    make -j     # the build is parallel-make-safe

To clean a repo sufficient to switch between 32-bit and 64-bit toolchains:

    make zap    # clean plus nuke all lua.exe related

Note that [MinGW gcc non-optionally dyn-links to MSVCRT.DLL](http://mingw-users.1079350.n2.nabble.com/2-Question-on-Mingw-td7578166.html)
which it assumes is already present on any Windows PC.

## Create Release files

A release file is a 7zip (Linux: tgz) archive containing the minimum fileset needed to use the editor.  On Windows two (2) variants of the release file are created by `make rls`: `k_rls.7z` and `k_rls.exe` (a self-extracting-console archive).

Use: decompress the release file in an empty directory and run `k.exe` (Linux: `k`).  K was designed to be "copy and run" (a "release") anywhere.  I have successfully run it from network/NFS shares and "thumb drives".

## Stability notes

The last nuwen.net MinGW release (w/GCC 4.8.1) that builds 32-bit targets is 10.4, released 2013/08/01, and no longer available from nuwen.net.  So, while I continue to build K as both 32- and 64- bit .exe's (and can supply a copy of the nuwen.net MinGW 10.4 release upon request), the future of K on the Windows platform is clearly x64 only.

The 64-bit build of K is relatively recent (first release 2014/02/09) but it's *mostly* working fine so far (updt: on Win7 (targeting a WQXGA (2560x1600) monitor), I get an assertion failure related to console reads (these never occur with the 32-bit K); also these never occur with the x64 K running in Win 8.x (but targeting HD+ (1600x900) resolution); the only time I use Win7 is at work (I am one of seemingly few people who can look past the "Metro" UI of Win 8.x and find a core OS that is superior to Win7).
Update 2016/04: I havn't used K on high-res monitors much lately, but haven't experienced this problem in recent memory (on Win 7, 8.1, or 10).

## Linux key-decoding status quo

The default (Windows-originated) K key mappings make extensive use of `ctrl+` and `alt+` modulated function and keypad keys.  Getting such key combinations to decode correctly on Linux/ncurses has been by far the most time-consuming and code-churning part of the port to Ubuntu Linux 14.04+ (see file conin_ncurses.md for the current state of this activity).  The status quo:

  * Ubuntu 14.04+ Desktop
    * common: with `TERM=xterm`, _after_ you disable various terminal-menu/-command key-modulation (e.g. `alt+`) hooks, default terminfo for xterm correctly decodes a substantial proportion of the Windows-supported key combinations that K uses.
    * Lubuntu Desktop (`lxterminal` nee `x-terminal-emulator`): mouse scroll wheel does not work.
    * I think I've exhausted the possibilities here
  * PuTTY (also PuTTYtray) (to Ubuntu 14.04+)
    * _must_ configure PuTTY to set `TERM=putty` (or `TERM=putty-256color`) because the default (`TERM=xterm`) is remarkably broken.
       * even using these two terminal types, only unmodulated function keys are correctly decoded; shift+, ctrl+, shift+ctrl+, and alt+ modulated function keys map to the corresponding unmodulated function keys.
    * better (but with a caveat): `TERM=putty-sco` adds support for shift+, ctrl+, and shift+ctrl+, but _NOT_ alt+ function keys.
       * **caveat**: `TERM=putty-sco` **requires nondefault PuTTY config**: Terminal / Keyboard / the function keys and keypad : select `SCO`.
    * to make these terminal types available on Debian-based (i.e. *ubuntu) Linux, package `ncurses-term` _may_ need to be installed.
    * [emacswiki/emacs/PuTTY](http://emacswiki.org/emacs/PuTTY) seems a good resource regarding PuTTY keyboard peculiarities.
  * tmux (1.8 - 2.0) (`TERM=screen`)
    * most `ctrl+` and `alt+` function and keypad modulations do not work.
    * I've not begun investigating the possibilities here.

# Debug/Development

I use [DebugView](http://technet.microsoft.com/en-us/sysinternals/bb896647.aspx) to capture the output from the DBG macros which are sprinkled liberally throughout the source code.

The newest nuwen.net (64-bit-only) MinGW distros include `gdb`, and I have used it a couple of times.  I generally only use a debugger to debug crashes, so if `gdb` is unavailable (e.g. when nuwen.net MinGW distros omitted `gdb`) I use [DrMinGW](https://github.com/jrfonseca/drmingw) as a minimalist way of obtaining a useful stack-trace when a crash occurs.  In order to use either DrMinGW or `gdb` it is necessary to build K w/full debug information; open GNUmakefile, search for "DBG_BUILD" for instructions on how to modify that file to build K most suitably for DrMinGW and `gdb`.

# Editor State/History Persistence

K persists information between sessions in state files written to

 * Windows: `%APPDATA%\Kevins Editor\*`
    * K ignores the Windows Registry.
 * Linux: `${XDG_CACHE_HOME:-$HOME/.cache}/kedit/$(hostname)/`
    * `$(hostname)` is added since it is not unusual for a user's $HOME to be located on a shared filesystem (e.g. NFS).

Information stored in state files includes:

 *  recent files edited (including window/cursor position)
 *  recent search-key and replace-string values
 *  function-invocation-count accumulators (for fact-driven key assignment choices)

# Tutorial

## Command line invocation

 * to edit the previously edited file, run `k`
 * to edit file `filename`, run `k filename`
 * run `k -h` to display full cmdline invocation help.

## Argtypes

Legend: `function` is the editor function (embodied in the editor C++ source code as `ARG::function()`) consuming the xxxARG.  

The following outline describes all possible argtypes.  Different `ARG::function()`s (and therefore `function`s) are specified as accepting particular argtypes (one or more), and the editor command invocation processing code (see `buildexecute.cpp`) which calls `ARG::function()`s will present the user's arg value to the invoked `ARG::function()` differently depending on these specifications.  The association of `function` name to `ARG::function()`, its acceptable argtypes, and its help-text is sourced from `cmdtbl.dat` which is preprocessed by `cmdtbl.lua` into `cmdtbl.h` at build time:

 * `NOARG`: if `function` is invoked with no arg prefix active.  Only the cursor position is passed to `ARG::function()`.
 * `NULLARG`: if `function` is invoked with an `arg` prefix active but without intervening cursor movement or entry of literal characters.  The actual argtype received by `ARG::function()` can vary, but always includes the cursor position and cArg, containing a count, the number of times `arg` was invoked prior:
     * if the `function`s argtype is qualified by `NULLEOW` or `NULLEOL` (these can only apply to `NULLARG`), `ARG::function()` receives a `TEXTARG` (string value) containing a string value read from buffer text content:
        * `NULLEOL`: from cursor to end of the line.
             * EX: `arg setfile` opens (switches to) the file or URL beginning at the cursor position.  Note that `ARG::setfile()` contains code which further parses the `TEXTARG` string value, truncating it at the first whitespace character or in other "magical" ways (see `FBUF::GetLineIsolateFilename()`).
        * `NULLEOW`: from cursor and including all contiguous "word characters" through end of line (if the cursor is positioned in the middle of a word, `NULLEOW` passes only the trailing substring of the word to `ARG::function()`). 
             * EX: `arg psearch` (likewise `msearch`, `grep`, `mfgrep`) searches for the word beginning at the cursor position.
 * `TEXTARG`: when a string value is passed to `ARG::anyfunction()`.  Generated when: 
      * a literal string arg entered: `arg` <user types characters to create the string text> `anyfunction`
      * `arg` <horizontal cursor movement selecting a segment of the current line> `anyfunction`.  Internally, if `ARG::anyfunction()` is specified as consuming `TEXTARG` qualified with `BOXSTR`, this selected text is transformed into a `TEXTARG` (string value) which is passed to `ARG::anyfunction()`.  The `TEXTARG` + `BOXSTR` argtype + qualifier combination prevents single-line `BOXARG`s from being passed to `ARG::function()` (since these are transformed into `TEXTARG`).
      * EX: `arg arg TEXTARG psearch` (likewise `msearch`, `grep`, `mfgrep`) searches for the regular expression TEXTARG.
 * `BOXARG`: if `ARG::anyfunction()` is specified as accepting `BOXARG`, the user (with the editor in boxmode, the default), to provide this arg type, invokes `arg`, moves the cursor to a different column, either on the same (note `BOXSTR` caveat above) or a different line.  A pair of Point coordinates (ulc, lrc) are passed to `ARG::function()`.
 * `LINEARG`: if `function` is specified as accepting `LINEARG` the user (with the editor in boxmode, the default), the user invokes `arg`, moves the cursor to a different line (while not moving the cursor to a different column) and invokes `function`.  A pair line numbers (yMin, yMax) are passed to `ARG::function()`.
 * `STREAMARG`: this argtype is seldom used and should be considered "under development."

## Essential Functions

The editor implements a large number of functions, all of which the user can invoke by name using the `execute` or `selcmd` functions, or bind to any key. Every key has one function bound to it (and the user is completely free to change these bindings).  The current key bindings can be viewed by executing function `newhelp` bound to `alt+h`. Functions can also be invoked by/within macros.  Following are some of the most commonly used functions:

 * `exit` (`ctrl+4`, `alt+F4`) exits the editor; the user is prompted to save any dirty files (one by one, or all remaining).
 * `undo` (`ctrl+e`,`alt+backspace`) undo the most recent editing operation.  Repeatedly invoking `undo` will successively undo all editing operations.
 * `redo` (`ctrl+r`,`ctrl+backspace`) redo the most recently `undo`ne editing operation.  Repeatedly invoking `redo` will successively redo all `undo`ne editing operations.
 * `arg` (`center`: numeric keypad 5 key with numlock off (the state I always use)).  Used to introduce arguments to other editor functions. `arg` can be invoked multiple times prior to invoking `anyfunction`; this may (depending on the editor function implementation) serve to modify the behavior of `anyfunction` (see `setfile`)
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
    * `arg arg` "name of file" `setfile` saves the current buffer to the file named "name of file" (and gives the buffer this name henceforth).
 * `ctrl+c` and `ctrl+v` xfr text between the Win32 (Windows) or X (Linux) Clipboard and the editor's <clipboard> buffer in (hopefully) intuitive ways.
    * The Linux implementation depends on [`xclip`](http://sourceforge.net/projects/xclip/) being installed; `sudo apt-get install xclip` FTW!
 * `ctrl+q`,`alt+F2` opens visited-file history buffer; from most- to least-recently visited.  Use cursor movement functions and `arg setfile` to switch among them.
 * `num++` (copy selection into <clipboard>), `num+-` (cut selection into <clipboard>) and `ins` (paste text from <clipboard>) keys on the numpad are used to move text between locations in buffers via <clipboard>.
 * `execute` (`ctrl+x`):
    * `arg` "editor command string" `execute` executes an editor function sequence (a.k.a. macro) string.
    * `arg arg` "CMD.exe shell command string" `execute` executes a CMD.exe shell (a.k.a. DOS) command string with stdout and stderr captured to an editor buffer.
 * `tags` (`alt+u`): looks up the identifier under the cursor (or arg-provided if any) and "hyperlinks" to it.  If >1 definition is found, a menu of the choices is offered.
    * [Exuberant Ctags](http://ctags.sourceforge.net/) `ctags.exe` is invoked to rebuild the "tags database" at the close of each successful build of K.
    * the set of tags navigated to are added to a linklist which is traversed via `alt+left` and `alt+right`.  Locations hyperlinked from are also added to this list, allowing easy return.
    * those functions appearing in the "Intrinsic Functions" section of <CMD-SWI-Keys> are methods of `ARG::` and can be tags-looked up (providing the best possible documentation to the user: the source code!).
 * PCRE Regular-expression (regex) search & replace: all search and replace functions, when prefixed with `arg arg` (2-arg), operate in regex mode.
 * `psearch` (`F3`) / `msearch` (`F4`) (referred to as `xsearch` in the following text) search forward / backward from the cursor position.
    * `alt+F3` opens a buffer containing previous search keys.
    * `xsearch` (w/o arg) searches for the next match of the current search key.
    * `arg xsearch` changes the current search key to the word in the buffer starting at the cursor and searches for the next match.
    * `arg` "searchkey" `xsearch` changes the current search key to "searchkey" and searches for the next match.
    * `grep` (`ctrl+F3`) creates a new buffer containing one line for each line matching the search key.  `gotofileline` (`alt+g`) comprehends this file format, allowing you to hyperlink back to the match in the grepped file.
    * `mfgrep` (`shift+F4`) creates a new buffer containing one line for each line, from a set of files, matching the search key.  The "set of files" is initialized the first time the user invokes the tags function (there are other ways of course).
    * In regex mode (when prefixed with `arg arg`) the search string is treated as a PCRE regular expression.
 * text-replace functions (note: these functions take three arguments: region to perform the replace, search-key, replace string, and the latter two arguments are required to be entered interactively by the user)
    * noarg `replace` (`ctrl+L`) performs a unconditional (noninteractive) replace from the cursor position to the bottom of the buffer.
    * noarg `qreplace` (`ctrl+\`) performs a query-driven (i.e. interactive) replace from the cursor position to the bottom of the buffer.
    * if a selection arg (line, box, stream) is prefixed to `replace` or `qreplace`, only the content of that selection region is subject to the replace operation.
    * `mfreplace` (`F11`) performs a query-driven (i.e. interactive) replace operation across multiple entire files.
    * Regular-expression (PCRE) replace is supported: in regex mode (when prefixed with `arg arg`) the search string is treated as a regular expression, and replace functions support the replacement string ; insertion of regex captures in the replacement string via `\n` where `n` is the capture number.
    * In regex mode (when prefixed with `arg arg`) the search string is treated as a PCRE regular expression, and the replacement string may reference regex captures in the replacement string via `\n` where `n` is the (single-digit) capture number.
 * the cursor keys (alone and chorded with shift, ctrl and alt keys) should all work as expected, and serve to move the cursor (and extend the arg selection if one is active).
 * `sort` (`alt+9`) sort contiguous range of lines.  Sort key is either (user provides BOXARG) substring of each line, or (user provides LINEARG) entire line.  After `sort` is invoked, a series of menu prompts allow the user to choose ascending/descending, case (in)sensitive, keep/discard duplicates).

### menu functions

K has a rudimentary TUI "pop-up menu system" (written largely in Lua), and a number of editor functions which generate a list of chioces to a menu, allowing the user to pick one.  These functions are given short mnemonic names as the intended invocation is `arg` "fxnm" `ctrl+x`

 * `mom` "menu of menus": menu of Lua-based editor menu functions
 * `sb` "system buffers"
 * `dp` "dirs of parent" all parent dirs
 * `dc` "dirs child" all child dirs
 * `gm` "grep-related commands"
 * `cur` "inert menu displaying dynamic macro definitions"

### Win32-only functions

 * `resize` (`alt+w`) allows you to interactively resize the screen and change the console font using the numpad cursor keys and those nearby.
 * `websearch` (`alt+6`): perform web search on string (opens in default browser)
     * `arg` "search string" `websearch`: perform Google web search for "search string"
     * `arg arg` "search string" `websearch`: display menu of all available search engines (see `user.lua`) and perform a web search for "search string" using the chosen search engine.

# Historical Notes

K is heavily based upon Microsoft's [M editor](http://www.texteditors.org/cgi-bin/wiki.pl?M) (a.k.a. "Microsoft Editor", released as `M.EXE` for DOS, and `MEP.EXE` for OS/2 and Windows NT), which was first released, and which I first started using, in 1988.  [According to Larry Osterman, a member of the 1990 Windows "NT OS/2" development team](http://blogs.msdn.com/b/larryosterman/archive/2009/08/21/nineteen-years-ago-today-1990.aspx):

> Programming editor -- what editor will we have?  Need better than a simple
> system editor (Better than VI!) [They ended up with ["M"](http://www.texteditors.org/cgi-bin/wiki.pl?M), the "Microsoft
> Editor" which was a derivative of the ["Z"](http://www.texteditors.org/cgi-bin/wiki.pl?Z) [editor](http://www.applios.com/z/z.html)].

K development started (in spirit) in 1988 when I started writing extensions for the Microsoft [M editor](http://www.texteditors.org/cgi-bin/wiki.pl?M) which was included with Microsoft (not _Visual_) C 5.1 for DOS & OS/2.  In the next Microsoft C releases (6.0, 6.0a, 7.x) for DOS and OS/2, Microsoft bloated-up M into [PWB](http://www.texteditors.org/cgi-bin/wiki.pl?PWB) (v1.0, 1.1, 2.0; see MSDN.News.200107.PWB.Article.pdf) then replaced it with the GUI "Visual Studio" IDE when Windows replaced DOS.  I preferred the simpler yet tremendously powerful M, so starting in 1991 I wrote my own version, K.  True to its DOS heritage, K is a Win32 Console App (with no mouse support aside from the scroll-wheel) because I have no interest in mice or GUIs.  The current (since 2005) extension language is Lua 5.1.  A full source distro of Lua, plus a few of its key modules, is included herein, and `lua.exe`, built herein, is used in an early build step.

2014/10: an "employment transition" into an (effectively) Linux-only environment (willingly) forced me to port K to (x64) Linux; I had wanted to do this for years, but lacked the motivation: the prospect of working daily on a platform w/o K provided the needed motivation!

2014/11: I just discovered ["Q" Text Editor](http://www.schulenberg.com/page2.htm), another (Win32+x64) re-implementation of the "M" Editor, written in FORTRAN using the QuickWin framework!

## Toolchain notes

Until 2012/06, I compiled K using the free "Microsoft Visual C++ Toolkit 2003" containing MSVC++ 7.1 32-bit command line build tools (since withdrawn, replaced by Visual Studio Express Edition).  While I used these MS build tools, I used [WinDbg](http://en.wikipedia.org/wiki/WinDbg) to debug crashes.

I have no fondness for massive IDE's (e.g. Visual Studio), nor for installers, so when I finally found [a reliable way to obtain MinGW](http://news.ycombinator.com/item?id=4112374)
and didn't have to pay a significant code-size price for doing so (updt: K.exe's disk footprint has grown significantly since then, mostly at the hands of GCC, though adopting `std::string` and other STL bits has doubtless contributed greatly...), I was thrilled!  Since then I have extensively modified the K code to take great
advantage of the major generic features of C++11; consequently K no longer compiles with MSVC++ 7.1.

Per [Visual-Studio-Why-is-there-No-64-bit-Version](http://blogs.msdn.com/b/ricom/archive/2009/06/10/visual-studio-why-is-there-no-64-bit-version.aspx) the 32-bit version of K may be the better (more efficient) one (unless your use case includes editing > 2GB files), but given STL's removal of support for 32-bit MinGW, we will "follow suit."  And of course, Linux in 2014+ is almost universally 64-bit (and 64-bit Linux K has no known anomalies).

[README Markdown Syntax Reference](https://confluence.atlassian.com/display/STASH/Markdown+syntax+guide)
