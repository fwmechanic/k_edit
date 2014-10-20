//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
//  FileIO System-call wrappers
//

#pragma once

#include "my_types.h"
#include <cstdio>

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#endif

#include <fcntl.h>
#include <errno.h>

class fio { // could use namespace vs. class but cannot declare private members of namespace
   STATIC_FXN int Read ( int fh, PVoid  pBuf, size_t bytesToRead  );
   STATIC_FXN int Write( int fh, PCVoid pBuf, size_t bytesToWrite );

public:
   STATIC_FXN bool     OpenFileFailed( int *pfh, PCChar pszFileName, bool fWrAccess, bool fCreateIfNoExist );
   STIL       bool     ReadOk(  int fh, PVoid  pBuffer, int bytes ) { return Read(  fh, pBuffer, bytes ) == bytes; }
   STIL       bool     WriteOk( int fh, PCVoid pBuffer, int bytes ) { return Write( fh, pBuffer, bytes ) == bytes; }
#if defined(_WIN32)
   STIL       int      Close(   int fh ) { return _close( fh ); }   // -1 == error, 0 == success
   STIL       int      Fsync(   int fh ) { return _commit( fh ); }  // -1 == error, 0 == success
   STIL       __int64  Lseek(   int fh, __int64 lDistanceToMove, int dwMoveMethod ) { return _lseeki64( fh, lDistanceToMove, dwMoveMethod ); }
   STIL       __int64  SeekEoF( int fh ) { return Lseek( fh, 0, SEEK_END ); }
#else
   STIL       int      Close(   int fh ) { return close( fh ); }   // -1 == error, 0 == success
   STIL       int      Fsync(   int fh ) { return fsync( fh ); }   // -1 == error, 0 == success
   STIL       off_t    Lseek(   int fh, off_t lDistanceToMove, int dwMoveMethod ) { return lseek( fh, lDistanceToMove, dwMoveMethod ); }
   STIL       off_t    SeekEoF( int fh ) { return Lseek( fh, 0, SEEK_END ); }
#endif
   STIL       void     SeekBoF( int fh ) {        Lseek( fh, 0, SEEK_SET ); }
   };

// wrappers
//
#define     unlinkOk( filename )  unlinkOk_( (filename), __func__ )
extern bool unlinkOk_( PCChar filename, PCChar caller );

#define     mkdirOk( dirname )  mkdirOk_( (dirname), __func__ )
extern bool mkdirOk_( PCChar dirname, PCChar caller );

extern bool MoveFileOk( PCChar pszCurFileName, PCChar pszNewFilename );

#define     CopyFileManuallyOk( pszCurFileName, pszNewFilename )  CopyFileManuallyOk_( pszCurFileName, pszNewFilename, __func__ )
extern bool CopyFileManuallyOk_( PCChar pszCurFileName, PCChar pszNewFilename, PCChar caller );

#define  VERBOSE_READ   0
#if      VERBOSE_READ
#  define  VR_( x ) x
#else
#  define  VR_( x )
#endif

#if 0

// read: classic look

#define kszRdNoiseOpen " Open                      "
#define kszRdNoiseSeek " Open Size                 "
#define kszRdNoiseAllc " Open Size Alloc           "
#define kszRdNoiseRead " Open Size Alloc Read      "
#define kszRdNoiseScan " Open Size Alloc Read Scan "

#else

// read: duty-cycle look

#define kszRdNoiseOpen " Open                      "
#define kszRdNoiseSeek "      Size                 "
#define kszRdNoiseAllc "           Alloc           "
#define kszRdNoiseRead "                 Read      "
#define kszRdNoiseScan "                      Scan "

#endif
