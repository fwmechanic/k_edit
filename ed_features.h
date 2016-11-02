//
// Copyright 2015-2016 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
//
// This file is part of K.
//
// K is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// K is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along
// with K.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

// DBGHILITE is a BOOLEAN (0,1)
#define  DBGHILITE  0
#ifndef  DBGHILITE
#error   DBGHILITE MUST BE DEFINED!
#endif

//-----------------------------------------------------------------------------
//
// defined(fn_argselkeymap) - what's going on here:
//
// When I use a laptop PC, the numeric keypad I rely on for easy/speedy cursor
// movement isn't there, at best replaced by an annoying ersatz replacement.
// This is one place where vi shines: everything can be done (albeit thru the
// use of *MODES*) from the keys between tab and enter.  So I thought, couldn't
// MY editor offer similar functionality for those times when I needed it?
//
// So I cast about for a way of doing this.  First challenge was "how do I let
// the _user_ assign two different functions to the same key?" and "how do I
// display both assignments to the same key?".  Next, "how do I engage this
// mode?"
//
// My first near-implementation: maintain separate PCMD tables for the alpha
// keys ([a-zA-Z]), and swap them with the primary PCMD table on
// mode-enable/-disable.  This would work, but gave no user access/visibility to
// the sel-mode key assignments.  Also, it was a bit kludgy.
//
// So I decided to add new EdKC's for the alpha keys, which would result in the
// primary PCMD table expanding equally.  Both keys would be user-visible at all
// times.  This works well.  The generation (xlation) from the raw to the
// sel-mode EdKC would be done in the key-read "driver" by checking a flag set
// in the arg processing code.  Kinda kludgy, but not too bad.
//
// Deciding when to enter sel-mode was a bit more of a challenge: I foolishly
// thought that engaging sel-mode when argCount > 0 would be sufficient: this
// doesn't work: one case I forgot about is NULLARG graphic (literal arg), which
// should engage GetTextargString with the graphic's value.  Since in defined(fn_argselkeymap)
// compiled code some graphic keys are reassigned to cursor movement functions,
// (1) the trigger to enter GTA mode does not occur when reassigned keys are
// hit, and (2) GetTextargString does not (yet) map alpha keys back to
// non-SEL_KEYMAP values.  #2 can be easily (if kludgily) fixed, but #1 is very
// tough.  Possible fixes for #1:
//
//   a) have two fn_arg variants: fn_argsel and fn_argliteral, which start
//   selection and literally-entered args respectively.
//
//   b) have a mode key which switches from literal-entry mode to SEL_KEYMAP
//   mode: space?  tab?  This is nice because dflt behavior does not change.
//
// I chose option (b) using the space key.
//
// 20161102 switching to option (a) using ctrl+a
//
// Regarding actual key assignments once in sel-mode, there are (at least) two
// choices: the aforementioned vi cursor movement keys, and those of Wordstar
// (also used by early Borland products).
//
// 20051008 klg turned this off as my fingers' "muscle memory" precludes it being useful
//
#define   SEL_KEYMAP  0

#ifdef fn_argselkeymap
   STIL bool SelKeymapEnabled() { extern bool g_fSelKeymapEnabled; return g_fSelKeymapEnabled; }
   STIL void SelKeymapEnable()  { extern bool g_fSelKeymapEnabled; g_fSelKeymapEnabled = true ; }
   STIL void SelKeymapDisable() { extern bool g_fSelKeymapEnabled; g_fSelKeymapEnabled = false; }
#else
   #define   SelKeymapEnabled()  (false)
   #define   SelKeymapEnable()
   #define   SelKeymapDisable()
#endif

//-----------------------------------------------------------------------------

#if defined(_WIN32)
#define  VARIABLE_WINBORDER  1
#else
#define  VARIABLE_WINBORDER  0
#endif
