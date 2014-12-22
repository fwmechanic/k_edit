//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

#include <cstdarg>
#include <cstring>
#include "my_types.h"
#include "ed_mem.h" // for Strdup()

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
   #define  vsnprintf  _vsnprintf
#endif


// snprintf_full: use to write multiple formatted strings to a fixed-len buffer
// without performing repeated strlen's.
// returns NZ if buffer was partially (including _not_) written, else Z
//

extern int   snprintf_full( char **ppBuf, size_t *pBufBytesRemaining, PCChar fmt, ... ) ATTR_FORMAT(3, 4);

//--------------------------------------------------------------------------------------------

const char HTAB('\t');
const char chESC('\\');
const char chQuot1('\'');
const char chQuot2('"' );
const char chBackTick('`');
const char chLSQ( '[' );
const char chRSQ( ']' );


enum { MAX_TAB_WIDTH = 8, // we don't support > MAX_TAB_WIDTH cols per tab!
       TICK     = 0x27,
       BACKTICK = 0x60,
   };

//#######################################################################################

extern int  strcmp4humans( PCChar s1, PCChar s2 );
extern int  stricmp_eos( PCChar s1, PCChar eos1, PCChar s2, PCChar eos2 );
extern int  strcmp_eos ( PCChar s1, PCChar eos1, PCChar s2, PCChar eos2 );

extern int  strnicmp_LenOfFirstStr( PCChar s1, PCChar s2 );
extern int  strnicmp_LenOfFirstStr( PCChar s1, PCChar s2, int s2chars );
extern int  strncmp_LenOfFirstStr( PCChar s1, PCChar s2 );
extern int  strncmp_LenOfFirstStr( PCChar s1, PCChar s2, int s2chars );
extern bool streq_LenOfFirstStr( PCChar s1, int s1chars, PCChar s2, int s2chars );

extern   int   StrToInt_variable_base( PCChar pszNumericString, int numberBase );
extern   boost::string_ref  StrSpnSignedInt( boost::string_ref src );
extern   bool  StrSpnSignedInt( PCChar pszString );
extern PCChar  Add_es( int count );
extern PCChar  Add_s(  int count );
extern   char  FlipCase( char ch );
extern PChar   xlatCh( PChar pStr, int fromCh, int toCh );
extern   int   StrTruncTrailBlanks( PChar pszString );
extern   int   DoubleBackslashes( PChar pDest, size_t sizeofDest, PCChar pSrc );
extern   void  StrUnDoubleBackslashes( PChar pszString );

STIL         bool   isBlank(    char ch ) { return ch == HTAB || ch == ' '; }
STIL         bool   isDecDigit( char ch ) { return ch >= '0'  && ch <= '9'; }
STIL         bool   StrContainsTabs( PCChar data, int dataBytes )  { return ToBOOL(memchr( data, HTAB, dataBytes  )); }
STIL         bool   StrContainsTabs( PCChar data, PCChar eos )     { return ToBOOL(memchr( data, HTAB, eos - data )); }
STIL         bool   StrContainsTabs( boost::string_ref src )       { return ToBOOL(memchr( src.data(), HTAB, src.length() )); }

TF_Ptr STIL  Ptr    StrNxtTab      ( Ptr data, Ptr eos )  { const auto pm( const_cast<Ptr>(static_cast<PChar>(memchr( data, HTAB, eos - data )))); return (pm ? pm : eos); }
TF_Ptr STIL  Ptr    StrNxtTabOrNull( Ptr data, Ptr eos )  {         return const_cast<Ptr>(static_cast<PChar>(memchr( data, HTAB, eos - data ))) ; }

extern bool eqi( boost::string_ref s1, boost::string_ref s2 );
extern char  toLower(     int ch );
extern bool  IsStringBlank( boost::string_ref src );
extern bool  StrEmpty(    PCChar inbuf );
extern PChar suckwhite(   PChar string, int xMin, int xMax );

extern int   consec_xdigits( PCChar pSt, PCChar eos=nullptr );
extern int   consec_bdigits( PCChar pSt, PCChar eos=nullptr );

//--------------------------------------------------------------------------------

// to avoid signed/unsigned mismatch warnings:

#if 0
STIL   int Strlen( PCChar pS ) { return int( strlen(pS) ); }
#else
extern int Strlen( PCChar pS );
#endif
//--------------------------------------------------------------------------------

extern PChar GetenvStrdup( PCChar pStart, size_t len );

