//
// Copyright 2015-2019 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include <cstdarg>
#include <cstring>
#include <tuple>

#include "my_types.h"
#include "ed_mem.h" // for Strdup()

extern void chkdVsnprintf( PChar buf, size_t bufBytes, PCChar format, va_list val );
#define   use_vsnprintf  chkdVsnprintf

#ifndef WL
   // porting abbreviation tool
   #if defined(_WIN32)
   #   define WL(ww,ll)  ww
   #else
   #   define WL(ww,ll)  ll
   #endif
#endif

// PChar vs PCChar
#define TF_Ptr template<typename Ptr>

#if defined(_WIN32) // Microsoft C Compiler for Win32?
   // use safeSprintf instead of sprintf, _snprintf or snprintf!
   // #define  snprintf   _snprintf
   // #define  vsnprintf  _vsnprintf
   // NB: the above (MSVC RTL *printf fxns) use DIFFERENT format specifiers than MinGW in gnu_printf format mode; see attr_format.h
   //     therefore restoring support for building with MSVC will 100% run afoul of current code using incompatible format specifiers
#endif

// snprintf_full: use to write multiple formatted strings to a fixed-len buffer
// without performing repeated strlen's.
// returns NZ if buffer was partially (including _not_) written, else Z
extern int   snprintf_full( char **ppBuf, size_t *pBufBytesRemaining, PCChar fmt, ... ) ATTR_FORMAT(3, 4);

//--------------------------------------------------------------------------------------------

constexpr char chNUL = '\0';
constexpr char HTAB = '\t';
constexpr char chESC = '\\';
constexpr char chQuot1 = '\'';
constexpr char chQuot2 = '"';
constexpr char chBackTick = '`';
constexpr char chLSQ = '[';
constexpr char chRSQ = ']';
#define        SPCTAB  " \t"

constexpr int MIN_TAB_WIDTH = 1;
constexpr int MAX_TAB_WIDTH = 8;  // we don't support > MAX_TAB_WIDTH cols per tab!

enum {
       TICK     = 0x27,
       BACKTICK = 0x60,
   };

// blank vs. space:
// http://www.cplusplus.com/reference/cctype/isblank/
//
//   A blank character is a space character used to separate words within a line of text.
//
//   The standard "C" locale considers blank characters the tab character ('\t') and the space character (' ').
//
// http://www.cplusplus.com/reference/cctype/isspace/
//
//   For the "C" locale, white-space characters are any of:
//   ' '	(0x20)	space (SPC)
//   '\t'	(0x09)	horizontal tab (TAB)
//   '\n'	(0x0a)	newline (LF)
//   '\v'	(0x0b)	vertical tab (VT)
//   '\f'	(0x0c)	feed (FF)
//   '\r'	(0x0d)	carriage return (CR)

// char predicates; return int to match http://en.cppreference.com/w/cpp/header/cctype is functions

STIL   int   notBlank    ( int ch ) { return !isblank( ch ); }
extern int   isWordChar  ( int ch );
STIL   int   notWordChar ( int ch ) { return !isWordChar( ch ); }
extern int   isHJChar    ( int ch );
STIL   int   notHJChar   ( int ch ) { return !isHJChar( ch ); }
STIL   int   isbdigit    ( int ch ) { return ch == '0' || ch == '1'; }
STIL   int   isQuoteEscCh( int ch ) { return chESC==ch; }

// predicate + extract; return exact char matching a char class (or chNUL)

STIL   char  isQuoteCh   ( char inCh ) { return chQuot2==inCh || chQuot1==inCh ? inCh : chNUL; }

// many stref (and std::string) methods return "index or npos" (the latter
// indicating a "not found" condition); I have chosen to signify a "not found"
// condition with the index equivalent of the cend() iterator value:
STIL sridx nposToEnd(       stref        str, sridx from ) { return from == stref::npos ? str.length() : from; }
STIL bool  atEnd    (       stref        str, sridx idx  ) { return idx == str.length(); }
STIL bool  atEnd    ( const std::string &str, sridx idx  ) { return idx == str.length(); }

// INNER stringref index to OUTER stringref index
STIL sridx isri2osri( stref osr, stref isr, sridx isri ) { return isri + (isr.data()-osr.data()); }

STIL sridx find( stref this_, stref key, sridx start=0 ) {
   this_.remove_prefix( start );
   const auto rv( this_.find( key ) );
   return rv == eosr ? eosr : rv + start ;
   }

