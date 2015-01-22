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

extern int g_iMaxUndel;

GLOBAL_CONST char szBakDirNm[] = ".kbackup";

int SaveFileMultiGenerationBackup( PCChar pszFileName ) { enum { DB=0 };
   DB && DBG( "SFMG+ '%s'", pszFileName );
   {
   FileAttribs fa( pszFileName );
   if( !fa.Exists() ) { DB && DBG( "SFMG! [1] noFile" );
      return SFMG_NO_EXISTING;
      }
   if( fa.IsReadonly() ) { DB && DBG( "SFMG! [2] ROfile" );
      return SFMG_CANT_MV_ORIG;
      }
   }

   auto dest( std::string( BSR2STR(Path::RefDirnm( pszFileName )) ) + szBakDirNm );
   {
   FileAttribs fd( dest.c_str() );
   if( fd.Exists() && fd.IsDir() ) {
      }
   else {
      if( !mkdirOk( dest.c_str() ) ) { DB && DBG( "SFMG! [2] Cant Mkdir" );
         return SFMG_CANT_MK_BAKDIR;
         }
#if defined(_WIN32)
      SetFileAttrsOk( dest.c_str(), FILE_ATTRIBUTE_HIDDEN );
#endif
      DB && DBG("SFMG  mkdir '%s'", dest.c_str() );
      }
   }
   const auto filenameNoPath( Path::RefFnameExt( pszFileName ) );  DB && DBG("SFMG  B '%" PR_BSR "'", BSR(filenameNoPath) );

   char tbuf[32];
   {
   struct_stat stat_buf;
   if( func_stat( pszFileName, &stat_buf ) == -1 ) { DB && DBG( "SFMG! [2] stat of '%s' FAILED!", pszFileName );
      return SFMG_CANT_MV_ORIG;
      }
   strftime( BSOB(tbuf), "%Y%m%d_%H%M%S", localtime( &stat_buf.st_mtime ) );
   }
   dest += (PATH_SEP_STR + std::string( BSR2STR( filenameNoPath ) ) + "." + tbuf);

   unlinkOk( dest.c_str() );
   if( !MoveFileOk( pszFileName, dest.c_str() ) ) { DB && DBG( "SFMG! [2] mv '%s' -> '%s' failed", pszFileName, dest.c_str() );
      return SFMG_CANT_MV_ORIG;
      }

   DB && DBG( "SFMG- [0]" );
   return SFMG_OK;
   }
