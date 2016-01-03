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

#include "ed_main.h"
#include "ed_search.h"

#define PCRE_STATIC
#include <pcre.h>

PCRE_EXP_DECL void * pcre_malloc_( size_t bytes ) {
   0 && DBG( "%s: %" PR_SIZET "u bytes", __func__, bytes );
   return AllocNZ_( bytes );
   }

PCRE_EXP_DECL void   pcre_free_( void *p ) {
   return Free_( p );
   }

void PCRE_API_INIT() {
   STATIC_VAR BoolOneShot first;
   if( first() ) {
      0 && DBG( "PCRE version '%s'", pcre_version() );
      // Msg( "loaded PCRE %s", version() ); if last saved SRCH was regex, this
      //    will crash because screen-geom vars are not initialized until after the
      //    saved search strings are loaded
      pcre_malloc = pcre_malloc_;
      pcre_free   = pcre_free_  ;
      }
   }

//------------------------------------------------------------------------------

enum { CAPT_DIVISOR = 2 };
CompiledRegex::CompiledRegex( pcre *pPcre, pcre_extra *pPcreExtra, int maxPossCaptures )
   : d_pPcre(pPcre)
   , d_pPcreExtra(pPcreExtra)
   , d_maxPossCaptures(maxPossCaptures)
   , d_pcreCapture(maxPossCaptures + ((maxPossCaptures+(CAPT_DIVISOR-1))/CAPT_DIVISOR)) // oddity: PCRE needs last third of this buffer for workspace
   {}

CompiledRegex::~CompiledRegex() {
   (*pcre_free)( d_pPcre );
   (*pcre_free)( d_pPcreExtra );
   }

CompiledRegex::capture_container::size_type CompiledRegex::Match( CompiledRegex::capture_container &captures, stref haystack, COL haystack_offset, HaystackHas haystack_has ) {
   0 && DBG( "CompiledRegex::Match called!" );
   const int options
      ( haystack_has == STR_MISSING_BOL ? PCRE_NOTBOL
      : haystack_has == STR_MISSING_EOL ? PCRE_NOTEOL
      :                                   0
      );
   const int rc( pcre_exec(
           d_pPcre
         , d_pPcreExtra
         , haystack.data()
         , haystack.length()
         , haystack_offset
         , options
         , &d_pcreCapture[0].oFirst
         , sizeof(d_pcreCapture) / sizeof(int)
         )
      );
   0 && DBG( "CompiledRegex::Match returned %d", rc );
   captures.clear(); // before any return
   if( rc <= 0 ) {
      switch( rc ) {
         case PCRE_ERROR_NOMATCH: break; // the only "expected" error: be silent
         case 0:  // if PCRE returns 0, it says there were more captures than we gave him space for (sizeof(d_pcreCapture) / sizeof(int)).
                  // This is an internal error.
                  ErrorDialogBeepf( "PCRE: actualPcreCaptureCount(?) < d_pcreCapture.size()(%d)", d_maxPossCaptures );
                  break;
         default: ErrorDialogBeepf( "PCRE returned %d?", rc );
                  break;
         }
      return 0;
      }
   if( rc > d_maxPossCaptures ) {
      ErrorDialogBeepf( "PCRE: rc(%d) > d_maxPossCaptures(%d)", rc, d_maxPossCaptures );
      return 0;
      }
   0 && DBG( "CompiledRegex::Match count=%d", rc );
   if( rc > 0 ) {
      captures.reserve( rc );
      for( auto ix(0); ix < rc; ++ix ) {
         // It is possible for an capturing subpattern number n+1 to match some
         // part of the subject when subpattern n has not been used at all.  For
         // example, if the string "abc" is matched against the pattern
         // (a|(z))(bc) subpatterns 1 and 3 are matched, but 2 is not.  When
         // this happens, both offset values corresponding to the unused
         // subpattern are set to -1.
         //
         if( d_pcreCapture[ix].NoMatch() || d_pcreCapture[ix].Len() < 0 ) {
            captures.emplace_back();
            }
         else {
            captures.emplace_back( d_pcreCapture[ix].oFirst, stref( haystack.data() + d_pcreCapture[ix].oFirst, d_pcreCapture[ix].Len() ) );
            }
         }
      }
   return captures.size();
   }