template < typename cont_inst, typename Pred >
typename cont_inst::size_type ToNextOrEnd( Pred pred, cont_inst src, typename cont_inst::size_type start ) {
   if( start < src.length() ) {
      for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
         if( pred( *it ) ) {
            return std::distance( src.cbegin(), it );
            }
         }
      }
   return std::distance( src.cbegin(), src.cend() );
   }

// following (ToNext...OrEnd() with key param) exist largely because boost::string_ref find (etc.) methods do not include a start param!

template < typename cont_inst >
typename cont_inst::size_type ToNextOrEnd( const char key, cont_inst src, typename cont_inst::size_type start ) {
   if( start < src.length() ) {
      for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
         if( key == *it ) {
            return std::distance( src.cbegin(), it );
            }
         }
      }
   return std::distance( src.cbegin(), src.cend() );
   }

template < typename cont_inst >
typename cont_inst::size_type ToNextOrEnd( stref key, cont_inst src, typename cont_inst::size_type start ) {
   if( start < src.length() ) {
      for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
         for( const auto kch : key ) {
            if( kch == *it ) {
               return std::distance( src.cbegin(), it );
               }
            }
         }
      }
   return std::distance( src.cbegin(), src.cend() );
   }

template < typename cont_inst >
typename cont_inst::size_type ToNextNotOrEnd( stref key, cont_inst src, typename cont_inst::size_type start ) {
   if( start < src.length() ) {
      for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
         auto hits( 0 );
         for( const auto kch : key ) {
            if( kch == *it ) {
               ++hits;
               }
            }
         if( 0 == hits ) {
            return std::distance( src.cbegin(), it );
            }
         }
      }
   return std::distance( src.cbegin(), src.cend() );
   }

#define SKIP_QUOTED_STR( quoteCh, it, end, EOS_LBL )           \
   if( quoteCh=isQuoteCh( *it ) ) {                            \
      for( ++it ; it != end ; ++it ) {                         \
         if( *it == quoteCh ) {                                \
            break;                                             \
            }                                                  \
         if( isQuoteEscCh( *it ) && ++it == end ) {            \
            goto EOS_LBL; /* [it..end) ends before unquote? */ \
            }                                                  \
         }                                                     \
      }

template < typename cont_inst, typename Pred >
typename cont_inst::size_type ToNextOrEndSkipQuoted( Pred pred, cont_inst src, typename cont_inst::size_type start ) {
   if( start < src.length() ) {
      char quoteCh( chNUL );
      for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
         if( pred( *it ) ) {
            return std::distance( src.cbegin(), it );
            }
         SKIP_QUOTED_STR( quoteCh, it, src.cend(), NO_MATCH )
         }
      }
NO_MATCH:
   return std::distance( src.cbegin(), src.cend() );
   }

template < typename cont_inst >
typename cont_inst::size_type ToNextOrEndSkipQuoted( const int key, cont_inst src, typename cont_inst::size_type start ) {
   if( start < src.length() ) {
      char quoteCh( chNUL );
      for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
         if( key == *it ) {
            return std::distance( src.cbegin(), it );
            }
         SKIP_QUOTED_STR( quoteCh, it, src.cend(), NO_MATCH )
         }
      }
NO_MATCH:
   return std::distance( src.cbegin(), src.cend() );
   }

//#######################################################################################

extern int  strcmp4humans( PCChar s1, PCChar s2 );

extern int  strnicmp_LenOfFirstStr( stref s1, stref s2 );
extern int  strnicmp_LenOfFirstStr( PCChar s1, PCChar s2, int s2chars );
extern int  strncmp_LenOfFirstStr( stref s1, stref s2 );
extern int  strncmp_LenOfFirstStr( PCChar s1, PCChar s2, int s2chars );
extern bool streq_LenOfFirstStr( PCChar s1, int s1chars, PCChar s2, int s2chars );

// extern   stref  StrSpnSignedInt( stref src );
extern   bool   StrSpnSignedInt( PCChar pszString );
extern   std::tuple<int, uintmax_t, stref, UI> conv_u( stref sr, UI numberBase=10 );

extern PCChar  Add_es( int count );
extern PCChar  Add_s(  int count );
extern   int   FlipCase( int ch );
extern PChar   xlatCh( PChar pStr, int fromCh, int toCh );
extern   int   DoubleBackslashes( PChar pDest, size_t sizeofDest, PCChar pSrc );
extern   void  StrUnDoubleBackslashes( PChar pszString );

