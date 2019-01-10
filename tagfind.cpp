//
// Copyright 2018-2019 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include "lua_intf_common.h"
#include <cmath>  // for std::log2
#ifdef UNITTEST
#include <cassert>
#endif

class TxtFileLineReader {
   /* Why this exists: to offer a safe, efficient interface to
      unlimited-line-length C stdio (fgets) based file-line-reading
      functionality (a C++ std::ifstream implemetation having been tried and
      found wanting in the areas of code size (1.3MB?!) and portability between
      versions of GCC).
    */
   FILE  *d_ifh;
   stref &d_line;  // ref to stref var in client's scope which we update upon successful getline call
   char  *d_buf = nullptr;
   size_t d_bufsize = 0;
   const size_t d_buf_alloc_incr;
#ifdef UNITTEST
   size_t d_bytesRead = 0;
#endif
public:
   TxtFileLineReader( FILE *ifh_, stref &line_, size_t buf_alloc_incr=2*1024 ) : d_ifh( ifh_ ), d_line(line_), d_buf_alloc_incr(buf_alloc_incr) {}
   ~TxtFileLineReader() { free( d_buf ); d_buf = nullptr; d_bufsize = 0; }
   // NB: fgetpos/fsetpos CANNOT be used (in lieu of fseet/ftell) because arithmetic on fpos_t is not defined https://stackoverflow.com/a/21042679
   typedef decltype( ::ftell( stdin ) ) ftell_rv_t;  // avoid having to declare type of ::ftell as it may change (e.g. ftello...)
   ftell_rv_t ftell() { return ::ftell( d_ifh ); }
   bool fseekFailed( ftell_rv_t skTgt, int origin  ) {
      if( fseek( d_ifh, skTgt, origin ) != 0 ) {
         ::DBG( "fseek() failed!" );
         return true;
         }
      return false;
      }
#ifdef UNITTEST
   decltype(d_bytesRead) bytesRead() const { return d_bytesRead; }
   decltype(d_bufsize)   bufsize()   const { return d_bufsize; }
#endif
   stref line() const { return d_line; }
   bool getline() {
      if( !d_buf ) {
         d_buf = static_cast<char *>( malloc( d_buf_alloc_incr ) );
         d_bufsize = d_buf_alloc_incr;
         }
      auto pinb( d_buf );
      auto avail( d_bufsize );
      for( ; ; ) {
         if( fgets( pinb, avail, d_ifh ) == nullptr ) { // eof, error?
            if( ferror( d_ifh ) ) {
               return false;
               }
            if( feof( d_ifh ) ) {
               *pinb = '\0';  // eof before any chars read; d_buf not written
               return false;
               }
            }
         const auto segLen( strlen( pinb ) );
         if( segLen == 0 || pinb[segLen-1] != '\n' ) {
            pinb += segLen;  // *pinb == '\0'
            const auto bchars( (pinb - d_buf) + 1 );  // include '\0'
            if( bchars == d_bufsize ) {
               d_bufsize += d_buf_alloc_incr;
               d_buf = static_cast<char *>( realloc( d_buf, d_bufsize ) );
               pinb = d_buf + bchars - 1;
               }
            avail = d_bufsize - (pinb - d_buf);
            continue;
            }
         // we have finished reading a line (to a '\n', OR hit EoF before a trailing '\n')
         stref rv( d_buf, (pinb - d_buf) + segLen );
#ifdef UNITTEST
         d_bytesRead += rv.length();
#endif
         // truncate any trailing EoL chars and return what's left
         const auto trimto( rv.find_first_of( "\n\r" ) );
         if( eosr != trimto ) {
            rv.remove_suffix( rv.length() - trimto );
            }
         d_line = rv;
         return true;
         }
      }
   };

static bool tagfieldValInTagrec( stref line, stref fieldVal ) {
   auto oField( line.find( fieldVal ) );
   return eosr != oField && ( oField += fieldVal.length(), oField+1==line.length() || '\t'==line[oField] );
   }

static stref tagFromLine( stref line ) {
   const auto oHTab( line.find('\t') );
   if( oHTab != eosr ) {
      line.remove_suffix( line.length() - oHTab );
      }
   return line;
   }

static bool srIsDunder( stref sr ) {
   return sr.length() > 4 && sr.starts_with("__") && sr.ends_with("__");
   }

/*
   new API requirement: ( stref classnm )
   1. bin search for topmost line for which srIsDunder( tag.name )
   2. linear walk down while( srIsDunder( tag.name ) ), adding to
      retval those lines for which
      tagfieldValInTagrec( line, "class:$classnm" ) && tagfieldValInTagrec( line, "language:Python" )

   ideas:
     a. don't worry about tag.name match exactitude: rather match
        if( tag.startwith "__" and tag.endswith "__" )
        and don't care about what's in between.
        then check for

  POR:
     A. Add write tagfield->tag xref file API: performs linear scan of entire tags file. param: array of field names to gen xref
     B. pass 2 Lua arrays (tables) of strings containing (1) tag and (2) field:val selector(s)
        enqueue_compile_jobs is sample code accepting array of strings parameter; easy-peasy.

  mods:
  x. if you're gonna perform a linear scan of a potentially gigantic file: do it ONLY ONCE.
     a. already: for <tagged-files> creation (impl shared with Lua)
     b. new: for xref-file creation; creation of <tagged-files> would probably be moved to C++

 */

