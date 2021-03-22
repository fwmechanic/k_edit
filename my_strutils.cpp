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

#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <algorithm>
#include <string>
#include "my_types.h"
#include "my_strutils.h"
#include "my_log.h"

//-------------------------------------------------------------------------------------------------------

void chkdVsnprintf( PChar buf, size_t bufBytes, PCChar format, va_list val ) {
   buf[ bufBytes - 1 ] = '\0';
   const auto rv( WL( vsnprintf, vsnprintf )( buf, bufBytes, format, val ) );
   if( rv == -1 || buf[ bufBytes - 1 ] != 0 ) {
      buf[ bufBytes - 1 ] = '\0';
      STATIC_CONST char fmt[] = "%s: STRING TRUNCATED: '%s'";
   // STATIC_VAR bool alerted;
   // if( !alerted )  ConIO::DbgPopf( fmt, __func__, buf );
   // else
                      DBG(     fmt, __func__, buf );
   // alerted = true;
      }
   }

STATIC_FXN void StrTruncd_( PCChar fxnm, int truncd, stref src, stref dst ) {
   // cd "$(cygpath -u "$K_LOGDIR")" && grep -F 'STR-TRUNC' *
   DBG( "%s: STR-TRUNC by %d chars..."   , fxnm, truncd );
   DBG( "%s: STR-TRUNC src '%" PR_BSR "'", fxnm, BSR(src) );
   DBG( "%s: STR-TRUNC dst '%" PR_BSR "'", fxnm, BSR(dst) );
   }

#define  StrTruncd( truncd, src, dst )  StrTruncd_( __func__, truncd, src, dest );

PChar safeSprintf( PChar dest, size_t sizeofDest, PCChar format, ... ) {
   va_list val;
   va_start(val, format);
   chkdVsnprintf( dest, sizeofDest, format, val );
   va_end(val);
   return dest;
   }

sridx scat( PChar dest, size_t sizeof_dest, stref src, size_t destLen ) {
   if( 0==destLen && sizeof_dest > 0 && dest[0] ) { destLen = Strlen( dest ); }
   size_t truncd( 0 );
   auto cpyLen( src.length() );
   if( destLen + cpyLen + 1 > sizeof_dest ) {
      truncd = cpyLen - (sizeof_dest - destLen - 1);
      cpyLen = sizeof_dest - destLen - 1;
      }
   if( cpyLen > 0 ) {
      memcpy( dest + destLen, src.data(), cpyLen );
      dest[ destLen + cpyLen ] = '\0';
      }
   if( truncd ) {
      StrTruncd_( __func__, truncd, src.data(), dest );
      }
   return destLen + cpyLen;
   }

sridx scpy( PChar dest, size_t sizeof_dest, stref src ) {
   auto truncd( 0 );
   const auto fullCpyChars( src.length()+1 );
   const auto destCharsToWr( std::min( sizeof_dest, fullCpyChars ) );
   if( fullCpyChars > destCharsToWr ) {
      truncd = fullCpyChars - destCharsToWr;
      }
   if( destCharsToWr > 1 ) {
      memcpy( dest, src.data(), destCharsToWr-1 );
      }
   if( destCharsToWr > 0 ) {
      dest[ destCharsToWr-1 ] = '\0';
      }
   if( truncd > 0 ) {
      StrTruncd_( __func__, truncd, src.data(), dest );
      }
   return destCharsToWr > 0 ? destCharsToWr-1 : 0;
   }

int strnicmp_LenOfFirstStr( stref s1, stref s2 )                   { return Strnicmp( s1.data(), s2.data(), s1.length()         ); }
int strncmp_LenOfFirstStr ( stref s1, stref s2 )                   { return strncmp ( s1.data(), s2.data(), s1.length()         ); }