CompiledRegex *Compile_Regex( PCChar pszSearchStr, bool fCase ) {
   PCRE_API_INIT();
   0 && DBG( "Compile_Regex! %s", pszSearchStr );
   const int options( fCase ? 0 : PCRE_CASELESS );
   PCChar errMsg;
   int errOffset;
   auto re( pcre_compile( pszSearchStr, options, &errMsg, &errOffset, nullptr ) );
   if( !re ) {
      extern void Display_hilite_regex_err( PCChar errMsg, PCChar pszSearchStr, int errOffset );
                  Display_hilite_regex_err(        errMsg,        pszSearchStr,     errOffset );
      return nullptr;
      }
   auto xtra( pcre_study( re, PCRE_STUDY_JIT_COMPILE, &errMsg ) );
   if( errMsg ) {
      (*pcre_free)( re );
      Msg( "Regex study failed: %s", errMsg );
      return nullptr;
      }
   int maxPossCaptures;
   pcre_fullinfo( re, xtra, PCRE_INFO_CAPTURECOUNT, &maxPossCaptures );
   ++maxPossCaptures; // the 0th capture is the WHOLE match
   0 && DBG( "captures: %d", maxPossCaptures );
   return new CompiledRegex( re, xtra, maxPossCaptures );
   }

#if 0