STIL   bool  StrContainsTabs( stref src )       { return ToBOOL(memchr( src.data(), HTAB, src.length() )); }

STIL bool eq( stref s1, stref s2 ) {
   return s1 == s2;
   }

STATIC_FXN bool eqi_( char ll, char rr ) {
   return std::tolower( ll ) == std::tolower( rr );
   }

STIL bool eqi( stref s1, stref s2 ) {
   if( s1.length() != s2.length() ) {
      return false;
      }
   for( sridx ix( 0 ); ix < s1.length() ; ++ix ) {
      if( !eqi_( s1[ix], s2[ix] ) ) {
         return false;
         }
      }
   return true;
   }

STIL int cmp( int c1, int c2 ) {
   if( c1 == c2 ) { return 0; }
   return c1 < c2 ? -1 : +1;
   }

STIL int cmpi( int c1, int c2 ) { // impl w/highly ASCII-centric optzn taken from http://www.geeksforgeeks.org/write-your-own-strcmp-which-ignores-cases/
   const auto cd( 'a'-'A' );
   if( c1 == c2 || (c1 ^ cd) == c2 ) { return 0; }
   return (c1 | cd) < (c2 | cd) ? -1 : +1;
   }

STIL int cmp( stref s1, stref s2 ) {
   const auto cmplen( std::min( s1.length(), s2.length() ) );
   for( sridx ix( 0 ); ix < cmplen ; ++ix ) {
      const auto rv( cmp( s1[ix], s2[ix] ) );
      if( rv != 0 ) {
         return rv;
         }
      }
   if( s1.length() == s2.length() ) return 0;
   return s1.length() < s2.length() ? -1 : +1;
   }

STIL int cmpi( stref s1, stref s2 ) {
   const auto cmplen( std::min( s1.length(), s2.length() ) );
   for( sridx ix( 0 ); ix < cmplen ; ++ix ) {
      const auto rv( cmpi( s1[ix], s2[ix] ) );
      if( rv != 0 ) {
         return rv;
         }
      }
   if( s1.length() == s2.length() ) return 0;
   return s1.length() < s2.length() ? -1 : +1;
   }

// the Blank family...

STIL bool IsStringBlank( stref src ) { // note that empty strings are Blank strings!
   return std::all_of( src.cbegin(), src.cend(), []( char ch ){ return isblank( ch ); } );
   }

//--------------------------------------------------------------------------------
// to avoid signed/unsigned mismatch warnings:
#if 0
STIL int Strlen( PCChar pS ) { return int( strlen(pS) ); }
#else
STIL int Strlen( register PCChar pc ) { // this MAY be faster than RTL version (which typically uses rep scasb); at least it uses stdcall ...
   const auto start(pc);
   for( ; *pc ; ++pc ) {
      }
   return pc - start;
   }
#endif

TF_Ptr STIL Ptr  Eos( Ptr psz ) { return psz + Strlen( psz ); }

//--------------------------------------------------------------------------------

extern stref scpy( PChar dest, size_t sizeof_dest, stref src );
extern stref scat( PChar dest, size_t sizeof_dest, stref src, size_t destLen=0 );

#define    bcpy( d, s )     scpy( BSOB(d), s )
#define    bcat( l, d, s )  scat( BSOB(d), s, (l) )

extern PChar  safeSprintf( PChar dest, size_t sizeofDest, PCChar format, ... ) ATTR_FORMAT(3,4);

STIL std::string & PadRight( std::string &inout, sridx width, char padCh=' ' ) {
   if( width > inout.length() ) { // trail-pad with spaces to width
      inout.append( width - inout.length(), padCh );
      }
   return inout;
   }

//--------------------------------------------------------------------------------
//
// We have const and non-const versions of each function, so that code can be
// const-correct in all cases.  This used to be done using templates, however
// when not-directly-PChar/-PCChar convertible parameters (e.g.  a Linebuf) were
// passed, they would NOT automtically convert to PChar or PCChar, and we would
// get a crash akin to the ones seen when a Linebuf is passed as a vararg.
//
// 20061021 kgoodwin
//
//-----------------

struct CharMap {
   char disp[257];
   bool is  [256];
   };

extern CharMap g_WordChars;
extern CharMap g_HLJChars;

extern const char szMacroTerminators[];

STIL PCChar Strchr( PCChar psz, int ch ) { return       strchr( psz, ch ); }
STIL PChar  Strchr( PChar  psz, int ch ) { return PChar(strchr( psz, ch )); }

#if !defined(_WIN32)