STIL PChar GetenvStrdup( PCChar pStart, PCChar pPastEnd ) { return GetenvStrdup( pStart, pPastEnd - pStart ); }

//--------------------------------------------------------------------------------

extern int safeStrcpy( PChar dest, size_t sizeofDest, PCChar src, int srcLen );

#define    SafeStrcpy( d, s )  safeStrcpy( BSOB(d), s )

STIL int safeStrcpy( PChar dest, size_t sizeofDest, PCChar src, PCChar pAtSrcTerm ) {
   return safeStrcpy( dest, sizeofDest, src, pAtSrcTerm - src );
   }

STIL int safeStrcpy( PChar dest, size_t sizeofDest, PCChar src ) {
   return safeStrcpy( dest, sizeofDest, src, Strlen( src ) );
   }

// if pWithinDest is within a buffer that can be sizeof()'d, this variant of safeStrcpy can be used in lieu of safeStrcat
//
// STIL int safeStrcpy( PChar destbuf, size_t sizeofDestbuf, PChar pWithinDest, PCChar src ) {
//    return safeStrcpy( pWithinDest, sizeofDestbuf - (pWithinDest - destbuf), src, Strlen( src ) );
//    }

#define     SafeStrcat( d, s )  safeStrcat( BSOB(d), s )

extern int safeStrcat( PChar dest, size_t sizeof_dest, int destLen, PCChar src, int srcLen );

STIL void safeStrcat( PChar dest, size_t sizeof_dest, int destLen, PCChar src )  { safeStrcat( dest, sizeof_dest, destLen       , src, Strlen( src ) ); }
STIL void safeStrcat( PChar dest, size_t sizeof_dest             , PCChar src )  { safeStrcat( dest, sizeof_dest, Strlen( dest ), src ); }

extern PChar safeSprintf( PChar dest, size_t sizeofDest, PCChar format, ... ) ATTR_FORMAT(3,4);

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

extern const char szSpcTab[];
extern const char szMacroTerminators[];
extern       char g_szWordChars[];

TF_Ptr STIL Ptr  Eos( Ptr psz ) { return psz + Strlen( psz ); }

STIL PCChar Strchr( PCChar psz, int ch ) { return       strchr( psz, ch ); }
STIL PChar  Strchr( PChar  psz, int ch ) { return PChar(strchr( psz, ch )); }

extern void string_tolower( std::string &inout );

#if !defined(_WIN32)

extern  PChar _strupr( PChar buf );
extern  PChar _strlwr( PChar buf );

#endif

STIL int Strnicmp( PCChar string1, PCChar string2, size_t count ) { return WL( _strnicmp, strncasecmp )( string1, string2, count ); }
STIL int Stricmp ( PCChar string1, PCChar string2 )               { return WL( _strcmpi , strcasecmp  )( string1, string2 ); }
STIL int Strcmp  ( PCChar string1, PCChar string2 )               { return      strcmp                 ( string1, string2 ); }

extern size_t Strnspn  ( PCChar str1, PCChar eos1, PCChar needle );
extern size_t Strncspn ( PCChar str1, PCChar eos1, PCChar needle );

TF_Ptr STIL Ptr  StrToNextOrEos( Ptr pszToSearch, PCChar pszToSearchFor ) { return pszToSearch + Strncspn( pszToSearch, nullptr, pszToSearchFor ); }
TF_Ptr STIL Ptr  StrPastAny(     Ptr pszToSearch, PCChar pszToSearchFor ) { return pszToSearch + Strnspn ( pszToSearch, nullptr, pszToSearchFor ); }

TF_Ptr STIL Ptr  StrToNextOrEos( Ptr ps, Ptr eos, PCChar pszToSearchFor ) { return ps + Strncspn( ps, eos, pszToSearchFor ); }
TF_Ptr STIL Ptr  StrPastAny(     Ptr ps, Ptr eos, PCChar pszToSearchFor ) { return ps + Strnspn ( ps, eos, pszToSearchFor ); }

TF_Ptr STIL Ptr  StrPastAnyBlanks(     Ptr pszToSearch ) { return StrPastAny(     pszToSearch, szSpcTab ); }
TF_Ptr STIL Ptr  StrToNextBlankOrEos( Ptr pszToSearch ) { return StrToNextOrEos( pszToSearch, szSpcTab ); }

