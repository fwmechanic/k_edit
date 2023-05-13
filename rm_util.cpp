//
// Copyright 2015-2022 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
#include "rm_util.h"
#include "my_log.h"
#include "filename.h"
#include <stdlib.h>
#include "my_strutils.h"
#include "my_fio.h"
#if defined(_WIN32)
#include "win32_base.h"
#endif

int SaveFileMultiGenerationBackup( PCChar pszFileName, const struct_stat &stat_buf ) { enum { SD=0 };
   SD && DBG( "SFMG+ '%s'", pszFileName );
   auto dest( std::string(Path::RefDirnm( pszFileName )) + kszBakDirNm );
   auto mkdirLen( dest.length() );
   NewScope { // validity of dirname
   const auto dirname( dest.c_str() );
   const auto err( WL( _mkdir( dirname ), mkdir( dirname, 0777 ) ) == -1 );
   if( !err ) { 0 && DBG( "mkdir (by %s) created dir '%s'", __func__, dirname );
     #if defined(_WIN32)
      SetFileAttrsOk( dirname, FILE_ATTRIBUTE_HIDDEN );
     #endif
      }
   else {
      mkdirLen = 0;
      switch( errno ) {
         case EEXIST: break;
         default    : DBG( "!!! mkdir (by %s) of '%s' failed, emsg='%s'", __func__, dirname, strerror( errno ) );
                      return SFMG_CANT_MK_BAKDIR;
         }
      }
   }
   struct tm tt;
   const auto fOk(
#if defined(_WIN32)
                   0       == localtime_s( &tt, &stat_buf.st_mtime )
#else
                   nullptr != localtime_r( &stat_buf.st_mtime, &tt )
#endif
                 );
   char tbuf[32];
   if( fOk ) { strftime( BSOB(tbuf), "%Y%m%d_%H%M%S", &tt ); }
   else      { bcpy( tbuf, "XlocaltimeX" ); }
   const auto filenameNoPath( Path::RefFnameExt( pszFileName ) );  SD && DBG("SFMG  B '%" PR_BSR "'", BSR(filenameNoPath) );
   dest.append( (DIRSEP_STR + std::string( filenameNoPath ) + "." + tbuf ) );
  #if defined(_WIN32)
   unlinkOk( dest.c_str() );
  #endif
   if( rename( pszFileName, dest.c_str() ) ) {
      SD && DBG( "SFMG! [2] mv '%s' -> '%s' failed", pszFileName, dest.c_str() );
      if( mkdirLen ) { // something to undo?
         dest.resize( mkdirLen ); // retrieve dirname as ASCIZ
         WL( _rmdir, rmdir )( dest.c_str() ); // undo mkdir
         }
      return SFMG_CANT_MV_ORIG;
      }

   SD && DBG( "SFMG- [0]" );
   return SFMG_OK;
   }
