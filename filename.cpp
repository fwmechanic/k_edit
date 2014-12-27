//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/naming_a_file.asp
//

#include "ed_main.h"

STATIC_FXN sridx DirnmLen( stref pPath ) { // a.k.a. IdxOfFnm()
   const auto rv( pPath.find_last_of( PATH_SEP_SRCH_STR ":" ) );
   if( stref::npos == rv ) {
      return 0;
      }
   else {
      return rv+1; // include last sep
      }
   }

stref Path::RefDirnm( stref src ) {
   return src.substr( 0, DirnmLen( src ) );
   }

Path::str_t Path::CpyDirnm( stref src ) {
   const auto rv( Path::RefDirnm( src ) );
   return Path::str_t( rv.data(), rv.length() );
   }

stref Path::RefFnameExt( stref src ) {
   return src.substr( DirnmLen( src ) );
   }

Path::str_t Path::CpyFnameExt( stref src ) {
   const auto rv( Path::RefFnameExt( src ) );
   return Path::str_t( rv.data(), rv.length() );
   }

stref Path::RefFnm( stref src ) { // a.k.a. IdxOfFnm()
   auto fx( Path::RefFnameExt( src ) );
   const auto idxDot( fx.find_last_of( '.' ) );
   if( idxDot == stref::npos || idxDot == 0 ) {
      return fx;
      }
   return fx.substr( 0, idxDot );
   }

Path::str_t Path::CpyFnm( stref src ) { // DOES NOT include the trailing "." !!!
   const auto rv( Path::RefFnm( src ) );
   Path::str_t rv_( rv.data(), rv.length() );           // 0 && DBG( "%s: '%s'->'%s'", __func__, src, rv_.c_str() );
   return rv_;
   }

stref Path::RefExt( stref src ) { // a.k.a. IdxOfFnm()
   auto fx( Path::RefFnameExt( src ) );
   const auto idxDot( fx.find_last_of( '.' ) );
   if( idxDot == stref::npos || idxDot == 0 ) {
      return stref( "", 0 );
      }
   return fx.substr( idxDot );
   }

Path::str_t Path::CpyExt( stref src ) { // if src contains an extension, prepends a leading "."
   const auto rv( Path::RefExt( src ) );
   Path::str_t rv_( rv.data(), rv.length() );           // 0 && DBG( "%s: '%s'->'%s'", __func__, src, rv_.c_str() );
   return rv_;
   }

bool Path::IsDotOrDotDot( stref str ) {
   return str ==                      "."
       || str ==                      ".."
       || str.ends_with( PATH_SEP_STR "."               )
       || str.ends_with( PATH_SEP_STR "."  PATH_SEP_STR )
       || str.ends_with( PATH_SEP_STR ".."              )
       || str.ends_with( PATH_SEP_STR ".." PATH_SEP_STR )
       ;
   }

bool Path::eq( stref name1, stref name2 ) {
   if( name1.length() != name2.length() ) {
      return false;
      }
   for( str_t::size_type ix( 0 ); ix < name1.length() ; ++ix ) {
      if( !PathChEq( name1[ix], name2[ix] ) ) {
         return false;
         }
      }
   return true;
   }

Path::str_t::size_type Path::CommonPrefixLen( stref s1, stref s2 ) {
   typedef Path::str_t::size_type s_t;
   const s_t past_end_ix( Min( s1.length(), s2.length() ) );
   s_t oPathSep( 0 );
   for( s_t ix( 0 ); ix < past_end_ix ; ++ix ) {
      if( !PathChEq( s1[ix], s2[ix] ) ) { break;             }
      if( IsPathSepCh( s1[ix] ) )       { oPathSep = ix + 1; }
      }
   return oPathSep;
   }

//----------------

Path::str_t Path::Union( stref s1, stref s2 ) { enum { DB=0 };
   // dest = (Path::CpyDirnm( s1 ) || Path::CpyDirnm( s2 ))
   //      + (Path::CpyFnm  ( s1 ) || Path::CpyFnm  ( s2 ))
   //      + (Path::CpyExt  ( s1 ) || Path::CpyExt  ( s2 ))
   auto dir( Path::RefDirnm( s1 ) ); if( dir.empty() )  dir = Path::RefDirnm( s2 );  // DB && DBG( "RExt:dir '%s'", dir.c_str() );
   auto fnm( Path::RefFnm  ( s1 ) ); if( fnm.empty() )  fnm = Path::RefFnm  ( s2 );  // DB && DBG( "RExt:fnm '%s'", fnm.c_str() );
   stref ext;
   if( !Path::IsDotOrDotDot( fnm ) ) {
      ext =  Path::RefExt  ( s1 )  ; if( ext.empty() )  ext = Path::RefExt  ( s2 );  // DB && DBG( "RExt:f+x '%s'", fnm.c_str() );
      }

   Path::str_t rv;
   rv.reserve( dir.length() + fnm.length() + ext.length() + 1 );
   rv.append( dir.data(), dir.length() );
   rv.append( fnm.data(), fnm.length() );
   rv.append( ext.data(), ext.length() );

   // following code was once commented out since this code
   //   converts "path\*." into "path\*", and "path\*" is equiv to "path\*.*",
   //   while "path\*." matches only extension-less files, which is sometimes
   //   needed.
   // UPDT: Win32::GetFullPathName does the same thing (converts "path\*." into "path\*")
   //       so I've restoring this code, which helps in other areas.
   //
   if( rv.back() == '.' )
      rv.pop_back();

   // DB && DBG( "RExt: '%s' + '%s' -> '%s+%s'", s2, s1, dir.c_str(), fnm.c_str() );
   return rv;
   }


bool Path::SetCwdOk( PCChar dnm ) {
   return !( WL( _chdir, chdir )( dnm ) == -1);
   }

Path::str_t Path::GetCwd_ps() {
   auto rv( Path::GetCwd() );
   if( rv.length() > 0 ) {
      rv += PATH_SEP_STR;
      }
   return rv;
   }

bool Path::IsLegalFnm( PCChar name ) {
   return StrToNextOrEos( name, Path::InvalidFnmChars() )[0] == '\0';
   }