bool ARG::pcre() {
   if( !pcre_LdOk() ) {
      return false;
      }
   PCChar error;
   int erroffset;
   auto re( pcre_compile( "^A.*Z", nullptr, &error, &erroffset, nullptr ) );
   if( !re ) {
      Msg( "compile failed: %s", error );
      return false;
      }
   Msg( "compile OK!" );
   pcreCapture ovector[ MAX_CAPTURES + (MAX_CAPTURES/2) ];
   const char str[] = "ABBAZABZBA";
   auto rc( pcre_exec( re, nullptr, str, Strlen(str), nullptr, 0, ovector, sizeof(ovector) / sizeof(int) ) );
   if( rc < 0 ) {
      // Maybe do something with these someday
      //
      //
      // If pcre_exec() fails, it returns a negative number.  The following are defined
      // in the header file:
      //
      // PCRE_ERROR_NOMATCH (-1)
      //
      //    The subject string did not match the pattern.
      //
      // PCRE_ERROR_NULL (-2)
      //
      //    Either code or subject was passed as nullptr, or ovector was nullptr and
      //    ovecsize was not zero.
      //
      // PCRE_ERROR_BADOPTION (-3)
      //
      //    An unrecognized bit was set in the options argument.
      //
      // PCRE_ERROR_BADMAGIC (-4)
      //
      //    PCRE stores a 4-byte "magic number" at the start of the compiled
      //    code, to catch the case when it is passed a junk pointer.  This is
      //    the error it gives when the magic number isn't present.
      //
      // PCRE_ERROR_UNKNOWN_NODE (-5)
      //
      //    While running the pattern match, an unknown item was encountered in
      //    the compiled pattern.  This error could be caused by a bug in PCRE
      //    or by overwriting of the compiled pattern.
      //
      // PCRE_ERROR_NOMEMORY (-6)
      //
      //    If a pattern contains back references, but the ovector that is
      //    passed to pcre_exec() is not big enough to remember the referenced
      //    substrings, PCRE gets a block of memory at the start of matching to
      //    use for this purpose.  If the call via pcre_malloc() fails, this
      //    error is given.  The memory is freed at the end of matching.
      //
      // PCRE_ERROR_NOSUBSTRING (-7)
      //
      //    This error is used by the pcre_copy_substring(),
      //    pcre_get_substring(), and pcre_get_substring_list() functions (see
      //    below).  It is never returned by pcre_exec().
      //
      // PCRE_ERROR_MATCHLIMIT (-8)
      //
      //    The recursion and backtracking limit, as specified by the
      //    match_limit field in a pcre_extra structure (or defaulted) was
      //    reached.  See the description above.
      //
      // PCRE_ERROR_CALLOUT (-9)
      //
      //    This error is never generated by pcre_exec() itself.  It is provided
      //    for use by callout functions that want to yield a distinctive error
      //    code.  See the pcrecallout documentation for details.
      //
      // PCRE_ERROR_BADUTF8 (-10)
      //
      //    A string that contains an invalid UTF-8 byte sequence was passed as
      //    a subject.
      //

      Msg( "no match" );
      }
   else {
      //
      // Captured substrings are returned to the caller via a vector of integer
      // offsets whose address is passed in ovector.  The number of elements in
      // the vector is passed in ovecsize.  The first two-thirds of the vector
      // is used to pass back captured substrings, each substring using a pair
      // of integers.  The remaining third of the vector is used as workspace by
      // pcre_exec() while matching capturing subpatterns, and is not available
      // for passing back information.  The length passed in ovecsize should
      // always be a multiple of three.  If it is not, it is rounded down.
      //
      // When a match has been successful, information about captured substrings
      // is returned in pairs of integers, starting at the beginning of ovector,
      // and continuing up to two-thirds of its length at the most.  The first
      // element of a pair is set to the offset of the first character in a
      // substring, and the second is set to the offset of the first character
      // after the end of a substring.  The first pair, ovector[0] and
      // ovector[1], identify the portion of the subject string matched by the
      // entire pattern.  The next pair is used for the first capturing
      // subpattern, and so on.  The value returned by pcre_exec() is the number
      // of pairs that have been set.  If there are no capturing subpatterns,
      // the return value from a successful match is 1, indicating that just the
      // first pair of offsets has been set.
      //
      // Some convenience functions are provided for extracting the captured
      // substrings as separate strings.  These are described in the following
      // section.
      //
      // It is possible for an capturing subpattern number n+1 to match some
      // part of the subject when subpattern n has not been used at all.  For
      // example, if the string "abc" is matched against the pattern (a|(z))(bc)
      // subpatterns 1 and 3 are matched, but 2 is not.  When this happens, both
      // offset values corresponding to the unused subpattern are set to -1.
      //
      // If a capturing subpattern is matched repeatedly, it is the last portion
      // of the string that it matched that gets returned.
      //
      // If the vector is too small to hold all the captured substrings, it is
      // used as far as possible (up to two-thirds of its length), and the
      // function returns a value of zero.  In particular, if the substring
      // offsets are not of interest, pcre_exec() may be called with ovector
      // passed as nullptr and ovecsize as zero.  However, if the pattern contains
      // back references and the ovector isn't big enough to remember the
      // related substrings, PCRE has to get additional memory for use during
      // matching.  Thus it is usually advisable to supply an ovector.
      //
      // Note that pcre_info() can be used to find out how many capturing
      // subpatterns there are in a compiled pattern.  The smallest size for
      // ovector that will allow for n captured substrings, in addition to the
      // offsets of the substring matched by the whole pattern, is (n+1)*3.
      //
      Msg( "'%.*s' matched", ovector[0].Len(), str+ovector[0].oFirst );
      }
   return true;
   }

#endif

//
// GenericListEl was developed for encoding regex replacement strings which can
//    include backreferences; an IS_INT variant signifies that "the content of the
//    backreference numbered N" is to be appended (IS_STR values are appended
//    directly).
//
//    Since the implementation is reasonably memory efficient (one alloc per
//    element, even for strings, with additional overhead being only the link
//    pointers plus sizeof(enum) (the minimal extra cost of including the IS_INT
//    variant)), this data struct is useful for general string concatenation as
//    well as its originally-intended purpose.
//
struct GenericListEl {
   DLinkEntry<GenericListEl> dlink;
   enum { IS_INT, IS_STRING } d_typeof;
   union {
      int   num;
      char  str[ sizeof(int) ];
      } value;
   bool   IsStr( const char **ppStr ) const {
      *ppStr = d_typeof == IS_STRING ? value.str : nullptr;
      return   d_typeof == IS_STRING;
      }
   bool   IsInt( int *pInt ) const {
      *pInt = d_typeof == IS_INT ? value.num : 0;
      return  d_typeof == IS_INT;
      }
   };

