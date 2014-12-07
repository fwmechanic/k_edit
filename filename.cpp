//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/naming_a_file.asp
//

#include "ed_main.h"
#include <regex>

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

boost::string_ref Path::RefFnameExt( boost::string_ref src ) {
   return src.substr( DirnmLen( src ) );
   }

boost::string_ref Path::RefFnm( boost::string_ref src ) { // a.k.a. IdxOfFnm()
   auto fx( Path::RefFnameExt( src ) );
   const auto idxDot( fx.find_last_of( '.' ) );
   if( idxDot == boost::string_ref::npos || idxDot == 0 ) {
      return fx;
      }
   return fx.substr( 0, idxDot );
   }

boost::string_ref Path::RefExt( boost::string_ref src ) { // a.k.a. IdxOfFnm()
   auto fx( Path::RefFnameExt( src ) );
   const auto idxDot( fx.find_last_of( '.' ) );
   if( idxDot == boost::string_ref::npos || idxDot == 0 ) {
      return boost::string_ref( "", 0 );
      }
   return fx.substr( idxDot );
   }

STATIC_FXN PCChar PPastLastPathSep( PCChar pPath ) {
   const auto p0( pPath );
   decltype(pPath) pNxt;
   while( *(pNxt=StrToNextOrEos( pPath, PATH_SEP_SRCH_STR ":" )) != 0 ) {
      pPath = pNxt + 1;
      }

   0 && DBG( "PPastLastPathSep '%s' -> '%s' (%" PR_SIZET "u)", p0, pPath, DirnmLen( p0 ) );
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

// bool Path::IsDotOrDotDot( PCChar pC ) { // true if pC _ends with_ "\.." or "\." "\..\" or "\.\" or is "." or ".."
//    const auto eos( Eos( pC ) );
//    const auto len( eos - pC );
//    return (len ==1 && 0==strcmp( pC     , "."  ))
//        || (len ==2 && 0==strcmp( pC     , ".." ))
//        || (len > 2 && 0==strcmp( eos - 2, PATH_SEP_STR "." ))
//        || (len > 3 && 0==strcmp( eos - 3, PATH_SEP_STR "." PATH_SEP_STR ))
//        || (len > 3 && 0==strcmp( eos - 3, PATH_SEP_STR ".." ))
//        || (len > 4 && 0==strcmp( eos - 4, PATH_SEP_STR ".." PATH_SEP_STR ))
//        ;
//    }

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

Path::str_t Path::CpyDirnm( PCChar pSrcFullname ) {           0 && DBG( "%s  in =%s'", __func__, pSrcFullname );
   const auto rv( Path::RefDirnm( pSrcFullname ) );
   return Path::str_t( rv.data(), rv.length() );
   }

Path::str_t Path::CpyFnameExt( PCChar pSrcFullname ) {
   const auto rv( Path::RefFnameExt( pSrcFullname ) );
   return Path::str_t( rv.data(), rv.length() );
   }

Path::str_t Path::CpyFnm( PCChar pSrcFullname ) { // DOES NOT include the trailing "." !!!
#if 1
   const auto rv( Path::RefFnm( pSrcFullname ) );
   Path::str_t rv_( rv.data(), rv.length() );           0 && DBG( "%s: '%s'->'%s'", __func__, pSrcFullname, rv_.c_str() );
   return rv_;
#else
   const auto pC( PPastLastPathSep( pSrcFullname ) );
   auto pEnd( Path::IsDotOrDotDot( pC ) ? Eos( pC ) : PAtLastDotOrEos( pC ) );
   return Path::str_t( pC, pEnd - pC );
#endif
   }

Path::str_t Path::CpyExt( PCChar pSrcFullname ) { // if pSrcFullname contains an extension, prepends a leading "."
#if 1
   const auto rv( Path::RefExt( pSrcFullname ) );
   Path::str_t rv_( rv.data(), rv.length() );           0 && DBG( "%s: '%s'->'%s'", __func__, pSrcFullname, rv_.c_str() );
   return rv_;
#else
   const auto pC( PPastLastPathSep( pSrcFullname ) );
   if( Path::IsDotOrDotDot( pC ) ) {
      return Path::str_t( "" );
      }
   return Path::str_t( PAtLastDotOrEos( pC ) );
#endif
   }

//----------------

Path::str_t Path::Union( PCChar pFirst, PCChar pSecond ) { enum { DB=0 };
   // dest = (Path::CpyDirnm( pFirst ) || Path::CpyDirnm( pSecond ))
   //      + (Path::CpyFnm  ( pFirst ) || Path::CpyFnm  ( pSecond ))
   //      + (Path::CpyExt  ( pFirst ) || Path::CpyExt  ( pSecond ))
   auto dir( Path::CpyDirnm( pFirst ) ); if( dir.empty() )  dir = Path::CpyDirnm( pSecond );  DB && DBG( "RExt:dir '%s'", dir.c_str() );
   auto fnm( Path::CpyFnm  ( pFirst ) ); if( fnm.empty() )  fnm = Path::CpyFnm  ( pSecond );  DB && DBG( "RExt:fnm '%s'", fnm.c_str() );
   if( !Path::IsDotOrDotDot( fnm.c_str() ) ) {
      auto ext( Path::CpyExt( pFirst ) ); if( ext.empty() )  ext = Path::CpyExt( pSecond );
      fnm += ext;                                                                             DB && DBG( "RExt:f+x '%s'", fnm.c_str() );
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
