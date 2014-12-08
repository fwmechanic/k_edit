//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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
   const auto rv( WL( _vsnprintf, vsnprintf )( buf, bufBytes, format, val ) );
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

STATIC_FXN void StrTruncd_( PCChar fxnm, int truncd, PCChar src, PCChar dst ) {
   STATIC_CONST char fmt[] = "%s: STRING TRUNCATED by %d chars\nsrc: '%s'\ndst: '%s'";
// STATIC_VAR bool alerted;    BUGBUG fix this
// if( !alerted )  ConIO::DbgPopf( fmt, fxnm, truncd, src, dst );
// else
                   DBG(     fmt, fxnm, truncd, src, dst );
// alerted = true;
   }

#define  StrTruncd( truncd, src, dst )  StrTruncd_( __func__, truncd, src, dest );

PChar safeSprintf( PChar dest, size_t sizeofDest, PCChar format, ... ) {
   va_list val;
   va_start(val, format);
   chkdVsnprintf( dest, sizeofDest, format, val );
   va_end(val);
   return dest;
   }

int safeStrcat( PChar dest, size_t sizeof_dest, int destLen, PCChar src, int srcLen ) {
   auto truncd( 0 );
   if( destLen + srcLen + 1 > sizeof_dest ) {
      truncd = srcLen - (sizeof_dest - destLen - 1);
      srcLen = sizeof_dest - destLen - 1;
      }

   auto rv( srcLen );
   if( srcLen > 0 ) {
      memcpy( dest + destLen, src, srcLen );
      dest[ destLen + srcLen ] = '\0';
      rv = destLen + srcLen;
      }

   if( truncd )
       StrTruncd_( __func__, truncd, src, dest );

   return rv;
   }

int safeStrcpy( PChar dest, size_t sizeofDest, PCChar src, int srcLen ) {
   auto truncd( 0 );
   if( srcLen >= sizeofDest ) {
       truncd = srcLen - (sizeofDest - 1);
       srcLen = sizeofDest - 1;
       }

   memcpy( dest, src, srcLen );
   dest[ srcLen ] = '\0';

   if( truncd )
       StrTruncd_( __func__, truncd, src, dest );

   return srcLen;
   }

int Strlen( register PCChar pc ) { // this MAY be faster than RTL version (which typically uses rep scasb); at least it uses stdcall ...
   const auto start(pc);
   for( ; *pc ; ++pc )
      ;

   return pc - start;
   }

#define strcmp_eos_def( fxnm, CMPFX )                        \
int fxnm( PCChar s1, PCChar eos1, PCChar s2, PCChar eos2 ) { \
   const auto l1( eos1-s1 );                                 \
   const auto l2( eos2-s2 );                                 \
   const auto rv( CMPFX ( s1, s2, Min( l1,l2 ) ) );          \
   if( l1==l2 || rv != 0 ) return rv;                        \
   return (l1 > l2) ? +1 : -1;                               \
   }

strcmp_eos_def( stricmp_eos, Strnicmp )
strcmp_eos_def( strcmp_eos , memcmp   )

#undef strcmp_eos_def

int strnicmp_LenOfFirstStr( PCChar s1, PCChar s2 )              { return Strnicmp( s1, s2,      Strlen( s1 )            ); }
int strnicmp_LenOfFirstStr( PCChar s1, PCChar s2, int s2chars ) { return Strnicmp( s1, s2, Min( Strlen( s1 ), s2chars ) ); }
int strncmp_LenOfFirstStr ( PCChar s1, PCChar s2 )              { return strncmp ( s1, s2,      Strlen( s1 )            ); }
int strncmp_LenOfFirstStr ( PCChar s1, PCChar s2, int s2chars ) { return strncmp ( s1, s2, Min( Strlen( s1 ), s2chars ) ); }

bool streq_LenOfFirstStr( PCChar s1, int s1chars, PCChar s2, int s2chars ) {
   if( s2chars == 0 ) { return s1chars==0; }
   return 0==Strnicmp( s1, s2, Min( s1chars, s2chars ) );
   }

