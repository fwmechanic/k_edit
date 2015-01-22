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

//  FileIO System-call wrappers
//

#include "ed_main.h"
#include "my_fio.h"
#if defined(_WIN32)
#include "win32_base.h"
#include <share.h>
#endif
#include "my_log.h"
#include <sys/stat.h>

bool fio::OpenFileFailed( int *pfh, PCChar pszFileName, bool fWrAccess, bool fCreateIfNoExist ) {
#if defined(_WIN32)
   enum { OPEN_SH =
#if 0
          // can't open file if any process is holding a write-capable handle.
          // EX: a program is writing a file when it crashes into JIT debugger:
          // while that program is being debugged, _sopen( file, _SH_DENYWR )
          // will fail.
          _SH_DENYWR
#else
          // no restrictions means, well, no restrictions.  Above can't-open case can't happen.
          _SH_DENYNO
#endif
      };

   const auto fh( _sopen(
        pszFileName
      , _O_BINARY | (fWrAccess ? _O_RDWR : _O_RDONLY) | (fCreateIfNoExist ? _O_CREAT : 0)
      , OPEN_SH
      , 0777 // permissions relevant iff _O_CREAT specified (and subject to umask anyway)
      )
    );
#else
   const auto fh( open(
        pszFileName
      , (fWrAccess ? O_RDWR : O_RDONLY) | (fCreateIfNoExist ? O_CREAT : 0)
      , 0777 // permissions relevant iff O_CREAT specified (and subject to umask anyway)
      )
    );
#endif
   if( fh == -1 )
      return true;

   0 && DBG( "%s [%d] %c%c '%s'", __func__, fh, fWrAccess?'W':'w', fCreateIfNoExist?'C':'c', pszFileName );
   struct_stat stat;
   if( func_fstat( fh, &stat ) == 0 && 0 == (stat.st_mode & WL( _S_IFREG, S_IFREG ) ) ) {
      WL( _close, close )( fh );
      return true;
      }

   *pfh = fh;
   return false;
   }

int fio::Read( int fh, PVoid pBuf, size_t bytesToRead ) {
   auto rv( WL( _read, read )( fh, pBuf, bytesToRead ) );
   if( rv == -1 )
       rv = 0;
   return rv;
   }

int fio::Write( int fh, PCVoid pBuf, size_t bytesToWrite ) {
   auto rv( WL( _write, write )( fh, pBuf, bytesToWrite ) );  0 && DBG( "%s [%d]: %" PR_SIZET "d -> %" WL( "d", "ld" ), __func__, fh, bytesToWrite, rv );
   if( rv == -1 )
       rv = 0;
   return rv;
   }

//-----------------

bool MoveFileOk( PCChar pszCurFileName, PCChar pszNewFilename ) {
   if( rename( pszCurFileName, pszNewFilename ) == 0 ) {
      // this is the NORMAL exit path, EVEN across drive letters or local <--> NETWORK drives!
      0 && DBG( "renamed '%s' -> '%s' OK", pszCurFileName, pszNewFilename );
      return true; // we're all done!
      }

   if( !CopyFileManuallyOk( pszCurFileName, pszNewFilename ) ) {
      return false;
      }

   return unlinkOk( pszCurFileName );
   }


bool unlinkOk_( PCChar filename, PCChar caller ) {
   const auto err( WL( _unlink, unlink )( filename ) == -1 );
   if( err ) { WL(0,1) && DBG( "!!! unlink (by %s) of '%s' failed, emsg='%s'", caller, filename, strerror( errno ) ); }
   else      { 0 && DBG( "unlink (by %s) of '%s' OK", caller, filename ); }
   return !err;
   }


bool mkdirOk_( PCChar dirname, PCChar caller ) {
   if( IsDir( dirname ) ) {
      0 && DBG( "mkdir (by %s) of already existing dir '%s'", caller, dirname );
      return true;
      }

   const auto err( WL( _mkdir( dirname ), mkdir( dirname, 0777 ) ) == -1 );
   if( err ) { DBG( "!!! mkdir (by %s) of '%s' failed, emsg='%s'", caller, dirname, strerror( errno ) );
      }
   else { 0 && DBG( "!!! mkdir (by %s) of '%s' successful", caller, dirname );
      if( 0 && !IsDir( dirname ) ) { DBG( "!!! mkdir (by %s) of '%s' failed, doesn't exist after successful create", caller, dirname );
         return false;
         }
      }

   return !err;
   }


bool CopyFileManuallyOk_( PCChar pszCurFileName, PCChar pszNewFilename, PCChar caller ) { DBG( "%s: manually copying", caller );
   auto cur_FILE( fopen( pszCurFileName, "rb" ) );
   if( cur_FILE == nullptr ) {  DBG( "%s(%s -> %s) fopen of src FAILED", caller, pszCurFileName, pszNewFilename );
      return false;
      }
   auto new_FILE( fopen( pszNewFilename, "w" ) );
   if( new_FILE == nullptr ) {  DBG( "%s(%s -> %s) fopen of dest FAILED", caller, pszCurFileName, pszNewFilename );
      fclose( cur_FILE );
      return false;
      }
   char copyBuf[32 * 1024];
   auto fOk(true);
   size_t bytesRead;
   while( (bytesRead=fread( copyBuf, sizeof copyBuf, 1, cur_FILE )) != 0 ) {
      if( bytesRead != fwrite( copyBuf, 1, bytesRead, new_FILE ) ) {  DBG( "%s(%s, %s) fwrite FAILED", caller, pszCurFileName, pszNewFilename );
         fOk = false;
         break;
         }
      }
   fclose( cur_FILE );
   fclose( new_FILE );
   return fOk;
   }
