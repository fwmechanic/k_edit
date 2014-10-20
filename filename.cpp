//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/naming_a_file.asp
//

#include "ed_main.h"
#include <regex>

STATIC_FXN PCChar PPastLastPathSep( PCChar pPath ) {
   const auto p0( pPath );
   decltype(pPath) pNxt;
   while( *(pNxt=StrToNextOrEos( pPath, PATH_SEP_SRCH_STR ":" )) != 0 ) {
      pPath = pNxt + 1;
      }

   0 && DBG( "PPastLastPathSep '%s' -> '%s'", p0, pPath );
   return pPath;
   }

STATIC_FXN PCChar PAtLastDotOrEos( PCChar pPath ) {
   const auto p0( pPath );
   decltype(pPath) pNxtDot;
   if( *(pNxtDot=StrToNextOrEos( pPath, "." )) == 0 ) {
      0 && DBG( "PAtLastDotOrEos '%s' -> '%s'", p0, pNxtDot );
      return pNxtDot; // NO DOT: RETURN pEOS
      }

   pPath = pNxtDot;

   while( *(pNxtDot=StrToNextOrEos( pPath+1, "." )) != 0 ) {
      pPath = pNxtDot;
      }

   0 && DBG( "PAtLastDotOrEos '%s' -> '%s'", p0, pPath );
   return pPath;
   }

bool Path::IsDotOrDotDot( PCChar pC ) { // true if pC _ends with_ "\.." or "\." "\..\" or "\.\" or is "." or ".."
   const auto eos( Eos( pC ) );
   const auto len( eos - pC );
   return (len ==1 && 0==strcmp( pC     , "."  ))
       || (len ==2 && 0==strcmp( pC     , ".." ))
       || (len > 2 && 0==strcmp( eos - 2, PATH_SEP_STR "." ))
       || (len > 3 && 0==strcmp( eos - 3, PATH_SEP_STR "." PATH_SEP_STR ))
       || (len > 3 && 0==strcmp( eos - 3, PATH_SEP_STR ".." ))
       || (len > 4 && 0==strcmp( eos - 4, PATH_SEP_STR ".." PATH_SEP_STR ))
       ;
   }

int Path::strcmp( const std::string &name1, const std::string &name2 ) {
   return WL( Stricmp, Strcmp )( name1.c_str(), name2.c_str() );
   }

std::string Path::CpyDirOk( PCChar pSrcFullname ) {                                       0 && DBG( "%s  in =%s'", __func__, pSrcFullname );
   std::string rv( pSrcFullname, PPastLastPathSep( pSrcFullname ) - pSrcFullname );       0 && DBG( "%s  out=%s'", __func__, rv.c_str()   );
   return rv;
   }

std::string Path::CpyFnameOk( PCChar pSrcFullname ) { // DOES NOT include the trailing "." !!!
   const auto pC( PPastLastPathSep( pSrcFullname ) );
   auto pEnd( Path::IsDotOrDotDot( pC ) ? Eos( pC ) : PAtLastDotOrEos( pC ) );
   return std::string( pC, pEnd - pC );
   }

std::string Path::CpyExtOk( PCChar pSrcFullname ) { // if pSrcFullname contains an extension, prepends a leading "."
   const auto pC( PPastLastPathSep( pSrcFullname ) );
   if( Path::IsDotOrDotDot( pC ) ) {
      return std::string( "" );
      }
   return std::string( PAtLastDotOrEos( pC ) );
   }

std::string Path::CpyFnameExtOk( PCChar pSrcFullname ) {
   return Path::CpyFnameOk( pSrcFullname ) + Path::CpyExtOk( pSrcFullname );
   }

std::string Path::Union( PCChar pFirst, PCChar pSecond ) { enum { DB=0 };
   // dest = (Path::CpyDirOk  ( pFirst ) || Path::CpyDirOk  ( pSecond ))
   //      + (Path::CpyFnameOk( pFirst ) || Path::CpyFnameOk( pSecond ))
   //      + (Path::CpyExtOk  ( pFirst ) || Path::CpyExtOk  ( pSecond ))
   auto dir( Path::CpyDirOk  ( pFirst ) ); if( dir.length() == 0 )  dir = Path::CpyDirOk  ( pSecond );  DB && DBG( "RExt:dir '%s'", dir.c_str() );
   auto fnm( Path::CpyFnameOk( pFirst ) ); if( fnm.length() == 0 )  fnm = Path::CpyFnameOk( pSecond );  DB && DBG( "RExt:fnm '%s'", fnm.c_str() );
   if( !Path::IsDotOrDotDot( fnm.c_str() ) ) {
      auto ext( Path::CpyExtOk( pFirst ) ); if( ext.length() == 0 )  ext = Path::CpyExtOk( pSecond );
      fnm += ext;                                                                                       DB && DBG( "RExt:f+x '%s'", fnm.c_str() );
      }

   // following code was once commented out since this code
   //   converts "path\*." into "path\*", and "path\*" is equiv to "path\*.*",
   //   while "path\*." matches only extension-less files, which is sometimes
   //   needed.
   // UPDT: Win32::GetFullPathName does the same thing (converts "path\*." into "path\*")
   //       so I've restoring this code, which helps in other areas.
   //
   if( fnm.back() == '.' )
      fnm.pop_back();

   DB && DBG( "RExt: '%s' + '%s' -> '%s+%s'", pSecond, pFirst, dir.c_str(), fnm.c_str() );
   return dir + fnm;
   }


bool Path::SetCwdOk( PCChar dnm ) {
   return !( WL( _chdir, chdir )( dnm ) == -1);
   }


DirMatches::~DirMatches() {
#if defined(_WIN32)
   if( d_hFindFile != Win32::Invalid_Handle_Value() ) {
      Win32::FindClose( d_hFindFile );
      }
#else
#endif
   }

DirMatches::DirMatches( PCChar pszPrefix, PCChar pszSuffix, WildCardMatchMode wcMode, bool fAbsolutize )
   : d_wcMode( wcMode )
   , d_fTriedFirst(false)
   , d_buf( std::string(pszPrefix ? pszPrefix : "") + std::string(pszSuffix ? pszSuffix : "") )
#if defined(_WIN32)
   , d_hFindFile( Win32::Invalid_Handle_Value() )
#else
#endif
   { // prep the buffer
   if( fAbsolutize ) {
      d_buf = Path::Absolutize( d_buf.c_str() );
      }

   const bool hasDriveLetter( isalpha( d_buf[0] ) && d_buf[1] == ':' ); // leading c: ?
   if( hasDriveLetter ) {
      d_buf[0] = tolower( d_buf[0] );
      }

   { // hackaround
   auto pDest = Path::StrToPrevPathSepOrNull( d_buf.c_str(), PCChar(nullptr) );
   if( pDest )
      ++pDest; // point past pathsep
   else
      pDest = d_buf.c_str();
   d_ixDest = pDest - d_buf.c_str();
   }
   }