TF_Ptr STIL Ptr  StrPastAnyBlanks(     Ptr ps, Ptr eos ) { return StrPastAny(     ps, eos, szSpcTab ); }
TF_Ptr STIL Ptr  StrToNextBlankOrEos( Ptr ps, Ptr eos ) { return StrToNextOrEos( ps, eos, szSpcTab ); }

extern boost::string_ref::size_type PastAnyBlanksToEnd( boost::string_ref src, boost::string_ref::size_type start );
extern boost::string_ref::size_type ToNextBlankOrEnd ( boost::string_ref src, boost::string_ref::size_type start );

TF_Ptr STIL Ptr  StrToNextWordOrEos( Ptr pszToSearch ) { return StrToNextOrEos( pszToSearch, g_szWordChars ); }

extern PCChar StrPastWord(  PCChar pszToSearch );
STIL   PChar  StrPastWord(  PChar  pszToSearch ) { return PChar(StrPastWord( PCChar(pszToSearch) )); }

extern PCChar StrPastWord(  PCChar pszToSearch, PCChar eos );
STIL   PChar  StrPastWord(  PChar  pszToSearch, PChar  eos ) { return PChar(StrPastWord( PCChar(pszToSearch), PCChar(eos) )); }

extern PCChar StrWordStart( PCChar bos, PCChar ps );
STIL   PChar  StrWordStart( PChar  bos, PChar  ps ) { return PChar(StrWordStart( PCChar(bos), PCChar(ps) )); }

extern boost::string_ref::size_type IdxLastWordCh ( boost::string_ref src, boost::string_ref::size_type start );
extern boost::string_ref::size_type IdxFirstWordCh( boost::string_ref src, boost::string_ref::size_type start );
extern boost::string_ref::size_type StrLastWordCh(  boost::string_ref src );

//-----------------

extern PCChar StrToPrevOrNull_(   PCChar pBuf, PCChar pInBuf, PCChar toMatch );
extern PCChar StrPastPrevOrNull_( PCChar pBuf, PCChar pInBuf, PCChar toMatch );

TF_Ptr STIL Ptr  StrToPrevOrNull(             Ptr pBuf, Ptr pInBuf, PCChar toMatch ) { return const_cast<Ptr>(StrToPrevOrNull_  ( pBuf, pInBuf, toMatch )); }
TF_Ptr STIL Ptr  StrPastPrevOrNull(           Ptr pBuf, Ptr pInBuf, PCChar toMatch ) { return const_cast<Ptr>(StrPastPrevOrNull_( pBuf, pInBuf, toMatch )); }

TF_Ptr STIL Ptr  StrPastPrevWordOrNull(       Ptr pBuf, Ptr pInBuf ) { return StrPastPrevOrNull( pBuf, pInBuf, g_szWordChars ); }
TF_Ptr STIL Ptr  StrToPrevBlankOrNull(   Ptr pBuf, Ptr pInBuf ) { return StrToPrevOrNull  ( pBuf, pInBuf, szSpcTab      ); }
TF_Ptr STIL Ptr  StrPastPrevBlankOrNull( Ptr pBuf, Ptr pInBuf ) { return StrPastPrevOrNull( pBuf, pInBuf, szSpcTab      ); }

//-----------------

extern int Dquot_strcspn( PCChar pszToSearch, PCChar pszToSearchFor );

TF_Ptr STIL Ptr DquotStrToNextOrEos( Ptr  pszToSearch, PCChar pszToSearchFor ) { return pszToSearch + Dquot_strcspn( pszToSearch, pszToSearchFor ); }

TF_Ptr STIL Ptr StrToNextMacroTermOrEos( Ptr pszToSearch ) { return DquotStrToNextOrEos( pszToSearch, szMacroTerminators ); }


//#######################################################################################

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
   { // assumes that last char in b is a '\0', and preserves it
   memmove( b+xCol+insertWidth, b+xCol, (sizeof_b-1) - (xCol+insertWidth) );
   }

template <int elements>
class FixedCharArray {
   char b[ elements ];

   public:

   FixedCharArray() {}
   FixedCharArray( PCChar src ) { Strcpy( src );  }
   FixedCharArray( const FixedCharArray &rhs )              { Strcpy( rhs.b ); } // COPY CTOR
   FixedCharArray & operator= ( const FixedCharArray &rhs ) { Strcpy( rhs.b ); return *this; } // ASGN_OPR
   operator PChar()        { return b; }
   operator PCChar() const { return b; }

   char &operator[](int subscript)         { return b[ subscript ]; }
   char  operator[](int subscript)   const { return b[ subscript ]; }