extern  PChar _strupr( PChar buf );
extern  PChar _strlwr( PChar buf );

#endif

extern sridx consec_xdigits( stref sr );
extern sridx consec_bdigits( stref sr );

STIL int Strnicmp( PCChar string1, PCChar string2, size_t count ) { return WL( _strnicmp, strncasecmp )( string1, string2, count ); }
STIL int Stricmp ( PCChar string1, PCChar string2 )               { return WL( _strcmpi , strcasecmp  )( string1, string2 ); }
STIL int Strcmp  ( PCChar string1, PCChar string2 )               { return      strcmp                 ( string1, string2 ); }
STIL int rb_strcmpi( PCVoid p1, PCVoid p2 ) { return Stricmp( static_cast<PCChar>(p1), static_cast<PCChar>(p2) ); }

extern size_t Strnspn  ( PCChar str1, PCChar eos1, PCChar needle );
extern size_t Strncspn ( PCChar str1, PCChar eos1, PCChar needle );

TF_Ptr STIL Ptr  StrToNextOrEos( Ptr pszToSearch, PCChar pszToSearchFor ) { return pszToSearch + Strncspn( pszToSearch, nullptr, pszToSearchFor ); }
TF_Ptr STIL Ptr  StrPastAny(     Ptr pszToSearch, PCChar pszToSearchFor ) { return pszToSearch + Strnspn ( pszToSearch, nullptr, pszToSearchFor ); }

TF_Ptr STIL Ptr  StrPastAnyBlanks(    Ptr pszToSearch ) { return StrPastAny(     pszToSearch, SPCTAB ); }
TF_Ptr STIL Ptr  StrToNextBlankOrEos( Ptr pszToSearch ) { return StrToNextOrEos( pszToSearch, SPCTAB ); }

// these are extern vs. inline because the requisite predicates are not public
extern sridx FirstNonBlankOrEnd( stref src, sridx start=0 );
extern sridx FirstBlankOrEnd   ( stref src, sridx start=0 );

extern sridx FirstNonWordOrEnd( stref src, sridx start=0 );
extern sridx IdxFirstWordCh( stref src, sridx start=0 );
extern sridx IdxFirstHJCh  ( stref src, sridx start=0 );
extern sridx StrLastWordCh(  stref src );

//-----------------

extern int Quot2_strcspn( PCChar pszToSearch, PCChar pszToSearchFor );

TF_Ptr STIL Ptr Quot2StrToNextOrEos( Ptr  pszToSearch, PCChar pszToSearchFor ) { return pszToSearch + Quot2_strcspn( pszToSearch, pszToSearchFor ); }

TF_Ptr STIL Ptr StrToNextMacroTermOrEos( Ptr pszToSearch ) { return Quot2StrToNextOrEos( pszToSearch, szMacroTerminators ); }

template<typename strlval> void string_tolower( strlval &inout ) { std::transform( inout.begin(), inout.end(), inout.begin(), ::tolower ); }

template<typename strlval>
STIL void trim( strlval &inout ) { // remove leading and trailing whitespace
   const auto ox( inout.find_first_not_of( SPCTAB ) );
   if( ox != eosr ) {
      inout.remove_prefix( ox );
      }
   const auto ix_last_non_white( inout.find_last_not_of( SPCTAB ) );
   const auto dix( inout.length() );
   if( ix_last_non_white != dix-1 ) { // any trailing blanks at all?
      // ix_last_non_white==eosr means ALL are blanks
      const auto rlen( ix_last_non_white==eosr ? dix : dix-1 - ix_last_non_white );
      inout.remove_suffix( rlen );
      }
   }

template<typename strlval>
STIL void unquote( strlval &inout ) { // remove any outer quotes, assuming input has been trimmed
   if( (inout[0]=='\'' || inout[0]=='"') && inout[0]==inout[inout.length()-1] ) {
      inout.remove_suffix( 1 );
      inout.remove_prefix( 1 );
      }
   }

template<typename strlval>
void rmv_trail_blanks( strlval &inout ) {
   while( !inout.empty() && isblank( inout.back() ) ) {
      inout.pop_back();
      }
   }

STIL void rmv_trail_blanks( stref &inout ) {
   auto trailSpcs( 0u );
   for( auto it( inout.crbegin() ) ; it != inout.crend() && *it == ' ' ; ++it ) {
      ++trailSpcs;
      }
   inout.remove_suffix( trailSpcs );
   }

