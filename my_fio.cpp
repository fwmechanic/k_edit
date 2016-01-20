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

bool fio::OpenFileFailed( int *pfh, PCChar pszFileName, bool fWrAccess, int create_mode ) {
   create_mode &= 0777;  // mask bits extraneous to our use (e.g. from stat st_mode)
#if defined(_WIN32)
   const auto fh( _sopen(
        pszFileName
      , _O_BINARY | (fWrAccess ? (_O_WRONLY|_O_TRUNC) : _O_RDONLY) | (create_mode ? O_CREAT : 0)
      , _SH_DENYNO
      , create_mode // permissions relevant iff _O_CREAT specified (and subject to umask anyway)
      )
    );
#else
   const auto fh( open(
        pszFileName
      , (fWrAccess ? (O_WRONLY|O_TRUNC) : O_RDONLY) | (create_mode ? O_CREAT : 0)
      , create_mode // permissions relevant iff O_CREAT specified (and subject to umask anyway)
      )
    );
#endif
   if( fh == -1 ) {
      return true;
      }
   1 && DBG( "%s [%d] %c mode=%03o '%s'", __func__, fh, fWrAccess?'W':'w', create_mode, pszFileName );
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
   if( rv == -1 ) {
       rv = 0;
       }
   return rv;
   }

int fio::Write( int fh, PCVoid pBuf, size_t bytesToWrite ) {
   auto rv( WL( _write, write )( fh, pBuf, bytesToWrite ) );  0 && DBG( "%s [%d]: %" PR_SIZET "d -> %" WL( "d", "ld" ), __func__, fh, bytesToWrite, rv );
   if( rv == -1 ) {
       rv = 0;
       }
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
