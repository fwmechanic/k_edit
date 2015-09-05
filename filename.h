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

#pragma once

/* I'm not thrilled about this (named string literals via #defines), but I know of no way to move
   these into a namespace while allowing compile-time concatenation with other string literals.
 */

#if defined(_WIN32)
  #define  PATH_SEP_CH        '\\'
  #define  PATH_SEP_STR       "\\"
  #define  PATH_SEP_SRCH_STR "/\\"
#else
  #define  PATH_SEP_CH        '/'
  #define  PATH_SEP_STR       "/"
  #define  PATH_SEP_SRCH_STR  PATH_SEP_STR
#endif

#include "my_types.h"

// moved from my_strutils.h
extern PCChar StrToPrevOrNull_(   PCChar pBuf, PCChar pInBuf, PCChar toMatch );
TF_Ptr STIL Ptr  StrToPrevOrNull(        Ptr pBuf, Ptr pInBuf, PCChar toMatch ) { return const_cast<Ptr>(StrToPrevOrNull_  ( pBuf, pInBuf, toMatch )); }
TF_Ptr STIL Ptr  StrToPrevBlankOrNull(   Ptr pBuf, Ptr pInBuf ) { return StrToPrevOrNull  ( pBuf, pInBuf, szSpcTab      ); }


namespace Path {
   typedef std::string str_t; // Path::str_t

   template<typename CharPtr> STIL CharPtr StrToNextPathSepOrEos(  CharPtr pszToSearch          ) { return StrToNextOrEos ( pszToSearch , PATH_SEP_SRCH_STR ); }
   template<typename CharPtr> STIL CharPtr StrToPrevPathSepOrNull( CharPtr pBuf, CharPtr pInBuf ) { return StrToPrevOrNull( pBuf, pInBuf, PATH_SEP_SRCH_STR ); }

   enum chars : char { chEnvSep = ';', chDirSepMS = '\\', chDirSepPosix = '/' };
#if defined(_WIN32)
   bool   STIL  IsPathSepCh( int ch ) { return ch == chDirSepMS || ch == chDirSepPosix; }
#else
   bool   STIL  IsPathSepCh( int ch ) { return ch == chDirSepPosix; }
#endif

   PCChar STIL  InvalidFnmChars()     { return "<>|*?"; }
   PCChar STIL  WildcardFnmChars()    { return "*?"; }
   PCChar STIL  EnvSepStr()           { return ";"; }
   extern bool  IsLegalFnm   ( stref name );
   extern bool  IsDot        ( stref str );
   extern bool  IsDotDot     ( stref str );
   extern bool  IsDotOrDotDot( stref str );

   extern bool  SetCwdOk        ( PCChar dnm );
   extern str_t GetCwd    ();
   extern str_t GetCwd_ps (); // w/trailing pathsep to make it comparable with CpyDirnm() retval

   extern stref RefDirnm(    stref src );
   extern stref RefFnm(      stref src );
   extern stref RefExt(      stref src );
   extern stref RefFnameExt( stref src );

   extern str_t Union      ( stref s1, stref s2 );
   extern str_t Absolutize   ( PCChar pszFilename );
#if defined(_WIN32)
   extern str_t CanonizeCase ( PCChar fnmBuf );
#else
   STIL   str_t CanonizeCase ( PCChar fnmBuf ) { return str_t( fnmBuf ); }
#endif
   STIL bool PathChEq( const char c1, const char c2 ) {
#if defined(_WIN32)
      return ::tolower( c1 ) == ::tolower( c2 );
#else
      return          ( c1 ) ==          ( c2 );
#endif
      }
   extern bool eq( stref name1, stref name2 ); // with appropriate case-sensitivity
   extern bool endsWith( stref haystack, stref needle ); // true if haystack ends with needle, using PathChEq
   str_t::size_type CommonPrefixLen( stref s1, stref s2 );

   extern char  DelimChar( PCChar fnm );
   };