STIL bool chomp( std::string &st ) {
   // return: whether at-entry st was not empty; matters because an (at-entry) legit blank line (containing
   // only newline chars which this fxn removes) and an empty string will both be returned as st.empty()
   if( st.empty() ) {
      return false;
      }
   const auto ixlast( st.length() - 1 );
   if( st[ixlast] == 0x0A ) {
      st.pop_back();
      if( ixlast > 0 && st[ixlast-1] == 0x0D ) {
         st.pop_back();
         }
      }
   return true;
   }

STIL sridx FirstAlphaOrEnd( stref src, sridx start=0 ) { return ToNextOrEnd( isalpha, src, start ); }
STIL sridx FirstDigitOrEnd( stref src, sridx start=0 ) { return ToNextOrEnd( isdigit, src, start ); }

//#######################################################################################

class lineIterator {
   stref d_remainder;
public:
   lineIterator( stref remainder ) : d_remainder( remainder ) {}
   bool empty() const { return d_remainder.empty(); }
   stref next();
   };

template <int elements> class FmtStr {
   char b[ elements ];
   FmtStr() = delete; // no dflt ctor
public:
   FmtStr( PCChar format, ... ) ATTR_FORMAT(2,3) { // this is ambiguous vs 'FixedCharArray( PCChar src )'
      va_list val;
      va_start(val, format);
      use_vsnprintf( b, sizeof(b), format, val );
      va_end(val);
      }
// operator PChar()        { return b; }
   operator PCChar() const { return b; }
   int      Len()    const { return Strlen( b ); }
   PCChar   k_str()  const { return b; }
   PChar    c_str()        { return b; }
   };

STIL void insert_hole( PChar b, size_t sizeof_b, int xCol, int insertWidth=1 )
   { // assumes that last char in b is a chNUL, and preserves it
   memmove( b+xCol+insertWidth, b+xCol, (sizeof_b-1) - (xCol+insertWidth) );
   }

template <size_t elements>
class FixedCharArray {
   char d_buf[ elements ];
public:
   FixedCharArray() { d_buf[0] = chNUL; }
   FixedCharArray( stref src ) { bcpy( d_buf, src ); }
   stref  sr()    const { return stref( d_buf, std::min( elements-1, Strlen( d_buf ) ) ); }
   PCChar k_str() const { return d_buf; }
   PChar  c_str()       { return d_buf; }
   void Vsprintf( PCChar format, va_list val ) { // yes, part of the PUBLIC interface!
      use_vsnprintf( BSOB(d_buf), format, val );
      }
   PCChar Sprintf( PCChar format, ... ) ATTR_FORMAT(2,3) {
      va_list args; va_start(args, format);
      Vsprintf( format, args );
      va_end(args);
      return d_buf;
      }
   PChar SprintfCat( PCChar format, ... ) ATTR_FORMAT(2,3) {
      const auto len( Strlen( d_buf ) );
      if( len < sizeof( d_buf ) - 1 ) {
         va_list args; va_start(args, format);
         use_vsnprintf( d_buf+len, sizeof( d_buf )-len, format, args );
         va_end(args);
         }
      return d_buf;
      }
private:
   // NO_ASGN_OPR(FixedCharArray);
   };

template <sridx elements>
class Catbuf { // purpose: allocate automatic (on-stack) buffers that are safely and efficiently cat- and re-copy-able
   char d_buf[ elements ];
public:
   class wref { // writeable reference to a trailing segment of d_buf
      PChar d_bp;
      sridx d_len;
   public:
      // these are FOR INTERNAL (Catbuf implementation) USE ONLY
      PCChar bp() const { return d_bp; }  // ONLY for stref sr( const wref& wr )
      wref( PChar bp_, sridx len ) : d_bp( bp_ ), d_len( len ) {}
      // only public interface: copy stref into trailing segment of d_buf
      wref cpy( stref src ) const {
         const auto srWr( scpy( d_bp, d_len, src ) );
         const auto ix( (d_bp - srWr.data()) + srWr.length() ); // index of first un-scpy-written char in d_buf
         if( ix < d_len ) {
            return wref{ d_bp + ix, ix - (d_len - 1) };
            }
         return wref{ d_bp, 0 };
         }
      };
   Catbuf() { d_buf[0] = chNUL; } // paranoia
   wref Wref() { return wref{ d_buf, elements }; }
   stref sr( const wref& wr ) const { return stref( d_buf, (wr.bp() - d_buf) ); }
   stref sr()                 const { return stref( d_buf, std::min( elements-1, Strlen( d_buf ) ) ); }
   PCChar k_str() const { return d_buf; }
   PChar  c_str()       { return d_buf; }
   // Test code:
   // Catbuf<5> tbuf;
   // auto t0( tbuf.Wref() );
   // const auto t1( t0.cpy( "filesettings.ftype_map." ) );                  DBG( "%s: t0 = '%s' t1='%" PR_BSR "'", __func__, tbuf.c_str(), BSR(tbuf.sr(t1)) );
   //       auto t2( t1.cpy( stref(d_key) ) );                               DBG( "%s: t1 = '%s' t2='%" PR_BSR "'", __func__, tbuf.c_str(), BSR(tbuf.sr(t2)) );
   };