struct GenericList {
   DLinkHead<GenericListEl> d_head;
   GenericList() {}
   virtual ~GenericList();
   void Cat( int num );
   void Cat( PCChar src, size_t len );
   };

GenericList::~GenericList() {
   while( auto pEl = d_head.front() ) {
      DLINK_REMOVE_FIRST( d_head, pEl, dlink );
      Free0( pEl );
      }
   }

void GenericList::Cat( int num ) {
   GenericListEl *rv;
   AllocArrayNZ( rv );
   rv->dlink.clear();
   rv->d_typeof = GenericListEl::IS_INT;
   rv->value.num = num;
   DLINK_INSERT_LAST(d_head, rv, dlink);
   }

void GenericList::Cat( PCChar src, size_t len ) {
   if( len == 0 ) {
       len = Strlen( src );
       }
   GenericListEl *rv = static_cast<GenericListEl *>( // cannot use auto due to 'sizeof( *rv )'
         AllocNZ_(
           sizeof( *rv )     // needed control struct
         + len+1             // space to store string value
         - sizeof(rv->value) // space within control struct that is used to store string value
         )
      );
   rv->dlink.clear();
   rv->d_typeof = GenericListEl::IS_STRING;
   memcpy( rv->value.str, src, len );
   rv->value.str[len] = '\0';
   DLINK_INSERT_LAST(d_head, rv, dlink);
   }

struct ReplaceWithCaptures : public GenericList {
   ReplaceWithCaptures( PCChar src );
   };

ReplaceWithCaptures::ReplaceWithCaptures( PCChar src ) {
         auto p   ( src );
   const auto end ( src + Strlen(p) );
   while (p < end) {
      const char *q;
      for (q = p; q < end && *q != '%'; ++q)
         {}
      if (q != p)             Cat( p, q - p );
      if (q < end) {
         if( ++q < end ) { // skip %
            if( isdigit(*q) ) Cat( *q - '0' );
            else              Cat( q, 1 );
            }
         p = q + 1;
         }
      else break;
      }
   }

STATIC_FXN void test_rpc( PCChar szRawReplace ) {
   GenericList ResultStr;
   const ReplaceWithCaptures rplc( szRawReplace ); // convert replace string into capture-handling representation
   DLINKC_FIRST_TO_LASTA( rplc.d_head, dlink, pEl ) {
      PCChar str;
      if( pEl->IsStr( &str ) ) {
         // buffer_addlstring (&ResultStr, str, num);
         continue;
         }
      int num;
      if( pEl->IsInt( &num ) ) {
         // if( ALG_SUBVALID (ud,num) ) // backref exists?
         //    buffer_addlstring (&ResultStr, argE.text + ALG_BASE(pos) + ALG_SUBBEG(ud,num), ALG_SUBLEN(ud,num));
         //
         continue;
         }
      }
   }

#if 0

// here's how lua does it ... lua-5.1\src\lpcre.c

#ifdef ALG_USERETRY
   #define SET_RETRY(a,b) (a=b)
#else
   #define SET_RETRY(a,b) ((void)a)
#endif

#ifdef ALG_USERETRY
   STATIC_FXN int gsub_exec (TUserdata *ud, TArgExec *argE, int offset, int retry);
   #define PCRE_EXEC gsub_exec
   STATIC_FXN int gsub_exec (TPcre *ud, TArgExec *argE, int st, int retry) {
      int eflags = retry ? (argE->eflags|PCRE_NOTEMPTY|PCRE_ANCHORED) : argE->eflags;
      return pcre_exec (ud->pr, ud->extra, argE->text, (int)argE->textlen,
        st, eflags, ud->match, (ALG_NSUB(ud) + 1) * 3);
      }
