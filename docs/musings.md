## Musings / Ramblings

OK, so I've been maintaining and evolving K, __while continuously using it DAILY__, 99.95% on my own in one form or another, for __decades__.

### If I had it to do over again, would I?

#### 2017/09

It's hard to second guess.

#### Pros

Having TOTAL CONTROL of my text editing environment has been an enormous boon.
It's impossible to overstate this.  It has been my primary motivation from the
beginning.  The ability to modify it to meet any need I might encounter, and to
be enormously productive w/o having to climb a learning curve each time I switch
jobs, etc., has been a huge benefit to my professional productivity and therefore
my career.

Various peer engineers early in my career (starting in 1990 on the MS-DOS
platform, before open source was _a thing_) chose to commit to closed-source
_commercial_ programmers' text editors:
[Brief](https://en.wikipedia.org/wiki/Brief_(text_editor) ) (DOS) was the big
name in the early 90's; as Windows (and in particular Win32) gained critical
mass, most of my Brief-using peers switched to Codewright (Win16, Win32) or
(many fewer) to Slickedit (Win32), but eventually all of these _commercial_
programmers' text editors were either EOL'd (Brief, Codewright) or were
acquired by a software holding company whose sole purpose was/is to extract
the maximum penance ($$$) from those who had the questionable judgement to
continue using the product (Slickedit).

While I conducted competitive research on Brief, and dallied briefly with
Codewright (tried to write an extension DLL that allowed CW to mimic K's
"reverse-polish" UI), my heart wasn't in it; the unwise aspect of becoming
dependent on a closed-source/commercial tool was already obvious.

Also, CodeWright and Slickedit closely resembled IDEs anyway (huge install,
GUI-access-only features, source-text language-custom features, opaque
project files), further diminished their appeal to me.

##### Case in point: File Open operation

Something I realize was a rudimentary yet key factor in preventing me from
adopting ANY native Windows GUI text editor (e.g.  Codewright, Slickedit) was
their unilateral use (as required by the Microsoft Rules for any Windows GUI
app) of the Windows (OS) "File Open" dialog box ("control"?) to ... select a
file to open.  This "control" is anathema to me: in most cases, it (a)
involves using the mouse; (b) opens with the wrong directory current,
requiring the user to navigate an unknown # of dir levels merely to be in the
correct starting location; thereafter, selecting a particular file is a
process consisting of a "visual search" process (complete with mouse
scrolling hither and yon) followed by clicking on the discovered target.
Altogether an entirely repellant process to someone who has a much simpler
and more productive facility at hand (in K).

Also, most such Windows apps, being opened from a "user-friendly" desktop
icon or start menu, have no concept of (or default to a useless) cwd, which
contributes to their tendency to open a File Open control in a completely
unsuitable directory; this nature also drives users into the figurative arms
of editor-specific "project files" in which the concepts of project cwd are
stored (unnecessarily, IMO).

Whereas K is trivially started from any shell and inherits that shell's
process cwd as its own (this being the basis for all non-absolute
file-read/-write ops) and with its ability to generate "wildcard buffers"
(containing absolute-named files, one per line) which can be searched/grepped
using editor string search functions, opening files whose names appear in
such (or any other) buffers via the `arg setfile` editor command is trivial.

Rephrase: the K philosophy is that the editor cwd is the basis from which
entities which users might expect to be found within a "project file" are
easily/trivially derived dynamically (rather than being statically stored in
the project file, where they are subject to rot and irrelevance vs. the cwd
filesystem content which is the "single source of truth" regarding the
"project").

##### Open Source Alternatives

At no time has it been viable for me to switch away from K to a mainstream
open-source competitor (if I wanted to choose a "fringe open-source
competitor"... well, that's K!):

* I've been forced to use vi/vim (due to it being the default `$EDITOR` for at
least various unices and Linuxes I've encountered over the decades), but (in a
__Windows__ environment, as noted elsewhere being the vast-majority OS choice of
the employers I've had in my career) vi/vim __alone__ (w/o the remainder of the
unix/linux toolset and shell) was (to me) an appallingly bad editing environment
that I could not migrate to w/o taking a massive productivity hit.
* Emacs ... well let's just say "complexity of key
sequences for (what I considered to be) basic operations" presented me with too
steep a learning curve, and w/o a clear payoff/benefit (capability that clearly
exceeded that of K), I was simply never positively motivated to leave K.  The one
aspect of emacs that I wish I hadn't missed out on is emacs' org mode.  But you
can't take the good w/o taking that bad, so I'm fated to miss out on org mode.
I'll live.
* The only other open-source editor I entertained using was [FTE](http://fte.sourceforge.net/ ).
Anyway, I didn't see in FTE anything profoundly superior to K, and it seemed to die (as an actively maintained project) anyway, so...
There seems to be a [recently maintained fork eFTE](https://github.com/lanurmi/efte ), but that occurred years after I was paying serious attention to FTE.

###### IDE's

IDE's (EX: Eclipse) have _never_ interested me.  I like doing things my own way,
and (Java-coded) behemoths created and maintained by "communities" ruled by
management hierarchies, reliant on "project files" and other opaque
near-magical figments causing mysterious behaviors, interest(ed) me not one
iota.  JVM-based apps may be a lot more performance-palatable in 2017 due to
broad increases in PC performance, but _performance_ was a lower-tier
objection I had to the IDE genre anyway.  IMO a text editor should be a _text
editor_, not a real-time language parser for the programming language source
code of the text being edited, that knows how to build the associated
"project".  This last is valuable for some (majority?) of programmers, but
not for me (and was not a feature I ever considered implementing in K).  I
want as few abstractions as possible between me and my toolchain; I don't
want some monolithic tool containing multiple opaque layers of abstractions
taking ownership of any more scope than necessary.  I want to be able to
understand it all...

This attitude has been a problem for me, when various actors have requested
that I "provide an Eclipse (etc) environment" for "my" users.  IMO it's
antithetical (and I resisted doing it); programmers "worth their salt" should
be able to independently setup their own development environment (according
to their own preference, as long as the "interface" between their environment
and their work product meets uniformity requirements). __Programmers who
don't care about their development environment or its customization__ and
rely on others to provide same for them, such that they will accept and align
with a strangers' preference, are [I don't have anything good to say, so I
won't say any more about it].

#### Cons

The only real problem remaining is whether I'll be able to continue use K in my
professional environment "going forward".

Lately I'm really feeling the pressure of increasingly authoritarian
corporate IT security policies that are penetrating the internal operations
of the Product Development Engineering Organizations (PDEO) within which I've
always worked (the vast majority of which have been _Windows-based_ (yes,
this _sucks_)).

IT orgs have a strong tendency toward rigidity: "here is our solution; we won't
change it.  You need to align your problem with our solution."  In the past, PDEO
could resist this imposition with various strategies (and could be convinced of
the need to do so), but what I'm seeing lately is that aggressive pursuit of
operating efficiencies and PDEO management that (amazingly enough) has minimal
appreciation of computing environment nuances and their impact on PDEO
efficiencies, are increasingly willing to kowtow blindly to the corporate IT
party line.  And corporate IT has demonstrated a willingness to follow [the lead
of Rahm Emmanuel in the Obama
administration](https://en.wikiquote.org/wiki/Rahm_Emanuel ):

> You never want a serious crisis to go to waste. ... This crisis provides the opportunity for us to do things that you could not before.

I've resisted IT-mandated (Anti-)Virus software (and other analogously harmfully
invasive mandates from IT: EX: "anti-malware" software), but this is increasingly
a losing battle since the OS chosen to be the platform of the PDEO, Windows (7!
desktop!), is so promiscuous as to be readily susceptible to viruses and malware
in the absence of this invasive garbage (can you say "crisis"?).  The consequence
(_aside from_ 50% of the resources of a given machine being consumed by the
activity of this invasive garbage) is that if I want to run K on a work PC, I
need admin privileges on said PC, in order to be able to create an (Anti-)Virus
software _exception_; w/o this, the (Anti-)Virus software will terminate and
swallow the K.exe with prejudice.  And, given above-mentioned trends, it's
increasingly doubtful whether IT policy will allow me to be granted admin
privileges a few years hence.

So basically, it's a race between my (early) retirement date arriving and the
closing of the corporate PDEO computing environment to any except "standard
tools".  And developer-private tools such as K are at the top of the list of
those that will be locked out.  The alternative is to elevate K to being a
maintained Linux package (what is the Windows analog?  I don't know).  I'm not
sure I have the energy to do this (if only for the reason that for Windows there
is no such thing as a standard package repo (format), so how would I even begin
to create an installable K package for Windows (another thing I absolutely loathe
is Windows Installers (of the executable kind)!).  K already has a "release
package" which can be built: it spits out (on Windows) a 7z archive and a
self-extracting (.exe) version of the same.  The latter is essentially the same
thing as the "packaging" of the Nuwen MinGW GCC toolset.  Will this be acceptable
to corporate IT who may govern my reality?  I have no idea what their policy
might be now, nor what it might become later when they decide to change it.  It
won't have my interests first and foremost anyway, and will be subject to change
at any time w/o notice, so why should I even bother?

### Summary

Yes, it has been a huge amount of work (a multi-decade, near-lifetime project)
to create and maintain K.  Obviously.  But it's a labor of love, and a long-term
investment that has paid off for me.  And the long term investment has been
secured by some major achievements I've made comparatively recently:

1. Switch to compile for Windows using GCC.  This was (is still) enabled by the Nuwen MinGW -x64 GCC package which I discovered some years ago (thanks STL!!!).  Once this was accomplished, it set the stage for
1. Ability to build K as a Linux ncurses app (on Ubuntu Linux).  The ability to build K on the two most dominant OS platforms of the day (and with the number of these declining rathe than increasing) means I (or anyone who might choose) will be able to continue to use K for as long as either platform and the GCC implementation of the C++ language continue to be mainstream platforms.

Does this mean I can "rest on my laurels"?  Well, the only "laurel" is having K available to me, but: YES.