#define Strnspn_def( fxnm, EQ_OP )                           \
size_t fxnm  ( PCChar str1, PCChar eos1, PCChar needle ) {   \
   if( !eos1 ) { eos1 = Eos( str1 ); }                       \
   const size_t len1( eos1 - str1 );                         \
   for( size_t ix(0) ; ix < len1 ; ++ix ) {                  \
      if( nullptr EQ_OP strchr( needle, str1[ix] ) ) {       \
         return ix;                                          \
         }                                                   \
      }                                                      \
   return len1;                                              \
   }

Strnspn_def( Strnspn , == ) // returns: The length of the initial part of str1 containing        only characters that appear in needle.
Strnspn_def( Strncspn, != ) // returns: The length of the initial part of str1 containing none of the characters that appear in needle.

#undef Strnspn_def

// "runtime support function" for "StrToPrev...OrNull()" family
//
// Interface SHALL:
// 1) if pX == 0: pX = p0 + Strlen(p0);
// 2) *pX (for initial value of pX) is NOT considered for a match
// 3) NULL is returned if NO prev match of 'toMatch' is found
//
// StrPastPrevOrNull_ identical to StrToPrevOrNull_ except strchr has a '!' in front of it
//
// returns NULL if ALL chars preceding pX match 'toMatch'

#define StrToPastPrevOrNull_def( fxnm, EQ_OP )                  \
PCChar fxnm( PCChar p0, PCChar pX, PCChar toMatch ) {           \
   if( !pX ) { pX = Eos( p0 ); }                                \
   if( pX == p0 ) { return nullptr; } /* no "prev" to match? */ \
   for( --pX ;  ; --pX ) {                                      \
      if( nullptr EQ_OP strchr( toMatch, *pX ) ) { return pX; } \
      if( pX == p0 ) { return nullptr; }                        \
      }                                                         \
   }

StrToPastPrevOrNull_def( StrToPrevOrNull_  , != ) // do NOT call this directly!  use StrToPrevOrNull or StrToPrevBlankOrNull instead!

#undef StrToPastPrevOrNull_def

int Quot2_strcspn( PCChar pszToSearch, PCChar pszToSearchFor ) {
   const auto p0( pszToSearch );
   auto fInQuot2dStr(false);
   char ch;
   for( ; (ch=*pszToSearch); ++pszToSearch ) {
      const auto isQuot2( ch == chQuot2 && (pszToSearch > p0) && (pszToSearch[-1] != chESC) );
      if( fInQuot2dStr ) {
         if( isQuot2 ) {
            fInQuot2dStr = false;
            }
         }
      else {
         if( isQuot2 ) {
            fInQuot2dStr = true;
            }
         else {
            if( strchr( pszToSearchFor, ch ) ) {
               return pszToSearch - p0;
               }
            }
         }
      }
   return pszToSearch - p0; // pszToSearch[ retVal ] == 0
   }

//
// snprintf_full: use to concatenate multiple formatted strings to a
// fixed-len buffer without performing repeated strlen's.
// returns NZ if buffer was partially (including _not_) written, else Z
//
int snprintf_full( char **ppBuf, size_t *pBufBytesRemaining, PCChar fmt, ... ) {
    // if prev call EXACTLY filled the buffer
    // then (*pBufBytesRemaining) == 1 !!!
    if( *pBufBytesRemaining <= 1 ) {
       return 1;
       }
    va_list ap;
    va_start( ap, fmt );
    const int chars_wr = vsnprintf( *ppBuf, *pBufBytesRemaining, fmt, ap );
    va_end( ap );
    //
    //           DON'T BELIEVE EVERY MAN PAGE YOU READ!
    //
    // From RedHat 7.1 Linux 'man vsnprintf' page:
    //
    //   "...  snprintf and vsnprintf do not write more than size bytes
    //   (including the trailing '\0'), and return -1 if the output was
    //   truncated due to this limit."
    //
    // That's probably why chars_wr was 79 when trying to print a long string
    // when *pBufBytesRemaining == 10 !!!
    //
    // OTOH, OpenBSD's online man page reflects observed reality (on our Linux
    // system):
    //
    //   "...  snprintf() and vsnprintf(), which return the number of characters
    //   that would have been printed if the size were unlimited (again, not
    //   including the final `\0').  If an output or encoding error occurs, a
    //   value of -1 is returned instead."
    //
    // Caveat reader!
    //
    if( chars_wr == -1 || chars_wr+1 > *pBufBytesRemaining ) {
       (*ppBuf)[(*pBufBytesRemaining)-1] = '\0';
       *ppBuf += *pBufBytesRemaining; // now points PAST END of buffer!
       *pBufBytesRemaining = 0;
       return 1;
       }
    *ppBuf              += chars_wr;
    *pBufBytesRemaining -= chars_wr;
    return *pBufBytesRemaining <= 1;
    }

