//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

/* I'm not thrilled about this (named string literals via #defines), but I know of no way to move
   these into a namespace while allowing compile-time concatenation with other string literals.
 */

#if defined(_WIN32)
  #define  PATH_SEP_STR       "\\"
  #define  PATH_SEP_SRCH_STR "/\\"
#else
  #define  PATH_SEP_STR       "/"
  #define  PATH_SEP_SRCH_STR  PATH_SEP_STR
#endif

#include "my_types.h"

namespace Path {
   typedef std::string str_t; // Path::str_t

   template<typename CharPtr> STIL CharPtr StrToNextPathSepOrEos(  CharPtr pszToSearch          ) { return StrToNextOrEos ( pszToSearch , PATH_SEP_SRCH_STR ); }
   template<typename CharPtr> STIL CharPtr StrToPrevPathSepOrNull( CharPtr pBuf, CharPtr pInBuf ) { return StrToPrevOrNull( pBuf, pInBuf, PATH_SEP_SRCH_STR ); }

   enum chars : char { chEnvSep = ';', chDirSepMS = '\\', PATH_SEP_CH = chDirSepMS, chDirSepPosix = '/' };
#if defined(_WIN32)
   bool   STIL  IsPathSepCh( int ch ) { return ch == chDirSepMS || ch == chDirSepPosix; }
#else
   bool   STIL  IsPathSepCh( int ch ) { return ch == chDirSepPosix; }
#endif

   PCChar STIL  InvalidFnmChars()     { return "<>|*?"; }
   PCChar STIL  WildcardFnmChars()    { return "*?"; }
   PCChar STIL  EnvSepStr()           { return ";"; }
   extern bool  IsLegalFnm  ( PCChar name );

   extern bool  IsDotOrDotDot( PCChar pC );

   extern bool  SetCwdOk        ( PCChar dnm );
   extern str_t GetCwd    ();
   extern str_t GetCwd_ps (); // w/trailing pathsep to make it comparable with CpyDirnm() retval
   extern str_t CpyDirnm   ( PCChar pSrcFullname );
   extern str_t CpyFnm     ( PCChar pSrcFullname );
   extern str_t CpyExt     ( PCChar pSrcFullname );
   extern str_t CpyFnameExt( PCChar pSrcFullname );
   extern str_t Union        ( PCChar pFirst, PCChar pSecond );
   extern str_t Absolutize   ( PCChar pszFilename );
   // extern
          str_t CanonizeCase ( PCChar fnmBuf )
#if defined(_WIN32)
      ;
#else
      { return str_t( fnmBuf ); }
#endif
   ;

   STIL bool PathChEq( const char c1, const char c2 ) {
#if defined(_WIN32)
      return ::tolower( c1 ) == ::tolower( c2 );
#else
      return          ( c1 ) ==          ( c2 );
#endif
      }
   extern bool eq( const str_t &name1, const str_t &name2 ); // with appropriate case-sensitivity
   str_t::size_type CommonLen( const std::string &s1, const std::string &s2 );

   extern char  DelimChar( PCChar fnm );
   };
