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
   typedef std::string str_t; //

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
   extern str_t CpyDirOk     ( PCChar pSrcFullname );
   extern str_t CpyFnameOk   ( PCChar pSrcFullname );
   extern str_t CpyExtOk     ( PCChar pSrcFullname );
   extern str_t CpyFnameExtOk( PCChar pSrcFullname );
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
   extern int   strcmp( const str_t &name1, const str_t &name2 ); // with appropriate case-sensitivity
   extern char  DelimChar( PCChar fnm );
   };