// stref StrSpnSignedInt( stref src ) {
//    if( src.empty() ) { return src; }
//    auto signofs( src[0] == '-' || src[0] == '+' ? 1 : 0 );
//    auto declook( src.substr( signofs ) );
//    const auto ixPast( declook.find_first_not_of( "0123456789" ) );
//    if( ixPast == stref::npos ) { return ""; }
//    return src.substr( 0, signofs + ixPast );
//    }

bool StrSpnSignedInt( PCChar pszString ) {
   if( *pszString == '-' || *pszString == '+' ) {
      ++pszString;
      }
   return *StrPastAny( pszString, "0123456789" ) == 0;
   }

typedef int (*isfxn)(int);

STATIC_FXN sridx consec_is_its( isfxn ifx, stref sr ) {
   auto rv( 0 );
   for( auto ch : sr ) {
      if( !ifx( ch ) ) {
         break;
         }
      ++rv;
      }
   return rv;
   }

sridx consec_xdigits( stref sr ) { return consec_is_its( isxdigit, sr ); }
sridx consec_bdigits( stref sr ) { return consec_is_its( isbdigit, sr ); }

STATIC_FXN stref conv_u_( int &errno_, uintmax_t &rv, stref sr, UI &numberBase ) {
   stref v2v( "0123456789abcdefghijklmnopqrstuvwxyz" );
   rv = 0;
   if( numberBase < 2 || numberBase > v2v.length() ) { // unsupported numberBase value?
ERR_NOTHING:
      errno_ = EDOM; // "Mathematics argument out of domain of function."  http://pubs.opengroup.org/onlinepubs/000095399/basedefs/errno.h.html
      return stref( sr.data(), 0 );
      }
   auto isBasePrefix = [&numberBase]( UI bs, char xfix, typeof(sr.cbegin()) &it, typeof(sr.cbegin()) itend ) {
      if( (  10 == numberBase  // default?
          || bs == numberBase  // explicit bs?
          ) && (*it=='0')
        ) {
         const auto it1( it + 1 );
         if( it1 != itend && (*it1==tolower(xfix)||*it1==toupper(xfix)) ) {
            numberBase = bs;
            it = it1;
            return true;
            }
         }
      return false;
      };

   sridx oFirst( stref::npos ), oLast( stref::npos );
   for( auto it( sr.cbegin() ) ; it != sr.cend() ; ++it ) {
      if( oFirst == stref::npos ) { // leading ...
         if( isblank(*it) ) { // ? skip
            continue;
            }
         if(  isBasePrefix( 16, 'x', it, sr.cend() )
           || isBasePrefix(  2, 'b', it, sr.cend() )
           ) {
            oFirst = std::distance( sr.cbegin(), it + 1 ); // nb: if found, it has been advanced; we advance further
            continue;
            }
         }
      const auto chVal( v2v.find( tolower(*it) ) );
      if( chVal == stref::npos || chVal > numberBase-1 ) { // not blank and not valid char in numberBase
         if( oFirst == stref::npos ) { // seen NO valid chars in numberBase?
            goto ERR_NOTHING;
            }
         break;
         }
      // *it (chVal) is valid in numberBase
      oLast = std::distance( sr.cbegin(), it );
      if( oFirst == stref::npos ) {
         oFirst = oLast;
         }
      const auto rv0( rv );
      rv = (rv * numberBase) + chVal;
      if( rv0 > rv ) { // result overflows ull
         errno_ = ERANGE; // EOVERFLOW while defined is not mandatory so ERANGE used
         goto SOMETHING;
         }
      }
   if( oFirst == stref::npos ) { // no valid chars in numberBase in sr (prior to an INvalid char)
      goto ERR_NOTHING;
      }
SOMETHING: // not necessarily an error
   return stref( sr.data() + oFirst, oLast - oFirst + 1 );
   }

