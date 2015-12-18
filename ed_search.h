//
// Copyright 2015 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#ifndef USE_PCRE
#error  USE_PCRE not defined
#endif

enum HaystackHas { STR_HAS_BOL_AND_EOL, STR_MISSING_BOL, STR_MISSING_EOL }; // so we can tell PCRE whether the searched string can match ^ or $

#if USE_PCRE

#define PCRE_STATIC
#include "pcre.h"

//----------- RegEx
// For simple Regex string searches (vs. search-thru-file-until-next-match) ops, use RegexCompile + Regex::Match
class Regex {
   NO_ASGN_OPR(Regex);
   NO_COPYCTOR(Regex);

public:

   // a vector of Capture (capture_container) is used to collect and return captures
   // a vector of stref is not used since a matching regex capture can be the empty string
   // which cannot be distinguished from a non-matching capture w/o extra info; d_valid is that
   class Capture { //
      bool  d_valid;
      stref d_value;
   public:
      Capture()             : d_valid( false ), d_value(    ) {}
      Capture( stref val_ ) : d_valid( true  ), d_value(val_) {}
      bool  valid() const { return d_valid; }
      stref value() const { return d_value; }
      };
   typedef std::vector<Capture> capture_container;

private:

   struct pcreCapture {
      int oFirst    = -1;
      int oPastLast = -1;
      bool NoMatch() const { return oFirst == -1 && oPastLast == -1; }
      int Len() const { return oPastLast - oFirst; }
      };

   pcre       *d_pPcre;
   pcre_extra *d_pPcreExtra;
   const int   d_maxPossCaptures;
   std::vector<pcreCapture> d_pcreCapture;

public:

   // User code SHOULD NOT call this ctor, _SHOULD_ CREATE Regex via RegexCompile!
   Regex( pcre *pPcre, pcre_extra *pPcreExtra, int maxPossCaptures ); // called ONLY by RegexCompile (when it is successful)
   ~Regex();

   int MaxPossCaptures() const { return d_maxPossCaptures; }

   capture_container::size_type Match( capture_container &captures, stref haystack, COL haystack_offset, HaystackHas haystack_has );
   };

extern Regex *RegexCompile( PCChar pszSearchStr, bool fCase );
extern void   RegexDestroy( Regex *pRe );

extern void register_atexit_search();

#else

#define register_atexit_search()

#endif