   PChar    Eos()                          { return ::Eos(b); }
   int      Len()                    const { return Strlen( b ); }
   int      Size()                   const { return sizeof(b); }
   int      FreeChars()              const { return Size() - (Len() + 1); }

   PCChar   k_str()                  const { return b; }
   PChar    c_str()                        { return b; }

   int   Strcpy(  PCChar src             ) { return safeStrcpy( b, sizeof( b ), src ); }
   int   Strcpy(  PCChar src, int srcLen ) { return safeStrcpy( b, sizeof( b ), src, srcLen ); }
   void  Strcat(  PCChar src             ) {        safeStrcat( b, sizeof( b ), src ); }
   void  Strncat( PCChar src, int srcLen ) {        safeStrcat( b, sizeof( b ), Len(), src, srcLen ); }
   int   xMaxNulCh()    const { return Size() - 1; } // lb[ lb.xMaxNulCh()  ] = 0; is only allowed deref
   int   xMaxNonNulCh() const { return Size() - 2; } // lb[ lb.xMaxNonNulCh() ] = any; is allowed

   void Vsprintf( PCChar format, va_list val ) {
      use_vsnprintf( b, sizeof(b), format, val );
      }

   PCChar Sprintf( PCChar format, ... ) ATTR_FORMAT(2,3)
      {
      va_list args;
      va_start(args, format);
      Vsprintf( format, args );
      va_end(args);
      return b;
      }

   PChar PSprintf( PCChar pinb, PCChar format, ... ) ATTR_FORMAT(3,4)
      {
      const auto len( pinb - b );
      if( len < sizeof( b ) - 1 ) {
         va_list args;
         va_start(args, format);
         use_vsnprintf( b+len, sizeof( b )-len, format, args );
         va_end(args);
         }
      return b;
      }

   PChar SprintfCat( PCChar format, ... ) ATTR_FORMAT(2,3)
      {
      const auto len( Len() );
      if( len < sizeof( b ) - 1 ) {
         va_list args;
         va_start(args, format);
         use_vsnprintf( b+len, sizeof( b )-len, format, args );
         va_end(args);
         }
      return b;
      }

   //====================================================================================
   //
   // C++ macros for editing text in linebuf's.  ASSUMES an operating mode where ALL
   // trailing (rightmost) chars after the first '\0' are also forced to value '\0'.
   //
   // insert_hole   - prior to inserting chars
   // collapse_hole - delete chars, slide trailing chars left to fill in
   // clear_right   - chars at and to right of xCol are '\0'd
   //
   void insert_hole( int xCol, int insertWidth=1 ) { // assumes that last char in b is a '\0', and preserves it
      ::insert_hole( b, sizeof(b), xCol, insertWidth );
      }

   void collapse_hole( int xCol, int collapseWidth=1 ) { // assumes that last char in b is a '\0', and preserves it
      memmove( b+xCol, b+xCol+collapseWidth, sizeof(b) - (xCol+collapseWidth) );
      if( collapseWidth > 1 )  // memset is optimized out if false
         memset( b + sizeof(b) - collapseWidth, 0, collapseWidth-1 );
      }

   void clear_right( int xCol ) { // zeroes all chars indices >= xCol (at and right of xCol)
      memset( b + xCol, 0, sizeof(b) - xCol );
      }

private:
   // NO_ASGN_OPR(FixedCharArray);
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
// sequece of strings w/ NO heap interaction is appreciated.
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
      Reset();
      }

   PCChar Strings() { return d_buf; }

   void Reset() {
      d_buf[0] = '\0';
      d_buf[1] = '\0';  // not ABSOLUTELY necessary, but cheap insurance...
      d_nxtS = d_buf;
      }

   PCChar AddString( PCChar key, PCChar eos=nullptr ) {
      if( !eos ) eos = Eos( key );
      const auto len( eos-key );
      if( len > ((d_buf + sizeof(d_buf) - 1) - d_nxtS) - 2 ) return nullptr;
      auto rv( d_nxtS );
      memcpy( d_nxtS, key, len );
       d_nxtS += len;
      *d_nxtS++ = '\0';
      *d_nxtS   = '\0';
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
   DiceableString( PCChar src )
      : d_heapString( Strdup( src, Strlen( src ) + 1 ) ) // tricky: this memcpy's n-1 chars from src (which _includes_ the NUL at the end of src) and pokes a (second) NUL into dest[n-1]
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
