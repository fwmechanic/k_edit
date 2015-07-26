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

class CapturedStrings {
   NO_ASGN_OPR(CapturedStrings);
   NO_COPYCTOR(CapturedStrings);

   protected:

   std::vector<std::string> d_Capture;

   public:

    CapturedStrings() {}
   ~CapturedStrings() {}

   // on the accessor side, we do not differentiate between an empty capture and
   // a nonexistent capture (both have Len()==0, and Str()=="")

   void clear() { d_Capture.clear(); }

   void Set( int ix, PCChar src, size_t srcLen ) {
      d_Capture.emplace_back( src, srcLen );
      }

   const std::vector<std::string> &get() const { return d_Capture; }
   };

//******************************************************************************

enum HaystackHas { STR_HAS_BOL_AND_EOL, STR_MISSING_BOL, STR_MISSING_EOL }; // so we can tell PCRE whether the searched string can match ^ or $

#if USE_PCRE

#define PCRE_STATIC
#include "pcre.h"

class Regex {
   NO_ASGN_OPR(Regex);
   NO_COPYCTOR(Regex);

   public:

   enum { MAX_CAPTURES = 40, CAPT_DIVISOR = 2 };

   struct pcreCapture {
      int oFirst    = -1;
      int oPastLast = -1;
      bool NoMatch() const { return oFirst == -1 && oPastLast == -1; }
      int Len() const { return oPastLast - oFirst; }
      };

   private:

   pcre       *d_pPcre;
   pcre_extra *d_pPcreExtra;
   const int   d_maxPossCaptures;
                        // PCRE oddity: it needs last third of this buffer for workspace
   pcreCapture d_capture[ MAX_CAPTURES + ((MAX_CAPTURES+(CAPT_DIVISOR-1))/CAPT_DIVISOR) ];

   public:

   Regex( pcre *pPcre, pcre_extra *pPcreExtra, int maxPossCaptures );
   ~Regex();

   int MaxPossCaptures() const { return d_maxPossCaptures; }

   PCChar Match( COL startingBufOffset, PCChar pBuf, COL validBufChars, COL *matchChars, HaystackHas tgtContent, CapturedStrings *pcs );
   };

//******************************************************************************

//----------- RegEx

extern void register_atexit_search();

// For simple Regex string searches (vs. search-thru-file-until-next-match) ops, use RegexCompile + Regex::Match

extern   Regex *RegexCompile( PCChar pszSearchStr, bool fCase );
extern   void   RegexDestroy( Regex *pRe );

#else

#define register_atexit_search()

#endif
