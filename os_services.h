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

// where .h code that is "more common than not, but still OS related" should be put

#pragma once

#if defined(_WIN32)
   #define FNM_CASE_SENSITIVE 0
#else
   #define FNM_CASE_SENSITIVE 1
#endif


#if defined(_WIN32)
   #include <direct.h>
#else
   #include <unistd.h>
   #include <sys/time.h>
   #include <sys/types.h>
   #include <dirent.h>
#endif

#if defined(_WIN32)
   #include "win32_base.h"
#else
   #include "linux_base.h"
#endif

// sw breakpoint, invokes debugger
#if defined(__GNUC__)
#define  SW_BP     __asm__("int $3");
#else
#define  SW_BP     _asm { int 3 }
#endif

STIL int SW_CBP()  { extern bool g_fBpEnabled; if( g_fBpEnabled ) { SW_BP } return 1; }

#if defined(_WIN32)
   #if USE_STAT64
      typedef struct __stat64 struct_stat;
      STIL int func_stat( const char *path, struct_stat *buf ) { return _stat64( path, buf ); }
      STIL int func_fstat( int fd, struct_stat *buf )          { return _fstat64( fd, buf ); }
   #else
      typedef struct _stat struct_stat;
      STIL int func_stat( const char *path, struct_stat *buf ) { return _stat( path, buf ); }
      STIL int func_fstat( int fd, struct_stat *buf )          { return _fstat( fd, buf ); }
   #endif
#else
   typedef struct stat struct_stat;
   STIL int func_stat( const char *path, struct_stat *buf ) { return stat( path, buf ); }
   STIL int func_fstat( int fd, struct_stat *buf )          { return fstat( fd, buf ); }
#endif

STIL void SleepMs( int ms ) {
#if defined(_WIN32)
   Win32::Sleep( ms );
#else
   usleep( ms * 1000 );
#endif
   }


class FileAttribs { // for lower-level usage where a sequence of checks may be needed
   // this class should cease to exist:
   // http://stackoverflow.com/questions/4908043/what-is-the-best-way-to-check-a-files-existence-and-file-permissions-in-linux-u
   //
   //   "The only _correct_ way to check if a file exists is to try to open it.  The
   //   only _correct_ way to check if a file is writable is to try to open it for
   //   writing.  Anything else is a race condition.  (Other API calls can tell you
   //   if the file existed a moment ago, but even if it did, it might not exist
   //   those 15 nanoseconds later, when you try to actually open it, so they're
   //   largely useless)"
   //
public:
   FileAttribs( PCChar pszFileName )
#if defined(_WIN32)
      : d_attrs( Win32::GetFileAttributes( pszFileName ) )
      {}
   bool Exists()     const { return -1 != d_attrs; } // ALWAYS prequalify the remaining accessors with Exists()!
   bool IsReadonly() const { return  ToBOOL(d_attrs & FILE_ATTRIBUTE_READONLY ); }
   bool IsDir()      const { return  ToBOOL(d_attrs & FILE_ATTRIBUTE_DIRECTORY); }
   bool IsFile()     const { return  ToBOOL(d_attrs & (FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY)); }
   private:
   const Win32::DWORD d_attrs;
#else
      : d_stat_rv( func_stat( pszFileName, &d_stat ) )
      {}
   bool Exists()     const { return -1 != d_stat_rv && ToBOOL(d_stat.st_mode & (S_IFREG|S_IFDIR) ); } // ALWAYS prequalify the remaining accessors with Exists()!
   bool IsReadonly() const { return false; /* only way to know is to try */ }
   bool IsDir()      const { return  ToBOOL(d_stat.st_mode & S_IFDIR); }
   bool IsFile()     const { return  ToBOOL(d_stat.st_mode & S_IFREG); }
   private:
   struct_stat d_stat;
   int d_stat_rv; // == -1 means error
#endif

public:

#if defined(_WIN32)
   int  Attrs()      const { return         d_attrs; } // ONLY if you need to SetFileAttributes!
   bool MakeWritableFailed( PCChar pszFileName ) const { return !Win32::SetFileAttributes( pszFileName, (Attrs() & ~FILE_ATTRIBUTE_READONLY) ); }
#else
// int  Attrs()      const { return         d_attrs; } // ONLY if you need to SetFileAttributes!
   bool MakeWritableFailed( PCChar pszFileName );
#endif

