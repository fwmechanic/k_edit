//
// Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include <pcre2.h>

//----------- CompiledRegex
// For simple Regex string searches (vs. search-thru-file-until-next-match) ops, use Regex_Compile + CompiledRegex::Match
class CompiledRegex {
   NO_ASGN_OPR(CompiledRegex);
   NO_COPYCTOR(CompiledRegex);
private:
   pcre2_code        *d_pcreCode;
   pcre2_match_data  *d_pcreMatchData;
public:
   // User code SHOULD NOT call this ctor, _SHOULD_ CREATE CompiledRegex via Regex_Compile!
   CompiledRegex( pcre2_code *pcreCode, pcre2_match_data *pcreMatchData )  // called ONLY by Regex_Compile (when it is successful)
      : d_pcreCode(pcreCode)
      , d_pcreMatchData( pcreMatchData )
      {
      }
   ~CompiledRegex() {
      pcre2_code_free( d_pcreCode );
      pcre2_match_data_free( d_pcreMatchData );
      }
   RegexMatchCaptures::size_type Match( RegexMatchCaptures &captures, stref haystack, COL haystack_offset, int pcre_exec_options );
   };

void PCRE_API_INIT() {
   STATIC_VAR BoolOneShot first;
   if( first ) {
      }
   }

stref RegexVersion() {
   STATIC_VAR char s_RegexVer[31] = { '\0', };
   if( '\0' == s_RegexVer[0] ) {
      char pcre2_version[25];
      pcre2_config( PCRE2_CONFIG_VERSION, pcre2_version );   1 && DBG( "PCRE version '%s'", pcre2_version );
      safeSprintf( BSOB(s_RegexVer), "PCRE %s", pcre2_version );
      }
   return s_RegexVer;
   }

//------------------------------------------------------------------------------

int DbgDumpCaptures( RegexMatchCaptures &captures, PCChar tag ) {
   auto ix(0);
   for( const auto &el : captures ) {
      DBG( "%s[%d] '%" PR_BSR "'", tag, ix++, BSR(el.valid() ? el.value() : stref("-1")) );
      }
   return 1;
   }

RegexMatchCaptures::size_type CompiledRegex::Match( RegexMatchCaptures &captures, stref haystack, COL haystack_offset, int pcre_exec_options ) {
   0 && DBG( "CompiledRegex::Match called!" );
   captures.clear(); // before any return
   // http://www.pcre.org/original/doc/html/pcreapi.html#SEC17  "MATCHING A PATTERN: THE TRADITIONAL FUNCTION" describes pcre_exec()
   const int rc( pcre2_match(
           d_pcreCode
         , reinterpret_cast<PCRE2_SPTR>( haystack.data() )
         , haystack.length()   // length
         , haystack_offset     // startoffset
         , pcre_exec_options   // options
         , d_pcreMatchData
         , nullptr             // pcre2_match_context *  (nullptr == use default behaviors)
         )
      );                                                   0 && DBG( "CompiledRegex::Match returned %d", rc );
   if( rc <= 0 ) {
      switch( rc ) {
         break;case PCRE2_ERROR_NOMATCH:  // the only "expected" error: be silent
         break;default: {
                        uint8_t errMsg[150];
                        if( PCRE2_ERROR_BADDATA == pcre2_get_error_message( rc, BSOB(errMsg) ) ) {
                           ErrorDialogBeepf( "pcre2_match returned unknown error %d", rc );
                           }
                        else {
                           ErrorDialogBeepf( "pcre2_match returned '%s'?", errMsg );
                           }
                        }
         }
      }
   else {                                                  0 && DBG( "CompiledRegex::Match count=%d", rc );
      // The first pair of integers, ovector[0] and ovector[1], identify the portion of the subject string matched by
      // the entire pattern (Perl's $0).  The next pair is used for the first capturing subpattern (Perl's $1), and so
      // on.  The value returned by pcre2_match() is one more than the highest numbered pair that has been set.  For
      // example, if two substrings have been captured, the returned value is 3.  If there are no capturing subpatterns,
      // the return value from a successful match is 1, indicating that just the first pair of offsets has been set.
      //
      const auto ovector_els = pcre2_get_ovector_count( d_pcreMatchData );
      captures.reserve( ovector_els );
      auto ovector = pcre2_get_ovector_pointer( d_pcreMatchData );
      for( std::remove_const_t<decltype(ovector_els)> ix{0}; ix < ovector_els; ++ix ) {
         const auto oFirst    = *ovector++;
         const auto oPastLast = *ovector++;
         // It is possible for an capturing subpattern number n+1 to match some part of the subject when subpattern n
         // has not been used at all.  For example, if the string "abc" is matched against the pattern (a|(z))(bc)
         // subpatterns 1 and 3 are matched, but 2 is not.  When this happens, both offset values corresponding to the
         // unused subpattern are set to PCRE2_UNSET.
         //
         if( oFirst == PCRE2_UNSET && oPastLast == PCRE2_UNSET ) {
            captures.emplace_back();
            }
         else {
            captures.emplace_back( oFirst, stref( haystack.data() + oFirst, oPastLast - oFirst ) );
            }
         }
      }
   return captures.size();
   }

RegexMatchCaptures::size_type Regex_Match( CompiledRegex *pcr, RegexMatchCaptures &captures, stref haystack, COL haystack_offset, int pcre_exec_options ) {
   return pcr->Match( captures, haystack, haystack_offset, pcre_exec_options );
   }

CompiledRegex *Regex_Delete0( CompiledRegex *pcr ) {
   delete pcr;
   return nullptr;
   }

CompiledRegex *Regex_Compile( stref pszSearchStr, bool fCase ) {  0 && DBG( "Regex_Compile! %" PR_BSR, BSR(pszSearchStr) );
   PCRE_API_INIT();
   const int options( fCase ? 0 : PCRE2_CASELESS );
   int errCode;
   PCRE2_SIZE errOffset;
   auto pcreCode( pcre2_compile( reinterpret_cast<PCRE2_SPTR>(pszSearchStr.data()), pszSearchStr.length(), options, &errCode, &errOffset, nullptr ) );
   if( !pcreCode ) {
      uint8_t errMsg[150];
      if( PCRE2_ERROR_BADDATA == pcre2_get_error_message( errCode, BSOB(errMsg) ) ) {
         Msg( "pcre2_compile returned unknown error %d", errCode );
         return nullptr;
         }
      0 && DBG( "Regex_Compile! Display_hilite_regex_err" );
      Display_hilite_regex_err( reinterpret_cast<PCChar>(errMsg), pszSearchStr, errOffset );
      return nullptr;
      }
   auto pcreMatchData = pcre2_match_data_create_from_pattern( pcreCode, nullptr );
   if( !pcreMatchData ) {
      pcre2_code_free( pcreCode );
      Msg( "pcre2_match_data_create_from_pattern returned NULL" );
      return nullptr;
      }
   return new CompiledRegex( pcreCode, pcreMatchData );
   }

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
   AllocArrayNZ( rv, 1 );
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
   while( p < end ) {
      const char *q;
      for (q = p; q < end && *q != '%'; ++q)
         {}
      if( q != p )            { Cat( p, q - p ); }
      if( q < end ) {
         if( ++q < end ) { // skip %
            if( isdigit(*q) ) { Cat( *q - '0' ); }
            else              { Cat( q, 1 );     }
            }
         p = q + 1;
         }
      else { break; }
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
