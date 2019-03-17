//
// Copyright 2015-2019 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include "ed_main.h"

bool ARG::true_ () { return true ; }
bool ARG::false_() { return false; }

bool ARG::wrstatefile() {
   WriteStateFile();
   return true;
   }

bool ARG::cwd() {
   return PushVariableMacro( Path::GetCwd().c_str() );
   }

bool ARG::dirty() {
   return g_CurFBuf()->IsDirty();
   }

bool ARG::seteol() {
   // d_cArg == 1: set UNIX EOL mode
   // d_cArg >= 2: set DOS  EOL mode
   return g_CurFBuf()->SetEolModeChanged( d_cArg >= 2 ? EolCRLF : EolLF );
   }

#if defined(_WIN32)
bool FBUF::MakeDiskFileWritable() {
   auto fFileROAttrChanged( false );
   if( FnmIsDiskWritable() ) {
      FileAttribs fa( Name() );
      if( fa.Exists() && fa.IsReadonly() ) {
         if( fa.MakeWritableFailed( Name() ) ) {
            return Msg( "Could not make '%s' writable!", Name() );
            }
         fFileROAttrChanged = true;
         }
      }
   SetDiskRW();
   if( !IsNoEdit() ) { // buffer already RW?
      if( fFileROAttrChanged ) {
         return !Msg( "%s buf was RW, diskfile now RW", Name() );
         }
      return Msg( "%s buf was RW, did nothing", Name() );
      }
   SetDiskRW();
   ClrNoEdit();
   Msg( "%s buf now RW, diskfile %s RW"
      , Name()
      , fFileROAttrChanged ? "now" : "was"
      );
   return true;
   }
#endif


#ifdef fn_rw

bool ARG::rw() {
   return g_CurFBuf()->MakeDiskFileWritable();
   }

#endif

//-------------------------------------------------------------------------------------------------

#ifdef fn_mergefilenames

bool ARG::mergefilenames() {
   switch( d_argType ) {
      case TEXTARG: {
           linebuf lbuf;
           Path::Union( BSOB(lbuf), d_textarg.pText, g_CurFBuf()->Name() );
           Msg( "%s", lbuf );
           return true;
           }
      case NOARG: {
           pathbuf drive, dir, fname, ext;
           _splitpath( g_CurFBuf()->Name(), drive, dir, fname, ext );
           Msg( "%s | %s | %s | %s", drive, dir, fname, ext );
           return true;
           }
      default:
           return BadArg();
      }
   }

#endif

#ifdef fn_nonseq_gap

STATIC_FXN bool strs_diff_by_1( stref s1, stref s2, UI base=10 ) { enum { DB=1 };
   int e1; uintmax_t v1; stref a1; UI b1; std::tie( e1, v1, a1, b1 ) = conv_u( s1, base );
   int e2; uintmax_t v2; stref a2; UI b2; std::tie( e2, v2, a2, b2 ) = conv_u( s2, base );
   const bool rv( e1 || e2 ? false : ((v1+1 == v2) || (v1 == v2+1)) );
   DB && DBG( "%s: %d '%" PR_BSR "'!'%" PR_BSR "'=%ju;%d vs '%" PR_BSR "'!'%" PR_BSR "'=%ju;%d", __func__, rv, BSR(s1), BSR(a1), v1, e1, BSR(s2), BSR(a2), v2, e2 );
   return rv;
   }

bool ARG::nonseq_gap() {
   LINE yMin, yMax;  GetLineRange  ( &yMin, &yMax );
   COL  xMin, xMax;  GetColumnRange( &xMin, &xMax );
   const auto possible(yMax - yMin);
   auto num_diff(0);
   for( auto iy(yMin) ; iy < yMax; ++iy ) {
      const auto sr1( g_CurFBuf()->PeekRawLineSeg( iy  , xMin, xMax ) );
      const auto sr2( g_CurFBuf()->PeekRawLineSeg( iy+1, xMin, xMax ) );
      if(  !IsStringBlank( sr1 )
        && !IsStringBlank( sr2 )
        && !strs_diff_by_1( sr1, sr2 )
        ) {
         g_CurFBuf()->InsBlankLinesBefore( iy+1 );
         ++iy, ++yMax, ++num_diff;
         }
      }
   Msg( "%d of %d nonsequential lines", num_diff, possible );
   return num_diff > 0;
   }

#endif
