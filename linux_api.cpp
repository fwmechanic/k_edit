//
// Copyright 2014-2015 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"
#include <boost/filesystem/operations.hpp>

void AssertDialog_( PCChar function, int line ) {
   fprintf( stderr, "Assertion failed, %s L %d", function, line );
   abort();
   }

Path::str_t Path::GetCwd() { // quick and dirty AND relies on GLIBC getcwd( nullptr, 0 ) semantics which are NONPORTABLE
   PChar mallocd_cwd = getcwd( nullptr, 0 );
   Path::str_t rv( mallocd_cwd );
   free( mallocd_cwd );                       DBG( "%s=%s'", __func__, rv.c_str() );
   return rv;
   }

PCChar OsVerStr() { return "Linux"; }

Path::str_t Path::Absolutize( PCChar pszFilename ) {  enum { DEBUG_FXN = 0 };
   // first approximation based on
   // http://stackoverflow.com/questions/1746136/how-do-i-normalize-a-pathname-using-boostfilesystem
   // note that this handles the case where some trailing part does not exist
   // contrast with canonical() which requires that the passed name exists
   const auto src( absolute( boost::filesystem::path( pszFilename ) ) );
                                               DBG( "%s src'%s' -> '%s'", __func__, pszFilename, src.c_str() );
   // Get canonical version of the existing part of src
   auto it( src.begin() );
   auto rv( *it++ );
   for( ; it != src.end() && exists(rv / *it); ++it ) {
      rv /= *it;
      }
   rv = canonical(rv);
   for( ; it != src.end(); ++it ) {  // now blindly append nonexistent components, handling "." and ".."
      if      (*it == "..") { rv = rv.parent_path(); } // "cancels" trailing rv component
      else if (*it == "." ) {}                         // nop
      else                  { rv /= *it; }             // else cat
      }

   Path::str_t destgs( rv.generic_string() );  DBG( "%s gs '%s' -> '%s'", __func__, pszFilename, destgs.c_str() );
// Path::str_t dests ( rv.string()         );  DBG( "%s s  '%s' -> '%s'", __func__, pszFilename, dests .c_str() );
   return destgs;
   }

#include <glob.h>

std::vector<std::string> glob( const std::string& pat ) { // http://stackoverflow.com/questions/8401777/simple-glob-in-c-on-unix-system
   using namespace std;
   glob_t glob_result;
   glob( pat.c_str(), GLOB_TILDE, nullptr, &glob_result );
   vector<string> ret;
   for( auto i=0u ; i < glob_result.gl_pathc ; ++i ) {
      ret.push_back( string( glob_result.gl_pathv[i] ) );
      }
   globfree( &glob_result );
   return ret;
   }

STIL bool KeepMatch_( const WildCardMatchMode want, const struct stat &sbuf, stref name ) {
   const auto fFileOk( ToBOOL(want & ONLY_FILES) );
   const auto fDirOk ( ToBOOL(want & ONLY_DIRS ) );
   const auto fIsDir ( ToBOOL(sbuf.st_mode & S_IFDIR) );
   const auto fIsFile( !fIsDir && (sbuf.st_mode & (S_IFREG | S_IFLNK)) );

   if( fDirOk  && fIsDir  ) return !Path::IsDotOrDotDot( name );
   if( fFileOk && fIsFile ) return true;
   return                          false;
   }

bool DirMatches::KeepMatch() {
   const auto name( (*d_globsIt).c_str() );
   if( stat( name, &d_sbuf ) != 0 ) {
      return false;
      }

   const auto rv( KeepMatch_( d_wcMode, d_sbuf, name ) );

   0 && DBG( "want %c%c, have %X '%s' rv=%d"
           , (d_wcMode & ONLY_DIRS ) ? 'D':'d'
           , (d_wcMode & ONLY_FILES) ? 'F':'f'
           , d_sbuf.st_mode
           , name
           , rv
           );

   return rv;
   }

const Path::str_t DirMatches::GetNext() {
   if( d_globsIt == d_globs.cend() ) // already hit no-more-matches condition?
      return Path::str_t("");

   for( ; d_globsIt != d_globs.cend() ; ++d_globsIt ) {
      if( KeepMatch() ) {
         break;
         }
      }

   if( d_globsIt == d_globs.cend() ) //         hit no-more-matches condition?
      return Path::str_t("");

   d_buf = *d_globsIt++;
   if( ToBOOL(d_sbuf.st_mode & S_IFDIR) )
      d_buf.append( PATH_SEP_STR );

   0 && DBG( "DirMatches::GetNext: '%s' (%X)", d_buf.c_str(), d_sbuf.st_mode );
   return d_buf;
   }

DirMatches::~DirMatches() {}

DirMatches::DirMatches( PCChar pszPrefix, PCChar pszSuffix, WildCardMatchMode wcMode, bool fAbsolutize )
   : d_wcMode( wcMode )
   , d_buf( Path::str_t(pszPrefix ? pszPrefix : "") + Path::str_t(pszSuffix ? pszSuffix : "") )
   { // prep the buffer
   if( fAbsolutize ) {
      d_buf = Path::Absolutize( d_buf.c_str() );
      }
   d_globs = glob( d_buf ); int ix=0;
   // for( const auto &st : d_globs ) { DBG( "glob[%d] %s", ix++, st.c_str() ); }
   d_globsIt = d_globs.cbegin();
   }

bool FileOrDirExists( PCChar pszFileName ) {
   struct stat sbuf;
   if( stat( pszFileName, &sbuf ) != 0 ) {
      return false;
      }
   return true; // basically, anything by this name gets in the way of a file write
   }

PCChar OsErrStr( PChar dest, size_t sizeofDest ) {
   return strerror_r( errno, dest, sizeofDest );
   }