std::tuple<int, uintmax_t, stref, UI> conv_u( stref sr, UI numberBase ) { enum { DB=0 }; // DBG wrapper
   auto errno_( 0 ); uintmax_t rslt( 0 ); auto baseActual( numberBase );
   const auto srused( conv_u_( errno_, rslt, sr, baseActual ) );
   DB && DBG( "conv_u: '%" PR_BSR "': e=%d '%" PR_BSR "' rv=%ju ", BSR(sr), errno_, BSR(srused), rslt );
   return std::make_tuple( errno_, rslt, srused, baseActual );
   }

// from http://www.stereopsis.com/strcmp4humans.html
STIL int parsedecnum( PCChar & pch ) {
   auto result( *pch - '0' );
   ++pch;
   while( isdigit(*pch) ) {
      result *= 10;
      result += *pch - '0';
      ++pch;
      }
   --pch;
   return result;
   }

int strcmp4humans( PCChar pA, PCChar pB ) {
   if( pA == pB      ) { return  0; }
   if( pA == nullptr ) { return -1; }
   if( pB == nullptr ) { return  1; }
   for ( ; *pA && *pB ; ++pA, ++pB ) {
      const auto a0( isdigit(*pA) ? parsedecnum(pA) + 256 :  tolower(*pA) );  // will contain either a number or a letter
      const auto b0( isdigit(*pB) ? parsedecnum(pB) + 256 :  tolower(*pB) );  // will contain either a number or a letter
      if( a0 < b0 ) { return -1; }
      if( a0 > b0 ) { return  1; }
      }
   if( *pA ) { return  1; }
   if( *pB ) { return -1; }
   return 0;
   }

#if !defined(_WIN32)

PChar _strupr( PChar buf ) {
   const auto rv( buf );
   for( ; *buf ; ++buf ) {
      if( islower( *buf ) ) {
         *buf = toupper( *buf );
         }
      }
   return rv;
   }

PChar _strlwr( PChar buf ) {
   const auto rv( buf );
   for( ; *buf ; ++buf ) {
      if( isupper( *buf ) ) {
         *buf = tolower( *buf );
         }
      }
   return rv;
   }

#endif

sridx FirstNonBlankOrEnd( stref src, sridx start ) {
   return ToNextOrEnd( notBlank, src, start );
   }

sridx FirstBlankOrEnd( stref src, sridx start ) {
   return ToNextOrEnd( isblank , src, start );
   }

stref lineIterator::next() {
   if( d_remainder.empty() ) { return d_remainder; }
   auto len( d_remainder.find_first_of( "\n\r" ) );
   const auto rv( d_remainder.substr( 0, len ) );
   d_remainder.remove_prefix( rv.length() );
   if( !d_remainder.empty() ) { // d_remainder[0] === '\n' or '\r'
      auto toRmv( 1 );
      // accommodate all possible EOL sequences:
      // Windows => "\x0D\x0A".
      // UNIX    => "\x0A"
      // MacOS   => "\x0A\x0D"
      // ???     => "\x0D"
      if( d_remainder.length() > 1 &&
            (d_remainder[0] == '\n' && d_remainder[1] == '\r')
         || (d_remainder[0] == '\r' && d_remainder[1] == '\n')
        ) { ++toRmv; }
      d_remainder.remove_prefix( toRmv ); // skip logical newline (may be one or two characters)
      }                                              0 && DBG( "next: '%" PR_BSR "'", BSR(rv) );
   return rv;
   }
