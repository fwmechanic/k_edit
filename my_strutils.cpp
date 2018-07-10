//
// Copyright 2015-2018 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
   STATIC_CONST char fmt[] = "%s: STRING TRUNCATED by %d chars\nsrc: '%%" PR_BSR "'\ndst: '%%" PR_BSR "'";
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

stref scat( PChar dest, size_t sizeof_dest, stref src, size_t destLen ) {
   if( 0==destLen && sizeof_dest > 0 && dest[0] ) { destLen = Strlen( dest ); }
   auto truncd( 0 );
   auto srcLen( src.length() );
   if( destLen + srcLen + 1 > sizeof_dest ) {
      truncd = srcLen - (sizeof_dest - destLen - 1);
      srcLen = sizeof_dest - destLen - 1;
      }
   auto rv( srcLen );
   if( srcLen > 0 ) {
      memcpy( dest + destLen, src.data(), srcLen );
      dest[ destLen + srcLen ] = '\0';
      rv = destLen + srcLen;
      }
   if( truncd ) {
      StrTruncd_( __func__, truncd, src.data(), dest );
      }
   return stref( dest, rv );
   }

stref scpy( PChar dest, size_t sizeof_dest, stref src ) {
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
   return stref( dest, destCharsToWr > 0 ? destCharsToWr-1 : 0 );
   }

//----- no-longer-used begin

extern int  stricmp_eos( PCChar s1, PCChar eos1, PCChar s2, PCChar eos2 );
extern int  strcmp_eos ( PCChar s1, PCChar eos1, PCChar s2, PCChar eos2 );

#define strcmp_eos_def( fxnm, CMPFX )                        \
int fxnm( PCChar s1, PCChar eos1, PCChar s2, PCChar eos2 ) { \
   const auto l1( eos1-s1 );                                 \
   const auto l2( eos2-s2 );                                 \
   const auto rv( CMPFX ( s1, s2, std::min( l1,l2 ) ) );     \
   if( l1==l2 || rv != 0 ) { return rv; }                    \
   return (l1 > l2) ? +1 : -1;                               \
   }

strcmp_eos_def( stricmp_eos, Strnicmp )
strcmp_eos_def( strcmp_eos , memcmp   )

#undef strcmp_eos_def

//----- no-longer-used end
typedef int slen_t; // should be size_t or sridx
int strnicmp_LenOfFirstStr( stref s1, stref s2 )                   { return Strnicmp( s1.data(), s2.data(), s1.length()         ); }
int strnicmp_LenOfFirstStr( PCChar s1, PCChar s2, slen_t s2chars ) { return Strnicmp( s1, s2, std::min( Strlen( s1 ), s2chars ) ); }
int strncmp_LenOfFirstStr ( stref s1, stref s2 )                   { return strncmp ( s1.data(), s2.data(), s1.length()         ); }
int strncmp_LenOfFirstStr ( PCChar s1, PCChar s2, slen_t s2chars ) { return strncmp ( s1, s2, std::min( Strlen( s1 ), s2chars ) ); }

bool streq_LenOfFirstStr( PCChar s1, slen_t s1chars, PCChar s2, slen_t s2chars ) {
   if( s2chars == 0 ) { return s1chars==0; }
   return 0==Strnicmp( s1, s2, std::min( s1chars, s2chars ) );
   }

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

stref StrSpnSignedInt( stref src ) {
   if( src.empty() ) { return src; }
   auto signofs( src[0] == '-' || src[0] == '+' ? 1 : 0 );
   auto declook( src.substr( signofs ) );
   const auto ixPast( declook.find_first_not_of( "0123456789" ) );
   if( ixPast == stref::npos ) { return ""; }
   return src.substr( 0, signofs + ixPast );
   }

bool StrSpnSignedInt( PCChar pszString ) {
   if( *pszString == '-' || *pszString == '+' ) {
      ++pszString;
      }
   return *StrPastAny( pszString, "0123456789" ) == 0;
   }

typedef int (*isfxn)(int);

STATIC_FXN int consec_is_its( isfxn ifx, stref sr ) {
   auto rv( 0 );
   for( auto ch : sr ) {
      if( !ifx( ch ) ) {
         break;
         }
      ++rv;
      }
   return rv;
   }

int consec_xdigits( stref sr ) { return consec_is_its( isxdigit, sr ); }
int consec_bdigits( stref sr ) { return consec_is_its( isbdigit, sr ); }

int StrToInt_variable_base( stref pszParam, int numberBase ) {
   // rv is nonnegative int or -1 if conv error
   if( (10 == numberBase || 16 == numberBase)
      && '0' == pszParam[0]
      && ('x' == pszParam[1] || 'X' == pszParam[1])
     ) {
      pszParam.remove_prefix( 2 );
      numberBase = 16;
      }
   if( numberBase < 2 || numberBase > 36 ) {
      return -1;
      }
   auto accum(0);
   auto pC( pszParam.cbegin() );
   for( ; pC != pszParam.cend() ; ++pC ) {
      auto ch( *pC ); // cannot auto: *pC => const char, we need ch to be non-const
      if( isdigit( ch ) )               { ch -= '0'     ; }
      else if( ch >= 'a' && ch <= 'z' ) { ch -= 'a' - 10; }
      else if( ch >= 'A' && ch <= 'Z' ) { ch -= 'A' - 10; }
      else                              { break;          }
      if( ch >= numberBase ) {
         break;
         }
      accum = (accum * numberBase) + ch;
      }
   const auto rv( std::distance( pszParam.cbegin(), pC ) ? accum : -1 );
   return rv;
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
