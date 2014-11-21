K is my personal Win32 programmer's text editor, whose design is derived
from Microsoft's "M" editor which was derived from the "Z" editor.

Features:
========

"Reverse-polish" user-command mode wherein the command-argument ("arg") is
provided (using various selection or data-entry modes) before the command is
selected, and the command's execution behavior adapts to the type of argument
received.

Can switch between line and box (column) selection mode simply by varying the
shape of the selection.

Infinite undo/redo.

K has no "project files" (although I'm starting to consider something like
them); instead K can perform multi-file greps (mfgrep) targeting a set of files
named in another file.  K supports powerful recursive (tree) directory scanning
with output to pseudofile, so, when combined with file-filtering edfuncs such
as grep, strip, etc.  it's easy to quickly construct a buffer containing only
the names of all of the files of interest to you.  And since this is based on
current dir-tree content, it's more likely to be complete and correct than a
"project" that has to be maintained.

Limitations
========

K has no "virtual memory" mechanism (as M did); edited files are loaded in
toto into RAM; K WILL CRASH if you attempt to open a file that is larger than
the biggest malloc'able block available to the K process.  I get hit by this
maybe once a year, and it's easy enough to head/tail/grep to chop a huge
(usually some sort of log) file into manageable pieces.  Also, if/when a
64-bit version becomes available, this ceiling will be raised considerably.

Since I'm a native English speaking US native, there is no support for
displaying Unicode/MBCS/etc.  Lately (very rarely) I get hit with problems
related to non-ASCII filenames: when I download music, it has occasionally
happened that file or dir-names contain characters which have to be TRANSLATED
into the charset that K uses.  If I then construct a cmdline to rename said
file/dir, the command will fail because the filename (containing the translated
character instead of the original character) will not exist.  As with the "lack
of VM" limitation, this is rarely annoying, and will only be resolved (a large
undertaking) if it becomes much more annoying to me (which seems unlikely).

Building
========