   NO_COPYCTOR(FileAttribs);
   NO_ASGN_OPR(FileAttribs);
   };

//----------------------------------------------------------------
// higher-level (one-shot) ops

STIL   bool   IsDir(  PCChar name )                      { FileAttribs at(name); return at.Exists() && at.IsDir(); }
STIL   bool   IsFile( PCChar name )                      { FileAttribs at(name); return at.Exists() && at.IsFile(); }
STIL   bool   SetFileAttrsOk( PCChar fnm, int attrs )    {
#if defined(_WIN32)
   return ToBOOL( Win32::SetFileAttributes( fnm, attrs ) );
#else
   return false; // BUGBUG
#endif
   }

//----------------------------------------------------------------
// filename checkers

STIL   sridx  ixFirstWildcardOrEos( stref psz )          { return nposToEnd( psz, psz.find_first_of( "?*" ) ); }
STIL   bool   HasWildcard( stref psz )                   { return !atEnd( psz, ixFirstWildcardOrEos( psz ) ); }

STIL   sridx  ixFirstLogicalWildcardOrEos( stref psz )   { return nposToEnd( psz, psz.find_first_of( "?*|" ) ); }
STIL   bool   FnmIsLogicalWildcard( stref psz )          { return !atEnd( psz, ixFirstLogicalWildcardOrEos( psz ) ); }

//##############################################################################

class PerfCounter {

protected:
   typedef WL( Win32::LARGE_INTEGER, struct timeval ) TCapture;

private:

#if defined(_WIN32)
   STATIC_CONST TCapture s_PcFreq;
   STATIC_FXN TCapture FxnQueryPerformanceFrequency();
#else
   STATIC_FXN double TCapture_to_double( const TCapture &tval ) {
      return double( tval.tv_sec ) + double( tval.tv_usec / 1e6 );
      }
#endif

protected:

   typedef enum  { stopped, running, paused } ePCstate;

private:

   ePCstate d_state;

   double               d_acc;    // accumulated time prior to most recent d_start capture
   TCapture d_start;

   double Interval( TCapture begin, TCapture end ) const {
      return WL( (double(end.QuadPart - begin.QuadPart) / s_PcFreq.QuadPart), (TCapture_to_double(end) - TCapture_to_double(begin)) );
      }
protected:
   STATIC_FXN void GetTOD( TCapture *dest ) {
      WL( QueryPerformanceCounter( dest ), gettimeofday( dest, nullptr ) );
      }

   protected:

   ePCstate state() const { return d_state; }
   void   Start_    ( const TCapture &now ) { d_state = running;  d_start = now; }
   double Capture_  ( const TCapture &now, ePCstate new_state=running )
                    {
                    if( d_state == running )
                       {
                       d_acc += Interval( d_start, now );
                       d_start = now;  // reset starting point for next nowure
                       d_state = new_state;
                       }
                    return d_acc;
                    }

   public: //===================================================================

    PerfCounter() { Reset(); }
   ~PerfCounter() { }

   void   Start()   { TCapture now; GetTOD( &now ); Start_( now ); }
   void   Reset()   { d_acc = 0; Start(); }
   void   Pause()   { Capture();  d_state = paused; }
   void   Resume()  { Start(); }
   double Capture() { TCapture now; GetTOD( &now ); return Capture_( now ); }

   };


class MainThreadPerfCounter : public PerfCounter {
   static DLinkHead <MainThreadPerfCounter> dhead;
   DLinkEntry       <MainThreadPerfCounter> dlink;

   public: //===================================================================

    MainThreadPerfCounter() { DLINK_INSERT_LAST( MainThreadPerfCounter::dhead, this, dlink ); }
   ~MainThreadPerfCounter() { DLINK_REMOVE(      MainThreadPerfCounter::dhead, this, dlink ); }

   static void PauseAll();
   static void ResumeAll();

   };

