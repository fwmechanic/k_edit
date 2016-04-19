//
// Copyright 2015 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

GLOBAL_CONST char szBakDirNm[] = ".kbackup";

int SaveFileMultiGenerationBackup( PCChar pszFileName ) { enum { DB=0 };
   DB && DBG( "SFMG+ '%s'", pszFileName );
   struct_stat stat_buf;
   if( func_stat( pszFileName, &stat_buf ) == -1 ) { DB && DBG( "SFMG! [2] stat of '%s' FAILED!", pszFileName );
      return SFMG_NO_SRCFILE;
      }
   auto dest( std::string( BSR2STR(Path::RefDirnm( pszFileName )) ) + szBakDirNm );
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
   char tbuf[32];
   strftime( BSOB(tbuf), "%Y%m%d_%H%M%S", localtime( &stat_buf.st_mtime ) );
   const auto filenameNoPath( Path::RefFnameExt( pszFileName ) );  DB && DBG("SFMG  B '%" PR_BSR "'", BSR(filenameNoPath) );
   dest.append( (PATH_SEP_STR + std::string( BSR2STR( filenameNoPath ) ) + "." + tbuf ) );
  #if defined(_WIN32)
   unlinkOk( dest.c_str() );
  #endif
   if( rename( pszFileName, dest.c_str() ) ) {
      DB && DBG( "SFMG! [2] mv '%s' -> '%s' failed", pszFileName, dest.c_str() );
      if( mkdirLen ) { // something to undo?
         dest.resize( mkdirLen ); // retrieve dirname as ASCIZ
         WL( _rmdir, rmdir )( dest.c_str() ); // undo mkdir
         }
      return SFMG_CANT_MV_ORIG;
      }

   DB && DBG( "SFMG- [0]" );
   return SFMG_OK;
   }