#define Strnspn_def( fxnm, EQ_OP )                           \
size_t fxnm  ( PCChar str1, PCChar eos1, PCChar needle ) {   \
   if( !eos1 ) eos1 = Eos( str1 );                           \
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

#define StrToPastPrevOrNull_def( fxnm, EQ_OP )               \
PCChar fxnm( PCChar p0, PCChar pX, PCChar toMatch ) {        \
   if( !pX ) pX = Eos( p0 );                                 \
   if( pX == p0 ) return nullptr; /* no "prev" to match? */  \
   for( --pX ;  ; --pX ) {                                   \
      if( nullptr EQ_OP strchr( toMatch, *pX ) ) return pX;  \
      if( pX == p0 ) return nullptr;                         \
      }                                                      \
   }

StrToPastPrevOrNull_def( StrToPrevOrNull_  , != ) // do NOT call this directly!  use StrToPrevOrNull or StrToPrevWhitespaceOrNull instead!
StrToPastPrevOrNull_def( StrPastPrevOrNull_, == ) // do NOT call this directly!  use StrToPrevOrNull or StrToPrevWhitespaceOrNull instead!

#undef StrToPastPrevOrNull_def

int Dquot_strcspn( PCChar pszToSearch, PCChar pszToSearchFor ) {
   const auto p0( pszToSearch );
   auto fInDQuotedStr(false);
   char ch;
   for( ; (ch=*pszToSearch); ++pszToSearch ) {
      const auto isDQUOT( ch == '"' && (pszToSearch > p0) && (pszToSearch[-1] != '\\') );
      if( fInDQuotedStr ) {
         if( isDQUOT )
            fInDQuotedStr = false;
         }
      else {
         if( isDQUOT )
            fInDQuotedStr = true;
         else
            if( strchr( pszToSearchFor, ch ) )
               return pszToSearch - p0;
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

    if( *pBufBytesRemaining <= 1 )
       return 1;

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

boost::string_ref StrSpnSignedInt( boost::string_ref src ) {
   if( src.empty() ) return src;
   auto signofs( src[0] == '-' || src[0] == '+' ? 1 : 0 );
   auto declook( src.substr( signofs ) );
   const auto ixPast( declook.find_first_not_of( "0123456789" ) );
   if( ixPast == boost::string_ref::npos ) { return ""; }
   return src.substr( 0, signofs + ixPast );
   }

bool StrSpnSignedInt( PCChar pszString ) {
   if( *pszString == '-' || *pszString == '+' )
      ++pszString;

   return *StrPastAny( pszString, "0123456789" ) == 0;
   }

typedef int (*isfxn)(int);

int consec_is_its( isfxn ifx, PCChar pSt, PCChar eos ) {
   if( !eos ) eos = Eos( pSt );
   auto pS(pSt);
   for( ; pS != eos ; ++pS ) {
      if( !ifx( *pS ) ) break;
      }
   return pS - pSt;
   }

int isxdigit_( int ch ) { return isxdigit( ch ); }
int isbdigit_( int ch ) { return ch == '0' || ch == '1'; }

int consec_xdigits( PCChar pSt, PCChar eos ) {
   return consec_is_its( isxdigit_, pSt, eos );
   }

int consec_bdigits( PCChar pSt, PCChar eos ) {
   return consec_is_its( isbdigit_, pSt, eos );
   }

// from http://www.stereopsis.com/strcmp4humans.html

STIL int parsedecnum( PCChar & pch ) {
   auto result( *pch - '0' );
   ++pch;

   while( isDecDigit(*pch) ) {
      result *= 10;
      result += *pch - '0';
      ++pch;
      }

   --pch;
   return result;
   }

int strcmp4humans( PCChar pA, PCChar pB ) {
   if (pA == pB     ) return  0;
   if (pA == nullptr) return -1;
   if (pB == nullptr) return  1;

   for ( ; *pA && *pB ; ++pA, ++pB ) {
      extern char toLower( int ch );
      const auto a0( isDecDigit(*pA) ? parsedecnum(pA) + 256 :  toLower(*pA) );  // will contain either a number or a letter
      const auto b0( isDecDigit(*pB) ? parsedecnum(pB) + 256 :  toLower(*pB) );  // will contain either a number or a letter
      if( a0 < b0 ) return -1;
      if( a0 > b0 ) return  1;
      }

   if (*pA) return  1;
   if (*pB) return -1;

   return 0;
   }

#if !defined(_WIN32)

PChar _strupr( PChar buf ) {
   const auto rv( buf );
   for( ; *buf ; ++buf ) {
      *buf = toupper( *buf );
      }
   return rv;
   }

PChar _strlwr( PChar buf ) {
   const auto rv( buf );
   for( ; *buf ; ++buf ) {
      *buf = tolower( *buf );
      }
   return rv;
   }

#endif

void string_tolower( Path::str_t &inout ) {
   std::transform( inout.begin(), inout.end(), inout.begin(), ::tolower );
   }