namespace ThisProcessInfo {
   void   Init();
   PCChar ExePath();  // includes trailing '\'
   PCChar ExeName();
#if !defined(_WIN32)
   PCChar hostname();
   PCChar euname(); // username of euid()
   int    euid();
   int    umask();
#endif
   }

// all this InterlockedExchange gyration because SetUserInterrupt() is called
// from multiple threads (whereas g_fSystemShutdownOrLogoffRequested is not...)
//

// from http://stackoverflow.com/questions/8268243/porting-interlockedexchange-using-gcc-intrinsics-only
typedef WL( Win32::LONG              ,                           uint32_t ) InterlockedExchangeRetval;
typedef WL( InterlockedExchangeRetval, volatile InterlockedExchangeRetval ) InterlockedExchangeOperand;

STIL InterlockedExchangeRetval InterlockedExchange_( InterlockedExchangeOperand& data, InterlockedExchangeOperand& new_val ) {
#if defined(_WIN32)
   return Win32::InterlockedExchange( &data, new_val );
#else
   return __sync_lock_test_and_set  ( &data, new_val );
#endif
   }

enum HaltReason { NEVER_REQUESTED, USER_INTERRUPT, CMD_ABEND, USER_CHOSE_EARLY_CMD_TERMINATE };
extern  PCChar    ExecutionHaltReason();
STIL    void      SetUserInterrupt_( InterlockedExchangeOperand reason )  { extern InterlockedExchangeOperand s_fHaltExecution; InterlockedExchange_( s_fHaltExecution, reason ); }
STIL    void      ClrExecutionHaltRequest()                               { SetUserInterrupt_( NEVER_REQUESTED ); }
extern HaltReason ExecutionHaltRequested_( PCChar ident );
#define           ExecutionHaltRequested()          ExecutionHaltRequested_( __PRETTY_FUNCTION__ )

#define  RTN_rv_ON_BRK( rv )  if( !ExecutionHaltRequested() ) {} else { return rv; }
#define  RTN_false_ON_BRK     RTN_rv_ON_BRK( false )
#define  RTN_ON_BRK           RTN_rv_ON_BRK()


// called in one place each
void STIL SetUserInterrupt()              { SetUserInterrupt_( USER_INTERRUPT ); }
void STIL SetCmdAbend()                   { SetUserInterrupt_( CMD_ABEND ); }

// called as needed (rarely)
void STIL SetUserChoseEarlyCmdTerminate() { SetUserInterrupt_( USER_CHOSE_EARLY_CMD_TERMINATE ); }

STIL   PCChar OsName()    { return WL( "win32", "linux" ); }
extern PCChar OsVerStr();

//--------------------------------------------------------------------------------

extern bool   PutEnvOk(    PCChar varName, PCChar varValue );
extern bool   PutEnvOk(    PCChar szNameEqualsVal );
extern bool   PutEnvChkOk( PCChar szNameEqualsVal );

extern PCChar Getenv( stref varnm );

//--------------------------------------------------------------------------------

enum WildCardMatchMode { ONLY_FILES=1, ONLY_DIRS=2, FILES_AND_DIRS=ONLY_FILES | ONLY_DIRS };

class DirMatches {
   const WildCardMatchMode d_wcMode;
   Path::str_t             d_buf;

#if defined(_WIN32)
   size_t                  d_ixDest;
   Win32::HANDLE           d_hFindFile;
   bool                    d_fTriedFirst = false;
   Win32::WIN32_FIND_DATA  d_Win32_FindData;
#else
   std::vector<std::string>::const_iterator d_globsIt;
   std::vector<std::string>                 d_globs;
   struct stat                              d_sbuf = { 0 };
#endif

   bool FoundNext();
   bool KeepMatch();

   NO_COPYCTOR(DirMatches);
   NO_ASGN_OPR(DirMatches);

   public: //===================================================================

    DirMatches( PCChar pszPrefix, PCChar pszSuffix=nullptr, WildCardMatchMode wcMode=FILES_AND_DIRS, bool fAbsolutize=true );
   ~DirMatches();

   const Path::str_t GetNext();
   };
