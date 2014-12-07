//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/naming_a_file.asp
//

#include "ed_main.h"

STATIC_FXN boost::string_ref::size_type DirnmLen( boost::string_ref pPath ) { // a.k.a. IdxOfFnm()
   const auto rv( pPath.find_last_of( PATH_SEP_SRCH_STR ":" ) );
   if( boost::string_ref::npos == rv ) {
      return 0;
      }
   else {
      return rv+1; // include last sep
      }
   }

boost::string_ref Path::RefDirnm( boost::string_ref src ) {
   return src.substr( 0, DirnmLen( src ) );
   }

Path::str_t Path::CpyDirnm( boost::string_ref src ) {
   const auto rv( Path::RefDirnm( src ) );
   return Path::str_t( rv.data(), rv.length() );
   }

boost::string_ref Path::RefFnameExt( boost::string_ref src ) {
   return src.substr( DirnmLen( src ) );
   }

Path::str_t Path::CpyFnameExt( boost::string_ref src ) {
   const auto rv( Path::RefFnameExt( src ) );
   return Path::str_t( rv.data(), rv.length() );
   }

boost::string_ref Path::RefFnm( boost::string_ref src ) { // a.k.a. IdxOfFnm()
   auto fx( Path::RefFnameExt( src ) );
   const auto idxDot( fx.find_last_of( '.' ) );
   if( idxDot == boost::string_ref::npos || idxDot == 0 ) {
      return fx;
      }
   return fx.substr( 0, idxDot );
   }

Path::str_t Path::CpyFnm( boost::string_ref src ) { // DOES NOT include the trailing "." !!!
   const auto rv( Path::RefFnm( src ) );
   Path::str_t rv_( rv.data(), rv.length() );           // 0 && DBG( "%s: '%s'->'%s'", __func__, src, rv_.c_str() );
   return rv_;
   }

boost::string_ref Path::RefExt( boost::string_ref src ) { // a.k.a. IdxOfFnm()
   auto fx( Path::RefFnameExt( src ) );
   const auto idxDot( fx.find_last_of( '.' ) );
   if( idxDot == boost::string_ref::npos || idxDot == 0 ) {
      return boost::string_ref( "", 0 );
      }
   return fx.substr( idxDot );
   }

Path::str_t Path::CpyExt( boost::string_ref src ) { // if src contains an extension, prepends a leading "."
   const auto rv( Path::RefExt( src ) );
   Path::str_t rv_( rv.data(), rv.length() );           // 0 && DBG( "%s: '%s'->'%s'", __func__, src, rv_.c_str() );
   return rv_;
   }

bool Path::IsDotOrDotDot( boost::string_ref str ) { // true if str _ends with_ "\.." or "\." "\..\" or "\.\" or is "." or ".."
   const auto pC ( str.data()   );
   const auto len( str.length() );
   const auto eos( pC + len );
   return (len==1 && 0==memcmp( pC, "." , 1 ))
       || (len==2 && 0==memcmp( pC, "..", 2 ))
#define LCMP( nn, st )  (len > (nn) && 0==memcmp( eos - (nn), PATH_SEP_STR "." , (nn) ))
       || LCMP( 2, PATH_SEP_STR "." )
       || LCMP( 3, PATH_SEP_STR "." PATH_SEP_STR )
       || LCMP( 3, PATH_SEP_STR ".." )
       || LCMP( 4, PATH_SEP_STR ".." PATH_SEP_STR )
       ;
#undef  LCMP
   }

bool Path::eq( boost::string_ref name1, boost::string_ref name2 ) {
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

Path::str_t::size_type Path::CommonPrefixLen( boost::string_ref s1, boost::string_ref s2 ) {
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

Path::str_t Path::Union( boost::string_ref s1, boost::string_ref s2 ) { enum { DB=0 };
   // dest = (Path::CpyDirnm( s1 ) || Path::CpyDirnm( s2 ))
   //      + (Path::CpyFnm  ( s1 ) || Path::CpyFnm  ( s2 ))
   //      + (Path::CpyExt  ( s1 ) || Path::CpyExt  ( s2 ))
   auto dir( Path::RefDirnm( s1 ) ); if( dir.empty() )  dir = Path::RefDirnm( s2 );  // DB && DBG( "RExt:dir '%s'", dir.c_str() );
   auto fnm( Path::RefFnm  ( s1 ) ); if( fnm.empty() )  fnm = Path::RefFnm  ( s2 );  // DB && DBG( "RExt:fnm '%s'", fnm.c_str() );
   boost::string_ref ext;
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
