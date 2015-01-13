//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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

int SaveFileMultiGenerationBackup( PCChar pszFileName ) {
   0 && DBG( "SFMG+ '%s'", pszFileName );
   {
   FileAttribs fa( pszFileName );
   if( !fa.Exists() ) { 0 && DBG( "SFMG! [1] noFile" );
      return SFMG_NO_EXISTING;
      }
   if( fa.IsReadonly() ) { 0 && DBG( "SFMG! [2] ROfile" );
      return SFMG_CANT_MV_ORIG;
      }
   }

   auto dest( std::string( BSR2STR(Path::RefDirnm( pszFileName )) ) + szBakDirNm );
   {
   FileAttribs fd( dest.c_str() );
   if( fd.Exists() && fd.IsDir() ) {
      }
   else {
      if( !mkdirOk( dest.c_str() ) ) { 0 && DBG( "SFMG! [2] Cant Mkdir" );
         return SFMG_CANT_MK_BAKDIR;
         }
#if defined(_WIN32)
      SetFileAttrsOk( dest.c_str(), FILE_ATTRIBUTE_HIDDEN );
#endif
      0 && DBG("SFMG  mkdir '%s'", dest.c_str() );
      }
   }
   const auto filenameNoPath( Path::RefFnameExt( pszFileName ) );  0 && DBG("SFMG  B '%" PR_BSR "'", BSR(filenameNoPath) );

   char tbuf[32];
   {
   struct_stat stat_buf;
   if( func_stat( pszFileName, &stat_buf ) == -1 ) { 0 && DBG( "SFMG! [2] stat of '%s' FAILED!", pszFileName );
      return SFMG_CANT_MV_ORIG;
      }
   strftime( BSOB(tbuf), "%Y%m%d_%H%M%S", localtime( &stat_buf.st_mtime ) );
   }
   dest += (PATH_SEP_STR + std::string( BSR2STR( filenameNoPath ) ) + "." + tbuf);

   unlinkOk( dest.c_str() );
   if( !MoveFileOk( pszFileName, dest.c_str() ) ) { 0 && DBG( "SFMG! [2] mv '%s' -> '%s' failed", pszFileName, dest.c_str() );
      return SFMG_CANT_MV_ORIG;
      }

   0 && DBG( "SFMG- [0]" );
   return SFMG_OK;
   }
