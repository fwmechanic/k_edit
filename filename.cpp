//
// Copyright 2015,2022 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/naming_a_file.asp
//

#include "ed_main.h"

STATIC_FXN sridx DirnmLen( stref pPath ) { // a.k.a. IdxOfFnm()
   const auto rv( pPath.find_last_of( DIRSEP_SRCH_STR ":" ) );
   if( eosr == rv ) {
      return 0;
      }
   else {
      return rv+1; // include last sep
      }
   }

stref Path::RefDirnm( stref src ) {
   return src.substr( 0, DirnmLen( src ) );
   }

stref Path::RefFnameExt( stref src ) {
   return src.substr( DirnmLen( src ) );
   }

stref Path::RefFnm( stref src ) { // a.k.a. IdxOfFnm()
   auto fx( Path::RefFnameExt( src ) );
   const auto idxDot( fx.find_last_of( '.' ) );
   if( idxDot == eosr || idxDot == 0 ) {
      return fx;
      }
   return fx.substr( 0, idxDot );
   }

stref Path::RefExt( stref src ) { // a.k.a. IdxOfFnm()
   auto fx( Path::RefFnameExt( src ) );
   const auto idxDot( fx.find_last_of( '.' ) );
   if( idxDot == eosr || idxDot == 0 ) {
      return stref( "", 0 );
      }
   return fx.substr( idxDot );
   }

bool Path::IsDot( stref str ) { // truly useless!
   return str ==                    "."
       || str.ends_with( DIRSEP_STR "."             )
       || str.ends_with( DIRSEP_STR "."  DIRSEP_STR )
       ;
   }

bool Path::IsDotDot( stref str ) {
   return str ==                    ".."
       || str.ends_with( DIRSEP_STR ".."            )
       || str.ends_with( DIRSEP_STR ".." DIRSEP_STR )
       ;
   }

bool Path::IsDotOrDotDot( stref str ) {
   return IsDot( str ) || IsDotDot( str );
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

bool Path::startsWith( stref haystack, stref needle ) {
   if( haystack.length() < needle.length() ) {
      return false;
      }
   auto hit( haystack.cbegin() );
   for( auto nit(  needle.cbegin() ) ; nit != needle.cend() ; ++nit, ++hit ) {
      if( !PathChEq( *nit, *hit ) ) {
         return false;
         }
      }
   return true;
   }

bool Path::endsWith( stref haystack, stref needle ) {
   if( haystack.length() < needle.length() ) {
      return false;
      }
   auto hit( haystack.crbegin() );
   for( auto nit(  needle.crbegin() ) ; nit != needle.crend() ; ++nit, ++hit ) {
      if( !PathChEq( *nit, *hit ) ) {
         return false;
         }
      }
   return true;
   }

Path::str_t::size_type Path::CommonPrefixLen( stref s1, stref s2 ) {
   typedef Path::str_t::size_type s_t;
   const s_t past_end_ix( std::min( s1.length(), s2.length() ) );
   s_t oPathSep( 0 );
   for( s_t ix( 0 ); ix < past_end_ix ; ++ix ) {
      if( !PathChEq( s1[ix], s2[ix] ) ) { break;             }
      if( IsDirSepCh( s1[ix] ) )       { oPathSep = ix + 1; }
      }
   return oPathSep;
   }

//----------------

Path::str_t Path::Union( stref s1, stref s2 ) { enum { SD=0 };
   // dest = (Path::CpyDirnm( s1 ) || Path::CpyDirnm( s2 ))
   //      + (Path::CpyFnm  ( s1 ) || Path::CpyFnm  ( s2 ))
   //      + (Path::CpyExt  ( s1 ) || Path::CpyExt  ( s2 ))
   auto dir( Path::RefDirnm( s1 ) ); if( dir.empty() ) { dir = Path::RefDirnm( s2 ); } // SD && DBG( "RExt:dir '%s'", dir.c_str() );
   auto fnm( Path::RefFnm  ( s1 ) ); if( fnm.empty() ) { fnm = Path::RefFnm  ( s2 ); } // SD && DBG( "RExt:fnm '%s'", fnm.c_str() );
   stref ext;
   if( !Path::IsDotOrDotDot( fnm ) ) {
      ext =  Path::RefExt  ( s1 )  ; if( ext.empty() ) { ext = Path::RefExt  ( s2 ); } // SD && DBG( "RExt:f+x '%s'", fnm.c_str() );
      }
   Path::str_t rv;
   rv.reserve( dir.length() + fnm.length() + ext.length() + 1 );
   rv.append( dir );
   rv.append( fnm );
   rv.append( ext );
   // following code was once commented out since this code
   //   converts "path\*." into "path\*", and "path\*" is equiv to "path\*.*",
   //   while "path\*." matches only extension-less files, which is sometimes
   //   needed.
   // UPDT: Win32::GetFullPathName does the same thing (converts "path\*." into "path\*")
   //       so I've restoring this code, which helps in other areas.
   //
   if( rv.back() == '.' ) {
      rv.pop_back();
      }
   // SD && DBG( "RExt: '%s' + '%s' -> '%s+%s'", s2, s1, dir.c_str(), fnm.c_str() );
   return rv;
   }

COMPLEX_STATIC_VAR class {
   Path::str_t d_nm = Path::GetCwd_();
public:
   bool SetCwdOk( PCChar dnm ) {
      bool cd_ok( !( WL( _chdir, chdir )( dnm ) == -1) );                        0 && DBG( "%s cd_ok=%c %s", __func__, cd_ok?'t':'f', dnm );
      if( cd_ok ) {
         d_nm = Path::GetCwd_();
         if( d_nm.empty() ) {
            cd_ok = false;
            }
         }
      return cd_ok;
      }
   Path::str_t GetCwd() {
      /* if( d_nm.empty() ) { d_nm = Path::GetCwd_(); } */                       0 && DBG( "%s nm %s", __func__, d_nm.c_str() );
      return d_nm;
      }
   Path::str_t GetCwd_ps() {
      /* if( d_nm.empty() ) { d_nm = Path::GetCwd_(); } */                       0 && DBG( "%s nm %s", __func__, d_nm.c_str() );
      auto rv( Path::IsDirSepCh( d_nm.back() ) ? d_nm : (d_nm + DIRSEP_STR) );   0 && DBG( "%s == %s", __func__, rv.c_str() );
      return rv;
      }
   } s_cwd_cache;

bool Path::SetCwdOk( PCChar dnm ) {
   return s_cwd_cache.SetCwdOk( dnm );
   }

Path::str_t Path::GetCwd() {
   return s_cwd_cache.GetCwd();
   }

Path::str_t Path::GetCwd_ps() {
   return s_cwd_cache.GetCwd_ps();
   }

bool Path::IsLegalFnm( stref name ) {
   return name.find_first_of( Path::InvalidFnmChars() ) == eosr;
   }

bool Path::IsAbsolute( stref name ) {
   if( name.length() == 0 ) { return false; }
   if( name.length() == 1 ) { return IsDirSepCh( name[0] ); }
   if( name.length() >= 3 ) {
      return isalpha( name[0] ) && name[1] == ':' && IsDirSepCh( name[2] );
      }
   return false;
   }
