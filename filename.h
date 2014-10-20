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
   extern std::string GetCwd    ();
   extern std::string CpyDirOk     ( PCChar pSrcFullname );
   extern std::string CpyFnameOk   ( PCChar pSrcFullname );
   extern std::string CpyExtOk     ( PCChar pSrcFullname );
   extern std::string CpyFnameExtOk( PCChar pSrcFullname );
   extern std::string Union        ( PCChar pFirst, PCChar pSecond );
   extern std::string Absolutize   ( PCChar pszFilename );
   extern std::string CanonizeCase ( PCChar fnmBuf );
   extern int   strcmp( const std::string &name1, const std::string &name2 ); // with appropriate case-sensitivity
   extern char  DelimChar( PCChar fnm );
   };
