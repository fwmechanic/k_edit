//
// Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

void AssertDialog_( PCChar function, int line ) {
   fprintf( stderr, "Assertion failed, %s L %d", function, line );
   abort();
   }

Path::str_t Path::GetCwd() { // quick and dirty AND relies on GLIBC getcwd( nullptr, 0 ) semantics which are NONPORTABLE
   PChar mallocd_cwd = getcwd( nullptr, 0 );
   Path::str_t rv( mallocd_cwd );
   free( mallocd_cwd );
   return rv;
   }

PCChar OsVerStr() { return "Linux"; }

Path::str_t Path::Absolutize( PCChar pszFilename ) {  enum { DEBUG_FXN = 0 };
   if( IsPathSepCh( pszFilename[0] ) ) {
      return pszFilename;
      }
   return GetCwd_ps() + pszFilename;
   }


STIL bool KeepMatch_( const WildCardMatchMode want, const struct stat &sbuf ) {
   const auto fFileOk( ToBOOL(want & ONLY_FILES) );
   const auto fDirOk ( ToBOOL(want & ONLY_DIRS ) );
   const auto fIsDir ( ToBOOL(sbuf.st_mode & S_IFDIR) );
   const auto fIsFile( !fIsDir && (sbuf.st_mode & (S_IFREG | S_IFLNK)) );

   if( fDirOk  && fIsDir  ) return true;
   if( fFileOk && fIsFile ) return true;
   return                          false;
   }

bool DirMatches::KeepMatch() {
   if( stat( d_dirent->d_name, &d_sbuf ) != 0 ) {
      return false;
      }

   const auto rv( KeepMatch_( d_wcMode, d_sbuf ) );

   1 && DBG( "want %c%c, have %X '%s' rv=%d"
           , (d_wcMode & ONLY_DIRS ) ? 'D':'d'
           , (d_wcMode & ONLY_FILES) ? 'F':'f'
           , d_sbuf.st_mode
           , d_dirent->d_name
           , rv
           );

   return rv;
   }

bool DirMatches::FoundNext() {
   d_dirent = readdir( d_dirp );
   if( !d_dirent ) {
      d_ixDest = std::string::npos; // flag no more matches
      return false; // failed!
      }

   return true;
   }

const Path::str_t DirMatches::GetNext() {
   if( d_ixDest == std::string::npos ) // already hit no-more-matches condition?
      return Path::str_t("");

   while( FoundNext() && !KeepMatch() )
      continue;

   if( d_ixDest == std::string::npos ) // already hit no-more-matches condition?
      return Path::str_t("");

   d_buf.replace( d_ixDest, std::string::npos, d_dirent->d_name );
   if( ToBOOL(d_sbuf.st_mode & S_IFDIR) && !Path::IsDotOrDotDot( d_buf.c_str() ) )
      d_buf.append( PATH_SEP_STR );

   0 && DBG( "DirMatches::GetNext: '%s' (%X)", d_buf.c_str(), d_sbuf.st_mode );
   return d_buf;
   }

DirMatches::~DirMatches() {
   if( d_dirp ) {
      closedir( d_dirp );
      }
   }

DirMatches::DirMatches( PCChar pszPrefix, PCChar pszSuffix, WildCardMatchMode wcMode, bool fAbsolutize )
   : d_wcMode( wcMode )
   , d_buf( Path::str_t(pszPrefix ? pszPrefix : "") + Path::str_t(pszSuffix ? pszSuffix : "") )
   { // prep the buffer
   if( fAbsolutize ) {
      d_buf = Path::Absolutize( d_buf.c_str() );
      }

   d_dirp = opendir( d_buf.c_str() );
   if( !d_dirp ) {
      d_ixDest = std::string::npos; // already hit no-more-matches condition?
      }
   else {
      d_dirent = readdir( d_dirp );
      if( !d_dirent ) {
         d_ixDest = std::string::npos; // already hit no-more-matches condition?
         }
      else {
         // hackaround
         const auto pDest( Path::StrToPrevPathSepOrNull( d_buf.c_str(), PCChar(nullptr) ) );
         if( pDest )
            d_ixDest = (pDest - d_buf.c_str()) + 1; // point past pathsep
         else
            d_ixDest = 0;
         }
      }
   }


bool FileOrDirExists( PCChar pszFileName ) {
   struct stat sbuf;
   if( stat( pszFileName, &sbuf ) != 0 ) {
      return false;
      }
   return true; // basically, anything by this name gets in the way of a file write
   }

PCChar OsErrStr( PChar dest, size_t sizeofDest ) {
   return (0==strerror_r( errno, dest, sizeofDest )) ? dest : nullptr;
   }