Prerequisite: I use the [nuwen.net distribution](http://nuwen.net/mingw.html) of MinGW.

K can be built as a 32-bit or 64-bit app.  The 64-bit build is recent (20140209) but
it's working fine so far (updt: on Win7 (targeting a WQXGA (2560x1600) monitor), I get
an assertion failure related to console reads; I have never had this happen on Win 8.x
(but targeting HD+ (1600x900) resolution).  Per
[Visual-Studio-Why-is-there-No-64-bit-Version](http://blogs.msdn.com/b/ricom/archive/2009/06/10/visual-studio-why-is-there-no-64-bit-version.aspx)
the 32-bit version may be the better (more efficient) one (unless your use case includes
editing > 2GB files).

The last nuwen.net MinGW release that supports building 32-bit apps is 10.4
(w/GCC 4.8.1), released 20130801.  All newer releases build 64-bit apps only
(the first 64-bit used to build K is 11.6 (w/GCC 4.8.2)).

The MinGW distro is downloaded as a self-extracting archive.  I decompress the
32-bit version into `c:\_tools` (therefore `c:\_tools\MinGW`; `mingw.bat` assumes
this), while I decompress the 64-bit version into `c:\_tools\MinGW64`;
`mingw64.bat` assumes this).

To build:

    cd K-repo-root
    mingw.bat   & rem run once to put MinGW exes in shell's PATH
    make clean  & rem unnecessary first time
    make -j

To switch build mode between 32-bit and 64-bit:

    make zap    & rem clean plus nuke all lua.exe related

Note that [MinGW gcc non-optionally dyn-links to MSVCRT.DLL](http://mingw-users.1079350.n2.nabble.com/2-Question-on-Mingw-td7578166.html)
which it assumes is already present on any Windows PC.

Release Fileset
========

A release is the minimum fileset needed to run the editor

run `krls outputdirname`  (this is currently broken)

1. outputdirname must already exist.  It may be relative to the cwd.
1. the output of this process also includes a self-extracting "installer"
  exe file (all the installer does is self-extract, so calling it an
  "installer" is a major stretch.

Deployment (a.k.a. "installation")
========

K was designed to be "copy and run" (a "release") anywhere.  I have
successfully run it from network shares and "thumb drives".

Persistent Footprint (a.k.a. spoor)
========

Editor state

 *  files edited (including window/cursor position)
 *  search history
 *  command usage count (history)

is stored in files in `%APPDATA%\Kevins Editor\*`

Debug/Development
========

I use [DebugView](http://technet.microsoft.com/en-us/sysinternals/bb896647.aspx) to capture the output from
the DBG macros which are sprinkled liberally throughout the source code.

The "distro" of MinGW which I use pointedly DOES NOT include GDB.  I only use
a debugger to debug crashes, so I use DrMinGW as a minimalist way of obtaining
a symbolic stack-trace when a crash occurs.  See gnumakefile for instructions
on how to compile K for use with DrMinGW.

Basic Tutorial
========

 * to edit file filename, run `k filename`.  For cmdline invocation help, run `k -h`
 * `exit` (`alt+F4`) exits the editor; the user is prompted to save any dirty files (one by one, or all).
 * `arg` is assigned to `goto` (numeric keypad 5 key with numlock off (the state I always use).  `arg` is used to introduce arguments to other editor functions.
 * `Alt+h` opens a buffer named <CMD-SWI-Keys> containing the runtime settings of the editor:
    * switches with current values (and comments regarding effect).
    * functions with current key assignment (and comments regarding effect).
    * macros with current definition
 * `alt+F2` opens file history buffer; it contents reflect a stack of filenames, current on top
 * `setfile` (`F2`) function is very powerful
    * `arg "name of thing to open" setfile` opens the thing; thing can be file or URL (latter is opened in dflt browser)
    * `setfile` (by itself) switches between two most recently viewed files.
    * `arg arg setfile` saves the current buffer (if dirty) to its corresponding disk file (if one exists)
    * `arg arg arg setfile` saves all dirty buffers to disk
    * `arg "text containing wildcard" setfile` will open a new "wildcard buffer" containing the names of all files matching the wildcard pattern.  If the "text containing wildcard" ends with a '|' character, the wildcard expansion is recursive.  EX: `arg "*.cpp|" setfile` opens a new buffer containing the names of all the .cpp files found in the cwd and its child trees.
 * `tags` (`alt+u`): looks up the identifier under the cursor (or arg-provided if any) and "hyperlinks" to it.  If >1 definition is found, a menu of the choices is offered.
    * the K build invokes `ctags.exe` (Exuberant Tags) to rebuild the tags database after each successful build
    * the set of tags navigated to for a linklist which is traversed via alt+left and alt+right.  Locations hyperlinked from are also added to this list, allowing easy return.
    * those functions appearing in the "Intrinsic Functions" section of <CMD-SWI-Keys> are all methods of `ARG::` and can be tags-looked up (providing the best possible documentation to the user: the source code!).
 * `psearch` (`F3`) and `msearch` (`F4`) are forward and backward text searches respectively.
    * `xsearch` (w/o arg) searches for the next occurrence of the current search key, in the particular direction
 * `grep` (`ctrl+F3`) creates a new buffer containing one line for each line matching the search key.  `gotofileline` (`alt+g`) comprehends this file format, allowing you to hyperlink back to the match in the grepped file.
 * `mfgrep` (`shift+F4`) creates a new buffer containing one line for each line, from a set of files, matching the search key.  The "set of files" is initialized the first time the user invokes the tags function (there are other ways of course).
 * the cursor keys should all work as expected, and serve to extend the arg selection if one is in effect.
 * `resize` (`alt+w`) allows you to interactively resize the screen and change the console font using the numpad cursor keys and those nearby.
 * `ctrl+c` and `ctrl+v` xfr text between the Windows Clipboard and the editor's <clipboard> buffer in (hopefully) intuitive ways.
 * `+` (copy selection into <clipboard>), `-` (cut selection into <clipboard>) and `ins` keys on the numpad are used to move text between locations in buffers.


Historical Notes
========

K is heavily based upon Microsoft's M editor (released as M.EXE for DOS, and
MEP.EXE for OS/2 and Windows NT), which I used starting in 1988.  According to
http://blogs.msdn.com/b/larryosterman/archive/2009/08/21/nineteen-years-ago-today-1990.aspx

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

Until 2012/06, I compiled K using MSVC 7.1 (Free 32-bit command line build
toolset offered by MS in 2003, since withdrawn, replaced by Visual Studio
Express Edition).  While I used these MS build tools, I used WinDbg, part of a
free (as of 2011/07/13) "Debugging Tools for Windows" from MS, to debug crashes.

Anyway, I have no fondness for Visual Studio, nor for installers, so when I
finally [I found a reliable way to obtain MinGW](http://news.ycombinator.com/item?id=4112374)
and didn't have to pay a significant code-size price for doing so (updt: K.exe has gown significantly since then, mostly at the hands of GCC, though adopting `std::string` and other STL bits has doubtless contributed greatly...), I was thrilled!  Since then I have extensively modified the code to take great
advantage of the major generic features of C++11.  As a result, K no longer
compiles with the MSVC 7.1 compiler.

[sucky Markdown Syntax Reference](http://daringfireball.net/projects/markdown/syntax)