int FindMatchingTagsLines(
#ifdef UNITTEST
                           PCChar fnm, stref tag
#else
                           lua_State *L
#endif
                         ) { enum { DB=0 };
#ifdef UNITTEST
   auto rv_append_string = []( stref sr ) { printf( "%" PR_BSR "\n", BSR(sr) ); };
   auto rv_setfield_string = []( PCChar fieldNm, stref sr ) { DB && fprintf( stderr, "%s=%" PR_BSR "\n", fieldNm, BSR(sr) ); };
#else
   auto  fnm( S_(1) );
   stref tag( S_(2) );
   lua_newtable(L);  // result
   auto arrayAppendIdx(0);
   auto rv_append_string = [&L,&arrayAppendIdx]( stref sr ) {
      lua_pushlstring( L, sr.data(), sr.length() );
      lua_rawseti( L, -2, ++arrayAppendIdx );
      };
   auto rv_setfield_string = [&L]( PCChar fieldNm, stref sr ) {
      lua_pushlstring( L, sr.data(), sr.length() );
      lua_setfield( L, -2, fieldNm );
      };
#endif
   auto bsearch_sorted_tagsfile = [&rv_append_string,&rv_setfield_string
      ]( FILE *ifh, stref src ) {
      // Binary search of a sorted ctags (text) file becomes feasible only
      // because we're all running SSDs now, so seeks are ~free!  Given I'm
      // encountering 110MiB tag files, performance needs all the help it can
      // get.  Also, avoiding the need for a tag->file index in RAM (Lua
      // table) is a big win.
      //
      // This implementation contains SOME tags-file specific coding (so it's
      // NOT a generic "binary search of sorted text file" solution):
      // 1. the tag name starts each line, and is terminated by a HTAB.
      // 2. A file tag's filename lies between line's first and second HTAB.

#ifdef UNITTEST
      std::string ut_tagsfile_mtime;
#endif
      struct stat statbuf;
      if( 0 == fstat( fileno( ifh ), &statbuf ) ) {
         char tmbuf[100]; const auto timeinfo( localtime( &statbuf.st_mtime ) );
         PCChar tagsfile_mtime_val( timeinfo && strftime( tmbuf, sizeof(tmbuf), "%Y%m%dT%H%M%S", timeinfo ) ? tmbuf : "strftime failed!" );
         rv_setfield_string( "tagsfile_mtime", tagsfile_mtime_val );
#ifdef UNITTEST
         ut_tagsfile_mtime = tagsfile_mtime_val;
#endif
         }
      stref line;  // set by tfrdr.getline()
      TxtFileLineReader tfrdr( ifh, line );
      if( src.starts_with( '\t' ) ) { // linear scan returning all files of file tags
         // If src starts with '\t' (src is therefore an invalid tag), we will
         // return tag[1] (tag.fnm) of all tag records matching this field.  NB:
         // src must NOT contain a trailing '\t' character; this is supplied
         // conditionally below since the field specified by src might occur at
         // the end of the tag record (line) where it would not be followed by a
         // trailing '\t'.
         //
         // If src=="kind:file", names of all files having kind:file tags will
         // be returned.
#ifdef UNITTEST
         auto mnum( 0u );
#endif
         while( tfrdr.getline() ) {  // NB: to run on brain-dead CMD.EXE `tagfind_c ^<tagged-files^>`
            if( tagfieldValInTagrec( line, src ) ) {
               const auto it1( line.find( '\t' ) ); // end of tag
               if( it1 != eosr ) {
                  line.remove_prefix( it1+1 );
                  const auto it2( line.find( '\t' ) ); // end of filename
                  if( it2 != eosr ) {
                     line.remove_suffix( line.length() - it2 );
                     rv_append_string( line );
#ifdef UNITTEST
                     ++mnum;
#endif
                     }
                  }
               }
            }
#ifdef UNITTEST
         ::DBG( "tmt=%" PR_BSR " R%" PR_SIZET " b%" PR_SIZET " M%d", BSR(ut_tagsfile_mtime), tfrdr.bytesRead(), tfrdr.bufsize(), mnum );
#endif
         }
      else { // normal mode: binary search for all tags named src
         DB && ::DBG( "%s: -----> '%" PR_BSR "'", __func__, BSR(src) );
         tfrdr.fseekFailed( 0, SEEK_END ); auto oMax( tfrdr.ftell() );
         tfrdr.fseekFailed( 0, SEEK_SET ); auto oMin( tfrdr.ftell() );
         if( oMin < 0 || oMax < 0 ) {  // CID 184286 fix
            ::DBG( "ftell error!" );
            return 1;
            }
         const auto maxProbes( static_cast<int>( std::log2( static_cast<double>(oMax) ) ) );

         constexpr decltype(oMin) START_OF_FILE( 0 );
         auto getLnCmp = [&tfrdr,&line,&src]() {
            if( !tfrdr.getline() ) { // will be full unless @ EOL
               DB && ::DBG( "getline() failed" );
               return -1;
               }
            auto cand( tagFromLine( line ) );
            DB && ::DBG( "cand '%" PR_BSR "'", BSR(cand) );
            return cmp( src, cand );
            };
         auto seekGetLnCmp = [&tfrdr,&getLnCmp]( decltype(oMin) skTgt ) {
            if( tfrdr.fseekFailed( skTgt, SEEK_SET ) != 0 ) {  // CID 184290 "fix"
               return -1;
               }
            if( skTgt != START_OF_FILE ) { // unless seek was to START_OF_FILE...
               tfrdr.getline();  // ...post-seek getline has to be assumed to yield ...
               }                 // ...a partial line which must be ignored/discarded
            return getLnCmp();
            };

         constexpr auto move_back_lines( 5 );  // SWAG based on current line length
         auto probes(0);  // debug/algo verification
         while( oMin <= oMax ) {
            ++probes;
#ifdef UNITTEST
            assert( probes <= maxProbes );
#endif
            //===========================================================
            auto cmpTgt( oMin + ((oMax - oMin) / 2) );  // overflow-proof version
            DB && ::DBG( "min/pt/max=%ld/%ld/%ld", oMin, cmpTgt, oMax );
            const auto rslt( seekGetLnCmp( cmpTgt ) );
            if( rslt < 0 ) { /* handle unsigned underflow/wraparound */
               if( cmpTgt == START_OF_FILE ) {
                  break;
                  }
               oMax = cmpTgt - 1;
               }
            if( rslt > 0 ) {
            // oMin = ftell( ifh ) - 1;  Don't do this: CAUSES BUG
               oMin = cmpTgt + 1;
               }
            //===========================================================
            if( rslt == 0 ) { // HIT!  But perhaps not FIRST hit.  mvBack until we see a non-match, then linear-search fwd to (and past) matches.
               DB && ::DBG( "Match after %d probes: %" PR_BSR "'", probes, BSR(line) );
               // linear search phase: move back then search fwd to find NON-matching line, then fwd (first) to matching line(s)
               for( auto movesBack(1) ; ; ++movesBack ) {
                  const auto mvBackAmount( (1+move_back_lines)*(1+line.length()) );
                  cmpTgt = (cmpTgt > mvBackAmount) ? cmpTgt - mvBackAmount : START_OF_FILE;
                  auto mnum( 0u );
                  auto showMatch = [&mnum,&line,&rv_append_string]() {
                     ++mnum;
                     rv_append_string( line );
                     };
                  auto linMiss( 0u );  // debug/algo verification
                  if( 0 == seekGetLnCmp(cmpTgt) ) { // still matching after mvBack?  May not be first match
                     if( cmpTgt != START_OF_FILE ) {  // can mvBack further?
                        continue;  // mvBack further
                        }
                     showMatch();  // match at START_OF_FILE is first match
                     }
                  else {
                     ++linMiss; // cmp after mvBack was a miss; proceed fwd to first match
                     }
                  for(;;) { // linMiss=1 includes the miss that got us here
                     if( 0 == getLnCmp() ) {
                        showMatch();
                        }
                     else {
                        if( mnum > 0 ) { // non-match after matches seen?  We're done!
#ifdef UNITTEST
                           ::DBG( "tmt=%" PR_BSR " P%02d/%d B%d L%d R%" PR_SIZET " b%" PR_SIZET " M%d T=%" PR_BSR "", BSR(ut_tagsfile_mtime), probes, maxProbes, movesBack, linMiss, tfrdr.bytesRead(), tfrdr.bufsize(), mnum, BSR(src) );
#endif
                           return 0;
                           }
                        ++linMiss;
                        }
                     }
                  }
               // assert( false );
               }
            }
#ifdef UNITTEST
         ::DBG( "tmt=%" PR_BSR " P%02d/%d R%" PR_SIZET " b%" PR_SIZET " M%d T=%" PR_BSR "", BSR(ut_tagsfile_mtime), probes, maxProbes, tfrdr.bytesRead(), tfrdr.bufsize(), 0, BSR(src) );
#endif
         }
      return 1;
      };

   DB && ::DBG( "tag '%" PR_BSR "'", BSR(tag) );
   auto ifh( fopen( fnm, "rt" ) );
   if( ifh != nullptr ) {
      bsearch_sorted_tagsfile( ifh, tag );
      fclose( ifh );
      }
   return 1;  // return the table
   }

#ifdef UNITTEST

int main(int argc, char *argv[]) {
   return FindMatchingTagsLines( argc > 2 ? argv[2] : "tags", argc > 1 ? argv[1] : "EdOpBoundary" );
   }

int DBG( char const *kszFormat, ... ) {
   va_list args;  va_start(args, kszFormat);
   vfprintf( stderr, kszFormat, args );
   fputc( '\n', stderr );
   va_end(args);
   return 1; // so we can use short-circuit bools like (DBG_X && DBG( "got here" ))
   }

#endif
