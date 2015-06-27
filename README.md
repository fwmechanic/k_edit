K is my personal programmer's text editor, whose design is derived from Microsoft's ["M"](http://www.texteditors.org/cgi-bin/wiki.pl?M) editor which was itself derived from the ["Z"](http://www.texteditors.org/cgi-bin/wiki.pl?Z) [editor](http://www.applios.com/z/z.html).

K is a TUI app that runs on Win32 (Console) and Linux (ncurses) platforms, in 32- and 64-bit form.

[Github has no markdown TOC feature; what a drag]

# Features

 * **Z**: "Reverse-polish" function-execution mode wherein the user creates the function-argument ("xxxARG") using various selection or data-entry modes or argtypes, before the function is invoked; the function's execution behavior adapts to the actual argtype it receives.
     * This allows each editor function (one bound to each key) to potentially perform many different operations, minimizing consumption of the "keyboard namespace".  EX: `setfile` (described below).
 * **Z**: Can switch between line and box (column) selection mode simply by varying the shape of the selection.
 * **M**: No installation: copy and run, delete when done. Run from removable storage.
 * **M**: Edit undo/redo limited only by available memory (effectively infinite).
 * **M**: Default automatic backup of previous versions of all files edited.  Every time a dirty file is saved to disk, the previous incarnation of the file (being overwritten) is moved to `.kbackup\filename.yyyymmdd_hhmmss` where `.kbackup` is a directory created by K in the directory containing `filename`, and `yyyymmdd_hhmmss` is the mtime of the instance `filename` of being saved.  This feature was a lifesaver in the "dark decades" preceding the availability of free, multi-platform DVCS (git, Mercurial), and is much less important when DVCS is used; **use DVCS**!
 * **K**: (Partial) syntax highlighting (C/C++, Lua, Python)
     * comments
     * literal strings/characters
     * conditional regions: C/C++ preprocessor, GNU make
 * **K**: Powerful file/source-code navigation
     * K is integrated with [Exuberant Ctags](http://ctags.sourceforge.net/), enabling a hypertext-linking experience navigating amongst tagged items in your programming project.
     * K can perform multi-file-greps and -replaces targeting sets of files enumerated in any editor buffer.  K supports powerful recursive (tree) directory scanning with output to an editor buffer, so, when combined with file-filtering functions such as grep, strip, etc. it's easy to quickly construct a buffer containing only
the names of all of the files of interest to you, and have the multi-file-aware functions reference this buffer.  And since this is based on current filesystem content, it's more likely to be complete and correct than a
"project file" which must be independently maintained (and thus can easily fall out of sync with workspace reality).

# Licensing

K itself is released under the [GPLv3 license](http://opensource.org/licenses/GPL-3.0).  See file COPYING.

The K source code distro contains, and K uses, the following source code from external sources:

 * [Lua 5.1](http://www.lua.org/versions.html#5.1) from 2005, licensed under the [MIT License](http://opensource.org/licenses/mit-license.html)
 * [James S. Plank's Red-Black Tree library](http://web.eecs.utk.edu/~plank/plank/rbtree/rbtree.html) from 2000, licensed under [LGPL](http://opensource.org/licenses/LGPL-2.1)

# Limitations

 * K is a Win32 Console or Linux ncurses app with no mouse support (aside from "scroll wheel" (or trackpad gestures which mimic scroll-wheel behaviors)).  The UI is fairly minimal: there are no "pulldown menus" though primitive "pop-up menus" are used on a per-function basis.
 * K has no "virtual memory" mechanism (as M did); edited files are loaded in toto into RAM; K WILL CRASH if you attempt to open a file that is larger than the biggest malloc'able block available to the K process.  I get hit by this maybe once a year, and it's easy enough to head/tail/grep to chop a huge (usually some sort of log) file into manageable pieces.  Also, the (default) 64-bit version raises this ceiling considerably.
 * K operates on ASCII files; is has no support for Unicode/MBCS/etc. content (it's a text-editor, not a word processor).  Lately (very rarely) I get hit with problems related to non-ASCII filenames: when I download music, the names of file or dirs in the received package occasionally contain characters which have to be TRANSLATED into the charset that K uses.  If I then construct a cmdline to rename said file/dir (a task I often perform with K), the command will fail because the filename (containing the translated character instead of the original character) will not exist.  As with the "lack of VM" limitation, this is rarely annoying, and will only be resolved (a large undertaking) if it becomes much more annoying to me (which seems unlikely).

# Building

## External build dependencies

### Windows

* GCC from the [nuwen.net distribution of MinGW](http://nuwen.net/mingw.html).  The downloads are self-extracting-GUI 7zip archives which contain bat files (I use `set_distro_paths.bat` below) which add the appropriate environment variable values sufficient to use gcc from the cmdline.  I use the following 1-line bat files (stored outside the K repo because their content is dependent on where the MinGW packages are extracted) to setup MinGW for building K (or any other C/C++ project):
     * `mingw.bat` (x64): `c:\_tools\mingw\64\mingw\set_distro_paths.bat`
     * `mingw32.bat` (i386): `c:\_tools\mingw\32\mingw\set_distro_paths.bat`
 * `ctags.exe` ([Exuberant Ctags](http://ctags.sourceforge.net/)) is invoked to rebuild the "tags database" at the close of each successful build of K, and must be in `PATH`.
 * `7z.exe` ([7zip](http://www.7-zip.org/)) is invoked when building the `rls` `make` target, and must be in `PATH`.

### Ubuntu Linux (14.04, 14.10)

 * each time you open a terminal to build K, run `. setup_build_tools_ubuntu.sh` (tab-completion is your friend); this operation (which corresponds with executing `mingw.bat` in Windows) will
    * remap the K build to use GCC 4.8.x rather than the default version of GCC
    * install GCC 4.8.x and other build tools/libraries if they're missing; in this case, `sudo apt-get ...` is executed, so you'll be prompted by `sudo` for your password.
        * the install-checking in `setup_build_tools_ubuntu.sh` is decidedly imperfect; to unilaterally install the build prereqs on *Ubuntu, run `install_build_tools_ubuntu.sh`

## To build

    make clean
    make -j     & the build is parallel-make-safe

To clean a repo sufficient to switch between 32-bit and 64-bit toolchains:

    make zap    & clean plus nuke all lua.exe related

Note that [MinGW gcc non-optionally dyn-links to MSVCRT.DLL](http://mingw-users.1079350.n2.nabble.com/2-Question-on-Mingw-td7578166.html)
which it assumes is already present on any Windows PC.

## Create Release files

A release file is a 7zip archive containing the minimum fileset needed to use the editor.  Two variants of the release file are created by `make rls`: `k_rls.7z` and `k_rls.exe` (a self-extracting-console archive). 

Use: decompress the release file in an empty directory and run `k.exe`.  K was designed to be "copy and run" (a "release") anywhere.  I have successfully run it from network shares and "thumb drives".

## Stability notes

The last nuwen.net MinGW release (w/GCC 4.8.1) that builds 32-bit targets is 10.4, released 2013/08/01, and no longer available from nuwen.net.  So, while I continue to build K as both 32- and 64- bit .exe's (and can supply a copy of the nuwen.net MinGW 10.4 release upon request), the future of K on the Win32 platform is clearly x64 only.

The 64-bit build of K is relatively recent (first release 2014/02/09) but it's *mostly* working fine so far (updt: on Win7 (targeting a WQXGA (2560x1600) monitor), I get an assertion failure related to console reads (these never occur with the 32-bit K); also these never occur with the x64 K running in Win 8.x (but targeting HD+ (1600x900) resolution); the only time I use Win7 is at work (I am one of seemingly few people who can look past the "Metro" UI of Win 8.x and find a core OS that is superior to Win7).

# Debug/Development

I use [DebugView](http://technet.microsoft.com/en-us/sysinternals/bb896647.aspx) to capture the output from the DBG macros which are sprinkled liberally throughout the source code.

The newest nuwen.net (64-bit-only) MinGW distros include `gdb`, and I have used it a couple of times.  I generally only use a debugger to debug crashes, so if `gdb` is unavailable (e.g. when nuwen.net MinGW distros omitted `gdb`) I use [DrMinGW](https://github.com/jrfonseca/drmingw) as a minimalist way of obtaining a useful stack-trace when a crash occurs.  In order to use either DrMinGW or `gdb` it is necessary to build K w/full debug information; open GNUmakefile, search for "DBG_BUILD" for instructions on how to modify that file to build K most suitably for DrMinGW and `gdb`.

# Editor State Files

K ignores the Windows Registry.  K persists information between sessions in state files written to `%APPDATA%\Kevins Editor\*` (`~/.kedit` on Linux).  Information stored in state files includes:

 *  files edited (including window/cursor position)
 *  search-key and replace-string history
 *  function invocation accumulator-counters

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
 * `BOXARG`: if `ARG::anyfunction()` is specified as accepting `BOXARG`, the user (with the editor in boxmode, the default), to provide this arg type, invokes `arg`, moves the cursor to a different column, either on the same (note `BOXSTR` caveat above) or a different line.  A pair of Point coordinates (ulc, lrc) are passed to `ARG::function()`.
 * `LINEARG`: if `function` is specified as accepting `LINEARG` the user (with the editor in boxmode, the default), the user invokes `arg`, moves the cursor to a different line (while not moving the cursor to a different column) and invokes `function`.  A pair line numbers (yMin, yMax) are passed to `ARG::function()`.
 * `STREAMARG`: this argtype is seldom used and should be considered "under development."

## Essential Functions

The editor implements a large number of functions, all of which the user can invoke. Every key has one function bound to it (and the user is completely free to change these bindings), and functions can also be invoked within macros and via the `execute` function.  Following are some of the most commonly used functions:

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
 * `ctrl+q`,`alt+F2` opens visited-file history buffer; from most- to least-recently visited.  Use cursor movement functions and `arg setfile` to switch among them.
 * `num++` (copy selection into <clipboard>), `num+-` (cut selection into <clipboard>) and `ins` (paste text from <clipboard>) keys on the numpad are used to move text between locations in buffers via <clipboard>.
 * `execute` (`ctrl+x`):
    * `arg` "editor command string" `execute` executes an editor function sequence (a.k.a. macro) string.
    * `arg arg` "CMD.exe shell command string" `execute` executes a CMD.exe shell (a.k.a. DOS) command string with stdout and stderr captured to an editor buffer.
 * `tags` (`alt+u`): looks up the identifier under the cursor (or arg-provided if any) and "hyperlinks" to it.  If >1 definition is found, a menu of the choices is offered.
    * [Exuberant Ctags](http://ctags.sourceforge.net/) `ctags.exe` is invoked to rebuild the "tags database" at the close of each successful build of K.
    * the set of tags navigated to are added to a linklist which is traversed via `alt+left` and `alt+right`.  Locations hyperlinked from are also added to this list, allowing easy return.
    * those functions appearing in the "Intrinsic Functions" section of <CMD-SWI-Keys> are methods of `ARG::` and can be tags-looked up (providing the best possible documentation to the user: the source code!).
 * `psearch` (`F3`) / `msearch` (`F4`) (referred to as `xsearch` in the following text) search forward / backward from the cursor position.
    * `alt+F3` opens a buffer containing previous search keys.
    * `xsearch` (w/o arg) searches for the next match of the current search key.
    * `arg xsearch` changes the current search key to the word in the buffer starting at the cursor and searches for the next match.
    * `arg` "searchkey" `xsearch` changes the current search key to "searchkey" and searches for the next match.
    * `grep` (`ctrl+F3`) creates a new buffer containing one line for each line matching the search key.  `gotofileline` (`alt+g`) comprehends this file format, allowing you to hyperlink back to the match in the grepped file.
    * `mfgrep` (`shift+F4`) creates a new buffer containing one line for each line, from a set of files, matching the search key.  The "set of files" is initialized the first time the user invokes the tags function (there are other ways of course).
    * Regular-expression (PCRE) search is supported.
 * text-replace functions (note: these functions take three arguments: region to perform the replace, search-key, replace string, and the latter two arguments are required to be entered interactively by the user)
     * noarg `replace` (`ctrl+L`) performs a unconditional (noninteractive) replace from the cursor position to the bottom of the buffer.
     * noarg `qreplace` (`ctrl+\`) performs a query-driven (i.e. interactive) replace from the cursor position to the bottom of the buffer.
     * if a selection arg (line, box, stream) is prefixed to `replace` or `qreplace`, only the content of that selection region is subject to the replace operation.
     * `mfreplace` (`F11`) performs a query-driven (i.e. interactive) replace operation across multiple files.
     * Regular-expression replacement is not (yet) supported.
 * the cursor keys (alone and chorded with shift, ctrl and alt keys) should all work as expected, and serve to move the cursor (and extend the arg selection if one is active).
 * `sort` (`alt+9`) sort contiguous range of lines.  Sort key is either (user provides BOXARG) substring of each line, or (user provides LINEARG) entire line.  After `sort` is invoked, a series of menu prompts allow the user to choose ascending/descending, case (in)sensitive, keep/discard duplicates).

### menu functions

K has a rudimentary TUI "pop-up menu system" (written largely in Lua), and a number of editor functions which generate a list of chioces to a menu, allowing the user to pick one.  These functions are given short mnemonic names as the intended invocation is `arg` "fxnm" `ctrl+x`

 * `mom` menu of Lua-based commands
 * `sb` "system buffers"
 * `dp` "dirs of parent" all parent dirs
 * `dc` "dirs child" all child dirs
 * `gm` "grep-related commands"
 * `cur` "inert menu displaying dynamic macro definitions"

### Win32-only functions

 * `ctrl+c` and `ctrl+v` xfr text between the Windows Clipboard and the editor's <clipboard> buffer in (hopefully) intuitive ways.
 * `resize` (`alt+w`) allows you to interactively resize the screen and change the console font using the numpad cursor keys and those nearby.
 * `websearch` (`alt+6`): perform web search on string (opens in default browser)
     * `arg` "search string" `websearch`: perform Google web search for "search string"
     * `arg arg` "search string" `websearch`: display menu of all available search engines (see `user.lua`) and perform a web search for "search string" using the chosen search engine.

# Historical Notes

K is heavily based upon Microsoft's M editor (released as M.EXE for DOS, and MEP.EXE for OS/2 and Windows NT), which was first released, and which I first started using, in 1988.  [According to a member of the 1990 Windows "NT OS/2" development team](http://blogs.msdn.com/b/larryosterman/archive/2009/08/21/nineteen-years-ago-today-1990.aspx):

> Programming editor -- what editor will we have?  Need better than a simple
> system editor (Better than VI!) [They ended up with ["M"](http://www.texteditors.org/cgi-bin/wiki.pl?M), the "Microsoft
> Editor" which was a derivative of the ["Z"](http://www.texteditors.org/cgi-bin/wiki.pl?Z) [editor](http://www.applios.com/z/z.html)].

K development started (in spirit) in 1988 when I started writing extensions for the Microsoft "M" Editor which was included with Microsoft (not _Visual_) C 5.1 for DOS & OS/2.  In the next MSC releases (6.0, 6.0a, 7.x) MS soon bloated-up M into PWB (v1.0, 1.1, 2.0; see MSDN.News.200107.PWB.Article.pdf) then replaced it with the GUI "Visual Studio" IDE when Windows replaced DOS.  I preferred the simpler yet tremendously powerful M, so starting in 1991 I wrote my own version, K.  True to its DOS heritage, K is a Win32 Console App (with no mouse support aside from the scroll-wheel) because I have no interest in mice or GUIs.  The current (since 2005) extension language is Lua 5.1.  A full source distro of Lua, plus a few of its key modules, is included herein, and `lua.exe`, built herein, is used in an early build step.

2014/11/23: I just discovered ["Q" Text Editor](http://www.schulenberg.com/page2.htm), another (Win32+x64) re-implementation of the "M" Editor, written in FORTRAN using the QuickWin framework!

## Toolchain notes

Until 2012/06, I compiled K using the free "Microsoft Visual C++ Toolkit 2003" containing MSVC++ 7.1 32-bit command line build tools (since withdrawn, replaced by Visual Studio Express Edition).  While I used these MS build tools, I used [WinDbg](http://en.wikipedia.org/wiki/WinDbg), to debug crashes.

I have no fondness for Visual Studio, nor for installers, so when I finally found [a reliable way to obtain MinGW](http://news.ycombinator.com/item?id=4112374)
and didn't have to pay a significant code-size price for doing so (updt: K.exe's disk footprint has grown significantly since then, mostly at the hands of GCC, though adopting `std::string` and other STL bits has doubtless contributed greatly...), I was thrilled!  Since then I have extensively modified the K code to take great
advantage of the major generic features of C++11.  As a result, K no longer compiles with the MSVC 7.1 compiler.

Per
[Visual-Studio-Why-is-there-No-64-bit-Version](http://blogs.msdn.com/b/ricom/archive/2009/06/10/visual-studio-why-is-there-no-64-bit-version.aspx)
the 32-bit version of K may be the better (more efficient) one (unless your use case includes editing > 2GB files), but given STL's removal of support for 32-bit MinGW, we will "follow suit."  And of course, Linux in 2014+ is almost universally 64-bit (and works fine).

[README Markdown Syntax Reference](https://confluence.atlassian.com/display/STASH/Markdown+syntax+guide)