#else
   STATIC_FXN int gsub_exec (TUserdata *ud, TArgExec *argE, int offset);
   #define PCRE_EXEC(a,b,c,d) gsub_exec(a,b,c)
   STATIC_FXN int gsub_exec (TPcre *ud, TArgExec *argE, int st) {
      return pcre_exec (ud->pr, ud->extra, argE->text, (int)argE->textlen,
        st, argE->eflags, ud->match, (ALG_NSUB(ud) + 1) * 3);
      }
#endif

int gsub (lua_State *L) {

   // removed code handling argE.reptype != LUA_TSTRING
   // removed code handling argE.maxmatch == GSUB_CONDITIONAL

   /*------------------------------------------------------------------*/
   TArgComp argC;
   TArgExec argE;
   checkarg_gsub (L, &argC, &argE);

   TUserdata *ud;
   compile_regex (L, &argC, &ud);

   TFreeList freelist;
   freelist_init (&freelist);
   /*------------------------------------------------------------------*/
   const ReplaceWithCaptures rplc( szRawReplace ); // convert replace string into capture-handling representation
   { // check for invalid index
   DLINKC_FIRST_TO_LASTA( rplc.d_head, dlink, pEl ) {
      int intval;
      if( pEl->IsInt( &intval ) && intval > ALG_NSUB(ud) ) {
         // ERROR  "invalid capture index"
         }
      }
   }

   /*------------------------------------------------------------------*/
   GenericList ResultStr;
   buffer_init (&ResultStr, 1024, L, &freelist);
   /*------------------------------------------------------------------*/
   int retry;
   SET_RETRY (retry, 0);
   int n_subst(0);
   int n_match(0);
   int pos(0);
   while ((argE.maxmatch < 0 || n_match < argE.maxmatch) && pos <= (int)argE.textlen) {
      const auto res( PCRE_EXEC( ud, &argE, pos, retry ) );
      if (res == PCRE_ERROR_NOMATCH) {
#ifdef ALG_USERETRY
         if (retry) {
            if (pos < (int)argE.textlen) {  /* advance by 1 char (not replaced) */
               buffer_addlstring (&ResultStr, argE.text + pos, 1);
               ++pos;
               retry = 0;
               continue;
               }
            }
#endif
         break;
         }
      else if( res < 0 ) {
         freelist_free (&freelist);
         return generate_error (L, ud, res);
         }
      ++n_match;
      const auto from( ALG_BASE(pos) + ALG_SUBBEG(ud,0) );
      const auto to  ( ALG_BASE(pos) + ALG_SUBEND(ud,0) );
      if (pos < from) {
         ResultStr.Cat( argE.text + pos, from - pos );
#ifdef ALG_PULL
         pos = from;
#endif
         }

      {
      DLINKC_FIRST_TO_LASTA( rplc.d_head, dlink, pEl ) {
         PCChar str;
         if( pEl->IsStr( &str ) ) {
            ResultStr.Cat( str, num );
            continue;
            }
         int num;
         if( pEl->IsInt( &num ) ) {
            if( ALG_SUBVALID (ud,num) ) // backref exists?
               ResultStr.Cat( argE.text + ALG_BASE(pos) + ALG_SUBBEG(ud,num), ALG_SUBLEN(ud,num) );
            continue;
            }
         }
      }

      ++n_subst;
      if( pos < to ) {
         pos = to;
         SET_RETRY (retry, 0);
         }
      else if (pos < (int)argE.textlen) {
#ifdef ALG_USERETRY
         retry = 1;
#else
         /* advance by 1 char (not replaced) */
         ResultStr.Cat( argE.text + pos, 1 );
         ++pos;
#endif
         }
      else
         break;
      }

   ResultStr.Cat( argE.text + pos, argE.textlen - pos );
   buffer_pushresult (&ResultStr);
   lua_pushinteger (L, n_match);
   lua_pushinteger (L, n_subst);
   freelist_free (&freelist);
   return 3;
   }

#endif
