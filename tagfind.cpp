//
// Copyright 2018 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
   auto bsearch_sorted_tagsfile = [&rv_append_string,&rv_setfield_string]( FILE *ifh, stref src ) {
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

      fseek( ifh, 0, SEEK_END ); auto oMax( ftell( ifh ) );
      fseek( ifh, 0, SEEK_SET ); auto oMin( ftell( ifh ) );
      const auto maxProbes( static_cast<int>( std::log2( static_cast<double>(oMax) ) ) );

      stref line; char *buf( nullptr ); size_t bufsize( 0 );
#ifdef UNITTEST
      size_t bytesRead( 0 );
#endif
      auto getline_dtor = [&buf,&bufsize]() { free( buf ); buf = nullptr; bufsize = 0; };
      auto getline = [&ifh,&buf,&bufsize,&line
#ifdef UNITTEST
         ,&bytesRead
#endif
         ]() {
         const size_t alloc_incr( 2*1024 );
         if( !buf ) {
            buf = static_cast<char *>( malloc( alloc_incr ) );
            bufsize = alloc_incr;
            }
         char  *pb( buf );
         size_t bb( bufsize );
         for( ; ; ) {
            if( fgets( pb, bb, ifh ) == nullptr ) { // eof, error?
               if( ferror( ifh ) ) {
                  return false;
                  }
               if( feof( ifh ) ) {
                  *pb = '\0';  // eof before any chars read; buf not written
                  return false;
                  }
               }
            const auto segLen( strlen( pb ) );
            if( segLen == 0 || pb[segLen-1] != '\n' ) {
               pb += segLen;  // *pb == '\0'
               const auto bchars( pb - buf + 1 );  // include '\0'
               if( bchars == bufsize ) {
                  bufsize += alloc_incr;
                  buf = static_cast<char *>( realloc( buf, bufsize ) );
                  pb = buf + bchars - 1;
                  }
               bb = bufsize - (pb - buf);
               continue;
               }
            // we have finished reading a line (to a '\n', OR hit EoF before a trailing '\n')
            stref rv( buf, pb - buf + segLen );
#ifdef UNITTEST
            bytesRead += rv.length();
#endif
            // truncate any trailing EoL chars and return what's left
            const auto trimto( rv.find_first_of( "\n\r" ) );
            if( eosr != trimto ) {
               rv.remove_suffix( rv.length() - trimto );
               }
            line = rv;
            return true;
            }
         };

      struct stat statbuf;
      if( 0 == fstat( fileno( ifh ), &statbuf ) ) {
         char tmbuf[100]; const auto timeinfo( localtime( &statbuf.st_mtime ) );
         rv_setfield_string( "tagsfile_mtime", timeinfo && strftime( tmbuf, sizeof(tmbuf), "%Y%m%dT%H%M%S", timeinfo ) ? tmbuf : "strftime failed!" );
         }
      if( src.empty() || src == "<tagged-files>" ) { // linear scan to list all files of file tags
         while( getline() ) {  // NB: to run on brain-dead CMD.EXE `tagfind_c ^<tagged-files^>`
            if( line.ends_with( "\t1;\"\tfile" ) ) {
               const auto it1( line.find( '\t' ) );
               if( it1 != eosr ) {
                  line.remove_prefix( it1+1 );
                  const auto it2( line.find( '\t' ) );
                  if( it2 != eosr ) {
                     line.remove_suffix( line.length() - it2 );
                     rv_append_string( line );
                     }
                  }
               }
            }
         }
      else { // normal mode: binary search for all tags named src
         constexpr auto START_OF_FILE( 0 );
         auto getLnCmp = [&getline,&line,&src]() {
            if( !getline() ) { // will be full unless @ EOL
               DB && ::DBG( "getline() failed" );
               return -1;
               }
            auto cand( line );
            const auto oT( cand.find('\t') );
            if( oT != eosr ) {
               cand.remove_suffix( cand.length() - oT );
               }
            DB && ::DBG( "cand '%" PR_BSR "'", BSR(cand) );
            return cmp( src, cand );
            };
         auto seekGetLnCmp = [&ifh,&getline,&getLnCmp]( decltype(oMin) skTgt ) {
            fseek( ifh, skTgt, SEEK_SET );
            if( skTgt != START_OF_FILE ) { // unless seek was to START_OF_FILE...
               getline(); // ...post-seek getline has to be assumed to yield ...
               }          // ...a partial line which must be ignored/discarded
            return getLnCmp();
            };

         DB && ::DBG( "%s: -----> '%" PR_BSR "'", __func__, BSR(src) );
         constexpr auto move_back_lines( 5 );  // SWAG based on current line length
         auto probes(0);  // debug/algo verification
         while( oMin <= oMax ) {
            ++probes;
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
            if( rslt == 0 ) { // HIT!  But perhaps not FIRST hit.
               DB && ::DBG( "Match after %d probes: %" PR_BSR "'", probes, BSR(line) );
               // linear search phase: move back then search fwd to find NON-matching line, then fwd (first) to matching line(s)
               for( auto movesBack(1) ; ; ++movesBack ) {
                  const auto mvBackAmount( (1+move_back_lines)*(1+line.length()) );
                  cmpTgt = (cmpTgt > mvBackAmount) ? cmpTgt - mvBackAmount : START_OF_FILE;
                  auto mnum( 0u );
                  auto showMatch = [&mnum,&line,&rv_append_string]() {
                     ++mnum;
                     DB && ::DBG( "MATCH %d: %" PR_BSR "'", mnum, BSR(line) );
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
                           ::DBG( "T=%" PR_BSR " P%d/%d B%d L%d R%" PR_SIZET " M%d", BSR(src), probes, maxProbes, movesBack, linMiss, bytesRead, mnum );
#endif
                           getline_dtor();
                           return 0;
                           }
                        ++linMiss;
                        }
                     }
                  }
               // assert( false );
               }
            }
         }
      getline_dtor();
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
   DBG_init();
   return FindMatchingTagsLines( argc > 2 ? argv[2] : "tags", argc > 1 ? argv[1] : "EdOpBoundary" );
   }

void DBG_init() {
   Win32::SetFileApisToOEM();
   }

int DBG( char const *kszFormat, ... ) {
   va_list args;  va_start(args, kszFormat);
   vfprintf( stderr, kszFormat, args );
   fputc( '\n', stderr );
   va_end(args);
   return 1; // so we can use short-circuit bools like (DBG_X && DBG( "got here" ))
   }

#endif