#include "filename.h"

#include <stdlib.h>

//======================================================================================================
//
// partial generalization of an old C'ism, the sequence of strings exemplified
// by the C RTL environment block: each string is 0-terminated, end of sequence
// is 2 0-terminators in a row.
//
// Useful in cases where the inability to store a string (due to "out of space"
// in the fixed-sized buffer) is not catastrophic, but general case of storing a
// sequence of strings w/ NO heap interaction is appreciated.
//
// sample that adds strings to the sequence
//
//    Reset();                                      // clear the sequence
//    CPCChar wuc = AddKey( snew, snewEos );        // add a string and rtn ptr to it
//    if( !wuc ) {                                  // NULL return is "string doesn't fit"
//       DBG_HL_EVENT && DBG(__func__" toolong");
//       return false;
//       }
//
//    if( (lin >= 0) && (d_wucLen > 2) && 0==strnicmp_LenOfFirstStr( "0x", wuc ) ) {
//       const int xrun( consec_xdigits( wuc+2 ) );
//       if( (d_wucLen==10) && (8==xrun) ) {
//          PCChar key = AddKey( wuc+2 );
//          }
//
// sample that scans the strings in sequence
//
//    for( PCChar pC=Strings() ; *pC ;  ) {
//       const int len( Strlen( pC ) );
//       CPCChar afind = strstr( pCh, pC );
//       if( afind && (!found || (afind < found)) ) { found = afind; mlen = len; }
//       pC += len+1;
//       }
//
// 20110612 kgoodwin
//

template <int elements>
class StringsBuf {
   char  d_buf[elements];
   PChar d_nxtS;
public:
   StringsBuf() {
      clear();
      }
   PCChar data() const { return d_buf; }
   bool find( stref st ) const {
      for( auto pNeedle( d_buf ) ; *pNeedle ;  ) {
         const stref needle( pNeedle );
         if( needle == st ) { return true; }
         pNeedle += needle.length() + 1;
         }
      return false;
      }
   bool empty() const { return d_buf[0] == chNUL; }
   void clear() {
      d_buf[0] = chNUL;
      d_buf[1] = chNUL;  // not ABSOLUTELY necessary, but cheap insurance...
      d_nxtS = d_buf;
      }
   PCChar AddString( stref sr ) {
      const auto len( sr.length() );
      if( len + 2 > sizeof(d_buf) - (d_nxtS-d_buf) ) {
         return "";
         }
      auto rv( d_nxtS );
      memcpy( d_nxtS, sr.data(), len );
       d_nxtS += len;
      *d_nxtS++ = chNUL;
      *d_nxtS   = chNUL;
      return rv;
      }
   };

//--------------------------------------------------------------------------------------------

#define DBG_DiceableString  0
#if DBG_DiceableString
#include "my_log.h"
#endif

class DiceableString {
   // a heap string containing an ascizz string (an ASCII string with TWO (2) trailing NUL chars)
protected:
   PChar d_heapString;
public:
   DiceableString( stref src )
      : d_heapString( Strdup( src, 1 ) ) // tricky: d_heapString is created with 2 trailing NULs
      {}
   void DBG() const;
   virtual ~DiceableString() { Free0( d_heapString ); }
#if DBG_DiceableString
   PCChar GetNext_( PCChar cur ) const {
#else
   PCChar GetNext( PCChar cur ) const {
#endif
      if( !cur ) return *d_heapString ? d_heapString : nullptr;
      const auto rv( Eos( cur ) + 1 );
      return *rv ? rv : nullptr;
      }
#if DBG_DiceableString
   PCChar GetNext( PCChar cur ) const {
      const auto rv( GetNext_( cur ) );
      ::DBG( "rv=%s|", rv );
      return rv;
      }
#endif
   };

//======================================================================================================
