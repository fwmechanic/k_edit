//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

#ifndef USE_PCRE
#error  USE_PCRE not defined
#endif

class CapturedStrings {
   NO_ASGN_OPR(CapturedStrings);
   NO_COPYCTOR(CapturedStrings);

   protected:

   const int         d_maxCount;
   PXbuf             d_pCs;

   public:

   CapturedStrings( int cnt=1 )
      : d_maxCount(cnt)
      , d_pCs( new Xbuf [cnt] )
      {}
   ~CapturedStrings() { delete [] d_pCs; }

   // on the accessor side, we do not differentiate between an empty capture and
   // a nonexistent capture (both have Len()==0, and Str()=="")
   //
   PCChar Str( int ix ) const { return (ix < d_maxCount) ? PCChar(d_pCs[ix].c_str()) : ""; }
   int    Len( int ix ) const { return (ix < d_maxCount) ?        d_pCs[ix].length()   : 0 ; }
   int    Count()       const { return       d_maxCount; }

   void clear() {
      for( auto ix(0); ix < d_maxCount; ++ix )
         d_pCs[ix].clear();
      }
   void Set( int ix, PCChar src, int srcLen ) {
      if( ix < d_maxCount )
         d_pCs[ix].assign( src, srcLen );
      }

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
      int oFirst;
      int oPastLast;
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
   };  STD_TYPEDEFS( Regex )

//******************************************************************************

//----------- RegEx

extern void register_atexit_search();

// For simple Regex string searches (vs. search-thru-file-until-next-match) ops, use RegexCompile + Regex::Match

extern   PRegex RegexCompile( PCChar pszSearchStr, bool fCase );
extern   void   RegexDestroy( PRegex pRe );

#else

#define register_atexit_search()

#endif
