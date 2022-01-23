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

#include "win32_pvt.h"

// #include <functional>

//--------------------------------------------------------------------------------------------

enum
   { TA_FAILED
   , TA_SUCCESS_CTRL_BREAK
   , TA_SUCCESS_WM_CLOSE
   , TA_SUCCESS_TERM_PROCESS
   , TA_SUCCESS_16
   };

//##############################################################################

namespace Win32 { // Win32::
   class Event { // Win32::Event
   protected:
      HANDLE d_hEvent;
      Event( bool fManualReset ) : d_hEvent( CreateEventA( nullptr,fManualReset,0,nullptr ) ) {}
      void SignalAndYield()                 { SetEvent( d_hEvent ); }
      virtual ~Event() { CloseHandle( d_hEvent ); }

   public:
      void WaitForSignalForever()                    {                        WaitForSingleObject( d_hEvent, INFINITE ); }
      bool WaitForSignalForMs_TimedOut( int waitMS ) { return WAIT_TIMEOUT == WaitForSingleObject( d_hEvent, waitMS   ); }
      void Unsignal()                                {                        ResetEvent( d_hEvent ); }
      };

   class AutoClrEvent : public Event { // Win32::AutoClrEvent
      // see http://blogs.msdn.com/oldnewthing/archive/2006/06/22/642849.aspx for insights
      public:
      AutoClrEvent() : Event( false ) {}
      void SignalOneWaiterAndYield()  { SignalAndYield(); }
      };

   class ManualClrEvent : public Event { // Win32::ManualClrEvent
      public:
      ManualClrEvent() : Event( true ) {}
      void SignalAllWaitersAndYield()  { SignalAndYield(); }
      };

   class AutoSignalEvent { // Win32::AutoSignalEvent
      ManualClrEvent &d_mce;
      public:
      AutoSignalEvent( ManualClrEvent &mce ) : d_mce( mce ) { d_mce.Unsignal(); }
      ~AutoSignalEvent()                                    { d_mce.SignalAllWaitersAndYield(); }
      };
   };

//--------------------------------------------------------------------------------------------

//
//
//#############################################################################################################################
//#####################################  Win32 PROCESS TERMINATION INTERFACE  #################################################
//#############################################################################################################################
//

enum { INVALID_dwProcessId = 0 }; // arg "win32 getprocessidofthread" google  "If the function fails, the return value is zero."  http://blogs.msdn.com/b/oldnewthing/archive/2004/02/23/78395.aspx


// based almost entirely on MSKB Q178893

namespace Win32 {

//
// SUMMARY
// In a perfect world, your process could ask another process, through some form of
// inter-process communication, to shut down.  However, if you do not have
// source-level control of the application that you wish to shut down, then you may
// not have this option.  Although there is no guaranteed "clean" way to shut down
// an application in Win32, there are steps that you can take to ensure that the
// application uses the best method for cleaning up resources.
//
// MORE INFORMATION
//
// 32-Bit Processes (and 16-Bit Processes under Windows 95) Under Win32, the
// operating system promises to clean up resources owned by a process when it shuts
// down.  This does not, however, mean that the process itself has had the
// opportunity to do any final flushes of information to disk, any final
// communication over a remote connection, nor does it mean that the process' DLL's
// will have the opportunity to execute their PROCESS_DETACH code.  This is why it
// is generally preferable to avoid terminating an application under Windows 95 and
// Windows NT.
//
// If you absolutely must shut down a process, follow these steps:
//
// 1.  Post a WM_CLOSE to all Top-Level windows owned by the process that you want
//     to shut down.  Many Windows applications respond to this message by shutting
//     down.
//
//     NOTE: A console application's response to WM_CLOSE depends on whether or not
//     it has installed a control handler.
//
//     Use EnumWindows() to find the handles to your target windows.  In your
//     callback function, check to see if the windows' process ID matches the
//     process you want to shut down.  You can do this by calling
//     GetWindowThreadProcessId().  Once you have established a match, use
//     PostMessage() or SendMessageTimeout() to post the WM_CLOSE message to the
//     window.
//
// 2.  Use WaitForSingleObject() to wait for the handle of the process.  Make sure
//     you wait with a timeout value, because there are many situations in which
//     the WM_CLOSE will not shut down the application.  Remember to make the
//     timeout long enough (either with WaitForSingleObject(), or with
//     SendMessageTimeout()) so that a user can respond to any dialog boxes that
//     were created in response to the WM_CLOSE message.
//
// 3.  If the return value is WAIT_OBJECT_0, then the application closed itself
//     down cleanly.  If the return value is WAIT_TIMEOUT, then you must use
//     TerminateProcess() to shutdown the application.
//
// NOTE: If you are getting3 a return value from WaitForSingleObject() other then
// WAIT_OBJECT_0 or WAIT_TIMEOUT, use GetLastError() to determine the cause.  By
// following these steps, you give the application the best possible chance to
// shutdown cleanly (aside from IPC or user-intervention).  The 16-Bit Issue (under
// Windows NT) The preceding steps work for 16-bit applications under Windows 95,
// however, Windows NT 16-bit applications work very differently.
//
// Under Windows NT, all 16-bit applications run in a virtual DOS machine (VDM).
// This VDM runs as a Win32 process (NTVDM) under Windows NT.  The NTVDM process
// has a process ID.  You can obtain a handle to the process through OpenProcess(),
// just like you can with any other Win32 process.  Nevertheless, none of the
// 16-bit applications running in the VDM have a process ID, and therefore you
// cannot get a Process Handle from OpenProcess().  Each 16-bit application in a
// VDM has a 16-bit Task Handle and a 32-bit thread of execution.  The handle and
// thread ID can be found through a call to the function VDMEnumTaskWOWEx().  For
// additional information, please see the following article in the Microsoft
// Knowledge Base: 175030 How To Enumerate Applications in Win32 Your first, and
// most straightforward, option when shutting down a 16-bit application under
// Windows NT is to shut down the entire NTVDM process.  You can do this by
// following the steps outlined above.  You only need to know the process ID of the
// NTVDM process (see the KB article 175030 cited above to find the process ID of
// an NTVDM).  The downside of this approach is that it closes all 16-bit
// applications that are running in that VDM.  If this is not your goal, then you
// need to take another approach.
//
// If you wish to shut down a single 16-bit application within a NTVDM process,
// following are the steps you need to take:
//
// 1.  Post a WM_CLOSE to all Top-Level windows that are owned by the process, and
//     that have the same owning thread ID as the 16-bit task you want to shut
//     down.  The most effective way to do this is by using EnumWindows().  In your
//     callback function, check to see if the window's process ID and thread
//     matches the 16-bit task you want to shut down.  Remember that the process ID
//     is going to be the process ID of the NTVDM process in which the 16-bit
//     application is running.
//
// 2.  Although you have a thread ID, you have no way to wait on the termination of
//     the 16-bit process.  As a result, you must wait for an arbitrary length of
//     time (to allow a clean shut down), and then try to shut the application down
//     anyway.  If the application has already shut down, then this will nothing.
//     If it hasn't shut down, then it will terminate the application.
//
// 3.  Terminate the application using a function called VDMTerminateTaskWOW(),
//     which can be found in the Vdmdbg.dll.  It takes the process ID of the VDM
//     and the task number of the 16-bit task.  This approach allows you to shut
//     down a single 16-bit application within a VDM under Windows NT.  However,
//     16-bit Windows is not very good at cleaning up resources of a terminated
//     task, and neither is the WOWExec running in the VDM.  If you are looking for
//     the cleanest possible approach to terminating a 16-bit application under
//     Windows NT, you should consider terminating the entire VDM process.  NOTE:
//     If you are starting a 16-bit application that you may terminate later, then
//     use the CREATE_SEPARATE_WOW_VDM with CreateProcess().
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

// #include <vdmdbg.h>

struct ewShData {
   DWORD    dwPID;
   uint32_t matchingWindows;
   };

STATIC_FXN BOOL CALLBACK TerminateAppEnum( HWND hwnd, LPARAM lParam ) {
   auto sd( (ewShData *)lParam );
   DWORD dwPID;
   GetWindowThreadProcessId( hwnd, &dwPID );
   if( dwPID == sd->dwPID ) {
      ++sd->matchingWindows;
      PostMessage( hwnd, WM_CLOSE, 0, 0 );  0 && DBG( "PID match, tx WM_CLOSE" );
      }
   return TRUE;
   }

STATIC_FXN int TerminateApp( const DWORD dwPID, int TmoutMs ) {
   if( INVALID_dwProcessId == dwPID ) {
      return TA_FAILED;
      }
   const HANDLE hProc( OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, dwPID) );
   if( hProc == nullptr ) {  // If we can't open the process with the necessary rights...
      return TA_FAILED;      // ...then we give up immediately
      }
   // post WM_CLOSE to all windows whose PID matches dwPID
   ewShData sd = { dwPID };
   EnumWindows( (WNDENUMPROC)TerminateAppEnum, (LPARAM)&sd );
   DWORD dwRet( TA_FAILED );
   if( !sd.matchingWindows ) {
      dwRet = GenerateConsoleCtrlEvent( CTRL_BREAK_EVENT, dwPID ) ? TA_SUCCESS_CTRL_BREAK : TA_FAILED;
      }
   else {
      // We sent at least one WM_CLOSE, so give the receiving process some time.
      // If it self-implodes, great. If it times out, then kill it.
      if( WaitForSingleObject( hProc, TmoutMs ) == WAIT_OBJECT_0 ) {
         dwRet = TA_SUCCESS_WM_CLOSE;
         }
      }
   if( dwRet == TA_FAILED ) { // if nothing else worked, whack it over the head!
      dwRet = TerminateProcess( hProc, 0 ) ? TA_SUCCESS_TERM_PROCESS : TA_FAILED;
      }
   CloseHandle(hProc);
   return dwRet;
   }

#pragma GCC diagnostic pop

   } // namespace Win32

#define CLOSEHANDLE(hvar)  { const auto chrv( Win32::CloseHandle( hvar ) );  Assert( chrv ); hvar = nullptr; }

//
//#############################################################################################################################
//############################################  Win32 Child Process EXEC/MGMT  ################################################
//#############################################################################################################################
//

PCChar szNO_ECHO_CMDLN( int flags ) { STATIC_CONST char rv[]{ CH_NO_ECHO_CMDLN , '\0' }; return (flags & NO_ECHO_CMDLN) ? rv : nullptr; }
PCChar szIGNORE_ERROR ( int flags ) { STATIC_CONST char rv[]{ CH_IGNORE_ERROR  , '\0' }; return (flags & IGNORE_ERROR ) ? rv : nullptr; }
PCChar szNOSHELL      ( int flags ) { STATIC_CONST char rv[]{ CH_NOSHELL       , '\0' }; return (flags & NOSHELL      ) ? rv : nullptr; }

STATIC_FXN void prep_cmdline_( PChar pc ) {
   // FIXME   this is horrendous!   FIXME
   // FIXME   this is horrendous!   FIXME
   // FIXME   this is horrendous!   FIXME
   // CMD shell cannot handle '/' dirsep in argv[0], so xlat to '\'
   const auto chDelim( Path::DelimChar( pc ) );
   const auto pEoArgv0( chDelim ? strchr(pc+1,chDelim) : StrToNextBlankOrEos( pc ) );
   for( ; pc < pEoArgv0 ; ++pc ) {
      if( Path::chDirSepPosix == *pc ) {
         *pc = Path::chDirSepMS;
         }
      }
   }

STATIC_FXN void prep_cmdline( PChar pc, int cmdFlags, PCChar func__ ) {
   prep_cmdline_( pc );
   0 && DBG( "%s: %s%s%s: %s"
      , func__
      , szNO_ECHO_CMDLN( cmdFlags )
      , szIGNORE_ERROR ( cmdFlags )
      , szNOSHELL      ( cmdFlags )
      , pc
      );
   }

GLOBAL_CONST NOAUTO Win32::HANDLE g_hCurProc = Win32::GetCurrentProcess();

class TPipeReader {
   Win32::HANDLE d_RdPipeHandle;
   Win32::DWORD  d_bytesInRawBuffer;
   PChar         d_pRawBuffer;
   char          d_rawBuffer[1024];
   int           RdChar();
   enum { EMPTY = -1, SD=0 };
public:
   TPipeReader( Win32::HANDLE hReadPipe ) : d_RdPipeHandle( hReadPipe ), d_bytesInRawBuffer( 0 ), d_pRawBuffer( d_rawBuffer ) {}
   ~TPipeReader() { Win32::CloseHandle( d_RdPipeHandle ); }
   int GetFilteredLine( PXbuf xb );
   };

int TPipeReader::RdChar() { // see http://support.microsoft.com/kb/q190351/
   if( d_bytesInRawBuffer == 0 ) {
      if( FALSE == Win32::ReadFile(
                    d_RdPipeHandle
                  , BSOB(d_rawBuffer)
                  , &d_bytesInRawBuffer
                  , nullptr
                  )
        ) {
         if( SD ) {
            char erbuf[265]; auto winerr( Win32::GetLastError() );
            DBG( "'%s' -> Win32::ReadFile FAILED: %lu %s", __func__, winerr, OsErrStr( BSOB(erbuf), winerr ) );
            }
         // all write handles to the pipe are closed
         //
         d_bytesInRawBuffer = 0;
         return EMPTY;
         }
      if( d_bytesInRawBuffer == 0 ) {
         return EMPTY;
         }
      d_pRawBuffer = d_rawBuffer;
      }
   --d_bytesInRawBuffer;
   const char rv( *d_pRawBuffer++ );
   return rv;
   }

int TPipeReader::GetFilteredLine( PXbuf xb ) {
   xb->clear();
   decltype(RdChar()) lastCh(0);
   while( 1 ) {
      lastCh = RdChar();
      switch( lastCh ) {
         break;case 0x0D:  // drop CR
         break;case 0x0A:  goto END_OF_LINE; // LF signifies EOL
         break;case EMPTY: goto END_OF_LINE; // no more data available (for now)
         break;case HTAB:  { // expand to spaces            01234567
                           STATIC_CONST char tabspaces[] = "        ";
                           xb->cat( tabspaces+( xb->length() & (MAX_TAB_WIDTH-1)) );
                           }
         break;default:    xb->push_back( lastCh );    SD && DBG( "%c", lastCh );
         }
      }
END_OF_LINE:
   return !(lastCh == EMPTY && 0 == xb->length());
   }

class ConsoleSpawnHandles {
   // see KB_190351.c too
   // NB http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499%28v=vs.85%29.aspx was last updated in 2012/11
   //
   // 20100218 kgoodwin creating this class as the topic's complexity has increased
   //                   (and I have had two parallel clones of the same impl for
   //                   awhile now)
   // 20100305 kgoodwin actually _implemented_ 95% of what's shown in http://support.microsoft.com/kb/190351
   //                   Benefit: this process's StdHandles are never switched
   //                   away from, thus the "mutex" which protects other threads
   //                   from using this process's StdHandles during that
   //                   "trampoline time" is not needed.  But this new
   //                   implementation _does not_ help the GNU make issue
   //                   "process_easy: DuplicateHandle(In) failed (e=6)"
   //                   which is seen when running zbpllwt.bat
   bool                  d_fError        ;
   Win32::HANDLE         d_hInputPipeWr  ;
   Win32::HANDLE         d_hOutputPipeRd ;
   Win32::STARTUPINFO    d_si            ;
   bool DupFailed( Win32::HANDLE *pOut, Win32::HANDLE in, Win32::DWORD fInherit ) {
      return !Win32::DuplicateHandle( g_hCurProc, in, g_hCurProc, pOut, 0, fInherit, DUPLICATE_SAME_ACCESS );
      }

public:
   bool Error() const { return d_fError; }
   ConsoleSpawnHandles()
      : d_fError( true )
      {
      memset( &d_si, 0, sizeof(d_si) );
      d_si.cb = sizeof(d_si);
      d_si.dwFlags = STARTF_USESTDHANDLES;
      Win32::SECURITY_ATTRIBUTES PipeAttributes = { 0 };
      PipeAttributes.nLength = sizeof(PipeAttributes);
      PipeAttributes.lpSecurityDescriptor = nullptr;
      PipeAttributes.bInheritHandle = TRUE;
      // create a pipe to Rx compile process' stdout AND stderr
      //
      // 20100217 kgoodwin create a pipe to attach to the child's stdin; this fixes a problem with GNU Make
      //                   (MSYS) when the child's stdin handle was INVALID_HANDLE (symptoms of this included
      //                   "process_easy: DuplicateHandle(In) failed (e=6)" and "make.exe: *** [all] Error 255")
      //
      Win32::HANDLE tmp_hInputPipeWr, tmp_hOutputPipeRd;
      d_fError =
         (  !Win32::CreatePipe( &d_si.hStdInput   , &tmp_hInputPipeWr , &PipeAttributes, 0 ) || DupFailed( &d_hInputPipeWr , tmp_hInputPipeWr , FALSE ) || !Win32::CloseHandle( tmp_hInputPipeWr  )
         || !Win32::CreatePipe( &tmp_hOutputPipeRd, &d_si.hStdOutput  , &PipeAttributes, 0 ) || DupFailed( &d_hOutputPipeRd, tmp_hOutputPipeRd, FALSE ) || !Win32::CloseHandle( tmp_hOutputPipeRd )
            // Create an (inheritable) duplicate of the output write handle for
            // the stderr write handle. This is necessary in case the child
            // application closes one of its std output (stdout or stderr) handles.
         || DupFailed( &d_si.hStdError, d_si.hStdOutput, TRUE )
         );
      }
   Win32::STARTUPINFO *StartHandlesSwapCriticalSection() { return &d_si; }
   void EndHandlesSwapCriticalSection() {
      CLOSEHANDLE( d_si.hStdInput  );
      CLOSEHANDLE( d_si.hStdOutput );
      CLOSEHANDLE( d_si.hStdError  );
      0 && DBG( "closed child's stdhandles" );
      }
   Win32::HANDLE hOutPipeRd() const { return d_hOutputPipeRd; }
   Win32::HANDLE hInPipeWr()  const { return d_hInputPipeWr ; }
   void PostChildTerminate() { CLOSEHANDLE( d_hInputPipeWr ); }
   ~ConsoleSpawnHandles() {}
   };

STATIC_FXN PChar showTermReason( PChar dest, size_t sizeofDest, const Win32::DWORD hProcessExitCode, const int unstartedJobCnt, const int failedJobsIgnored, double et ) {
   if( 0 == hProcessExitCode ) {
      FmtStr<30> ets( "in %.3f S", et );
      if( failedJobsIgnored )   { _snprintf( dest, sizeofDest, "--- processing successful, %d job-failure%s ignored %s ---", failedJobsIgnored, Add_s( failedJobsIgnored ), ets.c_str() ); }
      else                      { _snprintf( dest, sizeofDest, "--- processing successful %s ---", ets.c_str() );                                                                          }
      }
   else if( Win32::Status_Control_C_Exit() == hProcessExitCode ) {
      _snprintf( dest, sizeofDest, "--- process TERMINATED with prejudice" );
      }
   else {
      STATIC_CONST char hdr[] = "--- process FAILED, last exit code=0x";
     #if 0
      char erbuf[265];
      OsErrStr( BSOB(erbuf), hProcessExitCode );
      if( erbuf[0] ) {
         if( unstartedJobCnt )  { _snprintf( dest, sizeofDest, "%s%lX (%s), %d job%s unstarted ---", hdr, hProcessExitCode, erbuf, unstartedJobCnt, Add_s( unstartedJobCnt ) ); }
         else                   { _snprintf( dest, sizeofDest, "%s%lX (%s) ---"                    , hdr, hProcessExitCode, erbuf );                                            }
         }
      else
     #endif
         {
         if( unstartedJobCnt )  { _snprintf( dest, sizeofDest, "%s%lX, %d job%s unstarted ---"     , hdr, hProcessExitCode       , unstartedJobCnt, Add_s( unstartedJobCnt ) ); }
         else                   { _snprintf( dest, sizeofDest, "%s%lX ---"                         , hdr, hProcessExitCode       );                                             }
         }
      }
   return dest;
   }

#define  USE_ConsoleInputModeRestorer  0

STATIC_FXN void PutLastLogLine( PFBUF d_pfLogBuf, PCChar msg ) {
   WhileHoldingGlobalVariableLock gvlock; // wait until we own the output resource
   CapturePrevLineCountAllWindows( d_pfLogBuf, true );
   d_pfLogBuf->PutLastLineRaw( msg );
   MoveCursorToEofAllWindows( d_pfLogBuf, true );
   }

enum CP_PIPED_RC {
   CP_PIPED_RC_OK = 0,
   CP_PIPED_RC_EPIPE,
   CP_PIPED_RC_ECREATEPROCESS,
   CP_PIPED_RC_EGETEXITCODE,
   };

STATIC_FXN CP_PIPED_RC CreateProcess_piped
   ( Win32::PROCESS_INFORMATION *pPI
   , Win32::DWORD *pd_hProcessExitCode
   , PFBUF pfLogBuf
   , PCChar pS
   , int cmdFlags
   , PXbuf CommandLine
   , PXbuf xb
   ) { // arg arg "echo hello" execute   arg arg "^ls.exe -l" execute
   if( !(cmdFlags & NO_ECHO_CMDLN) ) {
      PCChar pDisp( pS );
      if( cmdFlags & IGNORE_ERROR ) {  // need scratch buffer?
         CommandLine->FmtStr( "-%s", pDisp );
         pDisp = CommandLine->c_str();
         }
      PutLastLogLine( pfLogBuf, pDisp );
      }
   STATIC_CONST char CMD_bin[]{ "CMD.EXE" };  // NB: `CMD.exe /c file` REQUIRES 'file' have extension .bat (`new tempfile` creates extension-less file)
   PCChar shell( "" );  // { echo "hello worlds in $(pwd)" ; }
   if( !(cmdFlags & NOSHELL) ) {  // { cd .kbackup && echo "hello worlds in $(pwd)" ; }
      shell = []() {
         auto envIsFile = []( PCChar envNm ) -> PCChar {  // explicit PCChar return type to ensure return type isn't deduced as 'char *'
            const auto val( getenv( envNm ) );
            return val && IsFile( val ) ? val : nullptr;
            };
         { const auto env( envIsFile( "SHELL"   ) ); if( env ) { return env; } }
         { const auto env( envIsFile( "COMSPEC" ) ); if( env ) { return env; } }
                                                                 return CMD_bin;  // default shell
         }();
      }
   auto usingAShell( ToBOOL( shell[0] ) );
   auto shellopt( usingAShell ? " " : "" );  auto ndTempFile( usingAShell );
   if( Path::endsWith( shell, CMD_bin ) ) { shellopt = " /c "; ndTempFile = false; }
   std::unique_ptr<tempfile> tempf( ndTempFile ? new tempfile( "w" ) : nullptr );
   if( tempf && tempf->fh() ) {  // echo "$0"
      fputs( pS, tempf->fh() );
      tempf->close();
      pS = tempf->name();
      }
   CommandLine->FmtStr( "%s%s%s", shell, shellopt, pS );
   1 && DBG( "%s: CommandLine='%s'", __func__, CommandLine->c_str() );
   const auto pXeq(      CommandLine->wbuf()  );  // Win32::CreateProcessA takes PChar (not PCChar) cmdline param
   const auto pXeqConst( CommandLine->c_str() );  // for internal use
   ConsoleSpawnHandles csh;
   if( csh.Error() ) {
      PutLastLogLine( pfLogBuf, "Cannot create pipe - did not create process" );
      return CP_PIPED_RC_EPIPE;
      }
   //  ^c:\klg\bin\unxutils\ls.exe
   //  -ls -l && sleep 2 && echo hello world
   //  -ls -l
   //  -ls -l && sleep 10
   #if USE_ConsoleInputModeRestorer
   const ConsoleInputModeRestorer cim; // save for later restore, to solve "ctrl+c/+s going away" problem
   #endif
   const auto d_fChildProcessStarted( Win32::CreateProcessA( nullptr, pXeq, nullptr, nullptr, TRUE, CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, csh.StartHandlesSwapCriticalSection(), pPI ) );
   csh.EndHandlesSwapCriticalSection();
   #if USE_ConsoleInputModeRestorer
   // The following FAILS to restore this process' cim (to solve the "ctrl+c/+s
   // going away" problem) for the time period while the child process is
   // alive.  Performing cim.Set() AFTER the child process terminates DOES
   // correct the problemWhileHoldingGlobalVariableLock.
   //
   // cim.Set();
   #endif
   auto rv( CP_PIPED_RC_OK );
   if( !d_fChildProcessStarted ) {
      char erbuf[265];
      OsErrStr( BSOB(erbuf) );
      ErrorDialogBeepf( "%s: CreateProcess '%s' FAILED: %s!!!", __func__, pXeqConst, erbuf );
      rv = CP_PIPED_RC_ECREATEPROCESS;
      }
   else {
      CLOSEHANDLE( pPI->hThread );
      TPipeReader pipeReader( csh.hOutPipeRd() );
      while( 1 ) {
         if( pipeReader.GetFilteredLine( xb ) ) {
            PutLastLogLine( pfLogBuf, xb->c_str() );
            } // as long as data isn't exhausted, don't even check for process death
         else if( 0 == Win32::WaitForSingleObject( pPI->hProcess, 0 ) ) { // NO WAIT, CHECK ONLY!
            // process has terminated;
            pPI->dwProcessId = INVALID_dwProcessId; // BUGBUG RACE!
            // now try to retrieve its exitcode
            if( 0 == Win32::GetExitCodeProcess( pPI->hProcess, pd_hProcessExitCode ) ) { // API failed?
               PutLastLogLine( pfLogBuf, "GetExitCodeProcess failed" );
               rv = CP_PIPED_RC_EGETEXITCODE;
               }
            else {
               if( *pd_hProcessExitCode && (cmdFlags & IGNORE_ERROR) ) {
                  PutLastLogLine( pfLogBuf, FmtStr<64>( "-   process exit code=%ld ignored", *pd_hProcessExitCode ) );
                  }
               }
            CLOSEHANDLE( pPI->hProcess );
            break;
            }
         }
      } // kill pipeReader
   csh.PostChildTerminate();
   #if USE_ConsoleInputModeRestorer
   cim.Set(); // _if_ this is ever needed again, it may need to be done WhileHoldingGlobalVariableLock
   #endif
   return rv;
   }

struct Win32pty_job_Q_el {
   PChar    d_PtyXeqParam; // heap string owned by this object!
   int      d_cmdFlags;
   DLinkEntry<Win32pty_job_Q_el> d_dlinkJobsOfPty;
   Win32pty_job_Q_el( PChar PtyXeqParam, int cmdFlags )
      : d_PtyXeqParam( PtyXeqParam )
      , d_cmdFlags   ( cmdFlags )
      {}
   ~Win32pty_job_Q_el() { Free0( d_PtyXeqParam ); }
   };

class Win32_pty {
   STATIC_VAR Win32_pty         *s_Win32_pty_ListHead;
   NO_COPYCTOR(Win32_pty);
   NO_ASGN_OPR(Win32_pty);
   Win32::ManualClrEvent         d_AllJobsDone;
   DLinkHead<Win32pty_job_Q_el>  d_jobQHead;
   int                           d_numJobRequestsPending;
   std::mutex                    d_jobQueueMtx;
   Win32::PROCESS_INFORMATION    d_processInfo = {nullptr};
   Win32::HANDLE                 d_hThread;
   Win32::DWORD                  d_hProcessExitCode;
   PFBUF                         d_pfLogBuf;
   Win32_pty                    *d_pNext;
   // _nolock methods do not lock the queue internally; caller MUST
   bool EnqueueJobPrimeThread_nolock();
   int  DeleteAllEnqueuedJobs_locks();
   void ThreadFxnRunAllJobs();

public:
   STATIC_FXN void QuiesceAll();
   STATIC_FXN int  ActiveQueues();
   Win32_pty( PCChar pszLogBufferName );
   ~Win32_pty();
   bool WaitExeDoneTimedout( int timeoutMS=20000 );
   bool IsThreadActive() const { return nullptr != d_hThread; }
   int  JobQueueDepth()  const { return d_numJobRequestsPending; }
   int  EnqueueJobsAndRun( const StringList &sl );
   int  KillAllJobsInBkgndProcessQueue();
   };

Win32_pty *Win32_pty::s_Win32_pty_ListHead;

void Win32_pty::QuiesceAll() {
   for( auto pQ(s_Win32_pty_ListHead) ; pQ ; pQ=pQ->d_pNext ) {
      pQ->KillAllJobsInBkgndProcessQueue();
      }
   }

int Win32_pty::ActiveQueues() {
   auto sum(0);
   for( auto pQ(s_Win32_pty_ListHead) ; pQ ; pQ=pQ->d_pNext ) {
      if( pQ->IsThreadActive() ) {
         ++sum;
         }
      }
   return sum;
   }

bool KillAllBkgndProcesses() {
   Win32_pty::QuiesceAll();
   return true;
   }

#ifdef fn_killall

// 20081212 kgoodwin ctrl+break seems to kill pending <compile> process-tree just fine so disable this

bool ARG::killall() {
   switch( d_argType ) {
      default:    return BadArg();
      case NOARG: KillAllBkgndProcesses();
                  return true;
      }
   }

#endif

Win32_pty::~Win32_pty() {
   // BUGBUG  tricky, but needs doing
   }

Win32_pty::Win32_pty( PCChar pszLogBufferName )
   : d_numJobRequestsPending( 0 )
   , d_hThread              ( nullptr )
   , d_hProcessExitCode     ( 0 )
   {
   FBOP::FindOrAddFBuf( pszLogBufferName, &d_pfLogBuf );
   d_pNext = s_Win32_pty_ListHead;
   s_Win32_pty_ListHead = this;
   }

// a pointer to this fxn is passed to AddBkgndProcessToJobQueue

bool Win32_pty::WaitExeDoneTimedout( int timeoutMS ) {
   MainThreadGiveUpGlobalVariableLock();
   const auto rv( d_AllJobsDone.WaitForSignalForMs_TimedOut( timeoutMS ) );
   MainThreadWaitForGlobalVariableLock();
   return rv;
   }

void Win32_pty::ThreadFxnRunAllJobs() { // RUNS ON ONE OR MORE TRANSIENT THREADS!!!
   // this (lambda) compiles and works, but passing it to a function is NOT
   // equvalent to passing a fxn pointer; it requires #include <functional> and
   // declaring the fxn param as std::function<>, which results in .dll size
   // growth of approx 50KB(!)
   //
   // interestingly, in its currently unreferenced form it does not generate a
   // warning (unref'd local), HOWEVER if the code is changed to
   //
   //                   *
   // auto PutLastLine_ = [=]( PCChar pline ) { PutLastLogLine( d_pfLogBuf, pline ); };
   //                   *
   //
   // the warning IS generated...
   //
   auto PutLastLine_( [=,this]( PCChar pline ) { PutLastLogLine( d_pfLogBuf, pline ); } );
   if( g_fMsgflush ) {
      WhileHoldingGlobalVariableLock gvlock;     // wait until we own the I/O
      d_pfLogBuf->MakeEmpty();
      }
   auto failedJobsIgnored(0);
   auto unstartedJobCnt(0);
   PerfCounter pc;
   Xbuf x1, x2;
   Win32::AutoSignalEvent amce( d_AllJobsDone );
   while( true ) { //**************** outerthreadloop ****************
      Win32pty_job_Q_el *pEl;
      {
      std::scoped_lock<std::mutex> LockTheJobQueue( d_jobQueueMtx ); // ##################### LockTheJobQueue ######################
      d_processInfo.dwProcessId = INVALID_dwProcessId;
      DispNeedsRedrawStatLn();
      if( d_jobQHead.empty() ) { // ONLY EXIT FROM THREAD IS HERE!!!
         linebuf buf;
         PutLastLogLine( d_pfLogBuf, showTermReason( BSOB(buf), d_hProcessExitCode, unstartedJobCnt, failedJobsIgnored, pc.Capture() ) );
         d_hProcessExitCode = 0;
         d_hThread = nullptr;
         return; // ##################### LockTheJobQueue ######################
         }
      DLINK_REMOVE_FIRST(d_jobQHead, pEl, d_dlinkJobsOfPty); //*** job has been taken from Queue, held in pEl
      --d_numJobRequestsPending;
      } // ##################### LockTheJobQueue ######################
      const auto cp_rc( CreateProcess_piped( &d_processInfo, &d_hProcessExitCode, d_pfLogBuf, pEl->d_PtyXeqParam, pEl->d_cmdFlags, &x1, &x2 ) );
      if( CP_PIPED_RC_OK == cp_rc ) {
         if( d_hProcessExitCode ) {
            if( pEl->d_cmdFlags & IGNORE_ERROR ) {  ++failedJobsIgnored;                             }
            else                                 {  unstartedJobCnt = DeleteAllEnqueuedJobs_locks(); }
            }
         }
      Delete0( pEl );
      } //**************** outerthreadloop ****************
   ConOut::Bell();
   }

STATIC_CONST auto cpct_start_fmts = "%s::CPCT vvvvvvvvvvvvvvvvvv THREAD STARTS vvvvvvvvvvvvvvvvvv";
STATIC_CONST auto cpct_exit_fmts  = "%s::CPCT ^^^^^^^^^^^^^^^^^^ THREAD EXITS  ^^^^^^^^^^^^^^^^^^";

bool Win32_pty::EnqueueJobPrimeThread_nolock() {
   if( !IsThreadActive() ) {
      Assert( d_pfLogBuf );
      std::thread cpct( Win32_pty::ThreadFxnRunAllJobs, this ); cpct.detach();
      return true;
      }
   return false;
   }

int Win32_pty::EnqueueJobsAndRun( const StringList &sl ) {
   auto numAdded(0);
   std::scoped_lock<std::mutex> LockTheJobQueue( d_jobQueueMtx );
   const auto fNeedToPrime( 0 == d_numJobRequestsPending );
   DLINKC_FIRST_TO_LASTA( sl.d_head, dlink, pCur ) {
      DBG( "%s '%s'", __func__,  pCur->string );
      auto flags(0);
      auto pf( pCur->string + xlat_cmdline_flag_chars( pCur->string, &flags ) );
      if( *pf ) {
         const auto pCD( Strdup( pf ) );
         prep_cmdline( pCD, flags, __func__ );  // todo: maybe defer this to subprocess-start-time?
         auto pEl( new Win32pty_job_Q_el( pCD, flags ) );
         DLINK_INSERT_LAST(d_jobQHead, pEl, d_dlinkJobsOfPty);
         ++d_numJobRequestsPending;
         ++numAdded;
         }
      }
   if( fNeedToPrime && numAdded > 0 ) {
      EnqueueJobPrimeThread_nolock();
      }
   return numAdded;
   }

int Win32_pty::DeleteAllEnqueuedJobs_locks() {
   std::scoped_lock<std::mutex> LockTheJobQueue( d_jobQueueMtx );
   const auto rmCnt( d_jobQHead.length() );
   while( auto pEl = d_jobQHead.front() ) {
      DLINK_REMOVE_FIRST( d_jobQHead, pEl, d_dlinkJobsOfPty );
      Delete0( pEl );
      }
   d_numJobRequestsPending -= rmCnt;
   Assert( d_numJobRequestsPending == 0 );
   return rmCnt;
   }

int Win32_pty::KillAllJobsInBkgndProcessQueue() {
   DeleteAllEnqueuedJobs_locks();
   if( IsThreadActive() && ConIO::Confirm( Sprintf2xBuf( "Kill background %s process (PID=%ld)?", d_pfLogBuf->Name(), d_processInfo.dwProcessId ) ) ) {
      PCChar msg;
      switch( Win32::TerminateApp( d_processInfo.dwProcessId, 2000 ) ) {
         break;default                      : msg = "WTF!?"                    ;
         break;case TA_FAILED               : msg = "TA_FAILED"                ;
         break;case TA_SUCCESS_CTRL_BREAK   : msg = "TA_SUCCESS_CTRL_BREAK"    ;
         break;case TA_SUCCESS_WM_CLOSE     : msg = "TA_SUCCESS_WM_CLOSE"      ;
         break;case TA_SUCCESS_TERM_PROCESS : msg = "TA_SUCCESS_TERM_PROCESS"  ;
         }
      Msg( "Win32::TerminateApp returned %s", msg );
      return true;
      }
   return !IsThreadActive();
   }

STATIC_VAR Win32_pty *s_pCompilePty;

bool CompileJobQueueWaitExeDoneTimedout( int timeoutMS ) {
   return s_pCompilePty->WaitExeDoneTimedout( timeoutMS );
   }

int CompilePty_CmdsAsyncExec( const StringList &sl, bool fAppend ) {
   return s_pCompilePty->EnqueueJobsAndRun( sl );
   }

int CompilePty_KillAllJobs() {
   return s_pCompilePty->KillAllJobsInBkgndProcessQueue();
   }

bool IsCompileJobQueueThreadActive() {
   return s_pCompilePty->IsThreadActive();
   }

//#################################################################################################################################
//#################################################################################################################################
//#################################################################################################################################

class InternalShellJobExecutor {
   NO_COPYCTOR(InternalShellJobExecutor);
   NO_ASGN_OPR(InternalShellJobExecutor);
   PFBUF                       d_pfLogBuf;
   std::unique_ptr<StringList> d_pSL;
   const size_t                d_numJobsRequested;
   Win32::PROCESS_INFORMATION  d_processInfo;
   Win32::HANDLE               d_hThread;
   Win32::DWORD                d_hProcessExitCode;
   std::mutex                  d_jobQueueMtx;
   Win32::ManualClrEvent       d_AllJobsDone;
public:
   InternalShellJobExecutor( PFBUF pfb, StringList *sl, bool fViewsActivelyTailOutput );
   ~InternalShellJobExecutor();
   void GetJobStatus( size_t *pNumRequested, size_t *pNumNotStarted ) const {
      *pNumRequested  = d_numJobsRequested;
      *pNumNotStarted = d_pSL->length();
      }
private:
   void ThreadFxnRunAllJobs();
   int  KillAllJobsInBkgndProcessQueue();
   int  DeleteAllEnqueuedJobs_locks();
   };

InternalShellJobExecutor::InternalShellJobExecutor( PFBUF pfb, StringList *sl, bool fViewsActivelyTailOutput )
   : d_pfLogBuf         ( pfb )
   , d_pSL              ( sl )
   , d_numJobsRequested ( sl->length() )
   , d_hThread          ( nullptr )
   , d_hProcessExitCode ( 0 )
   {
   std::thread rajt( InternalShellJobExecutor::ThreadFxnRunAllJobs, this ); rajt.detach();
   }

InternalShellJobExecutor::~InternalShellJobExecutor() {
   KillAllJobsInBkgndProcessQueue();
   }

void InternalShellJobExecutor::ThreadFxnRunAllJobs() { // RUNS ON ONE OR MORE TRANSIENT THREADS!!!
   if( g_fMsgflush ) {
      WhileHoldingGlobalVariableLock gvlock;     // wait until we own the I/O
      d_pfLogBuf->MakeEmpty();
      }
   auto failedJobsIgnored(0);
   auto unstartedJobCnt(0);
   PerfCounter pc;
   Xbuf x1, x2;
   Win32::AutoSignalEvent amce( d_AllJobsDone );
   while( true ) { //**************** outerthreadloop ****************
      StringListEl *pEl;
      {
      std::scoped_lock<std::mutex> LockTheJobQueue( d_jobQueueMtx ); // ##################### LockTheJobQueue ######################
      d_processInfo.dwProcessId = INVALID_dwProcessId;
      DispNeedsRedrawStatLn(); // ???
      if( !(pEl=d_pSL->remove_first()) ) { // ONLY EXIT FROM THREAD IS HERE!!!
         linebuf buf;
         PutLastLogLine( d_pfLogBuf, showTermReason( BSOB(buf), d_hProcessExitCode, unstartedJobCnt, failedJobsIgnored, pc.Capture() ) );
         d_hProcessExitCode = 0;
         d_hThread = nullptr;
         return; // ##################### LockTheJobQueue ######################
         }
      } // ##################### LockTheJobQueue ######################
      auto cmdFlags(0);
      auto pS( pEl->string + xlat_cmdline_flag_chars( pEl->string, &cmdFlags ) );
      if( *pS ) {
         prep_cmdline( pS, cmdFlags, __func__ );
         const auto cp_rc( CreateProcess_piped( &d_processInfo, &d_hProcessExitCode, d_pfLogBuf, pS, cmdFlags, &x1, &x2 ) );
         if( CP_PIPED_RC_OK == cp_rc ) {
            if( d_hProcessExitCode ) {
               if( cmdFlags & IGNORE_ERROR ) {  ++failedJobsIgnored;  }
               else                          {  unstartedJobCnt = DeleteAllEnqueuedJobs_locks();  }
               }
            }
         }
      FreeStringListEl( pEl );
      } //**************** outerthreadloop ****************
   ConOut::Bell();
   }

PFBUF StartInternalShellJob( StringList *sl, bool fAppend ) {
   STATIC_VAR size_t s_nxt_shelljob_output_FBUF_num;
   if( !fAppend ) {
NEXT_OUTBUF:
      ++s_nxt_shelljob_output_FBUF_num;
      }
   char fnm[30];
   auto pFB( FBOP::FindOrAddFBuf( safeSprintf( BSOB(fnm), "<shell_output-%03" PR_SIZET ">", s_nxt_shelljob_output_FBUF_num ) ) );
   if( pFB ) {
      if( pFB->d_pInternalShellJobExecutor ) goto NEXT_OUTBUF; // don't want to append to an FBUF currently in use by a d_pInternalShellJobExecutor
      pFB->PutFocusOn();
      pFB->d_pInternalShellJobExecutor = new InternalShellJobExecutor( pFB, sl, true );
      }
   return pFB;
   }

int InternalShellJobExecutor::DeleteAllEnqueuedJobs_locks() {
   std::scoped_lock<std::mutex> LockTheJobQueue( d_jobQueueMtx );
   const auto rmCnt( d_pSL->length() );
   d_pSL->clear();
   return rmCnt;
   }

int InternalShellJobExecutor::KillAllJobsInBkgndProcessQueue() {
   DeleteAllEnqueuedJobs_locks();
   if(   INVALID_dwProcessId != d_processInfo.dwProcessId
      && ConIO::Confirm( FmtStr<55>( "Kill background %s process (PID=%ld)?", d_pfLogBuf->Name(), d_processInfo.dwProcessId ) )
      && INVALID_dwProcessId != d_processInfo.dwProcessId
      ) {
      PCChar msg;
      switch( Win32::TerminateApp( d_processInfo.dwProcessId, 2000 ) ) {
         break;default                      : msg = "WTF!?"                   ;
         break;case TA_FAILED               : msg = "TA_FAILED"               ;
         break;case TA_SUCCESS_CTRL_BREAK   : msg = "TA_SUCCESS_CTRL_BREAK"   ;
         break;case TA_SUCCESS_WM_CLOSE     : msg = "TA_SUCCESS_WM_CLOSE"     ;
         break;case TA_SUCCESS_TERM_PROCESS : msg = "TA_SUCCESS_TERM_PROCESS" ;
         }
      d_processInfo.dwProcessId = INVALID_dwProcessId;
      Msg( "Win32::TerminateApp returned %s", msg );
      return 1;
      }
   return 1; // !IsThreadActive();
   }

STATIC_FXN void IdleThread() {
   0 && DBG( "*** %s STARTING***", __func__ );
   while( true ) {
      SleepMs( 100 );  // was 50 @ 20130101
      WhileHoldingGlobalVariableLock gvlock;
       {
       IdleIntegrityCheck();
       }
      if( EditorLoadCountChanged() ) {
         DispNeedsRedrawStatLn();
         }
      UpdateConsoleTitle();
      if( g_fSystemShutdownOrLogoffRequested && EditorFilesystemNoneDirty() ) { // silently exit if no harm would be done
         EditorExit( 0, true );
         }
      // DispDoPendingRefreshes() is here PRIMARILY for aux-thread related screen
      // updates (when the screen changes NOT as a result of user edit activity,
      // but from piped in output from a child process): when this is commented
      // out, NORMAL edit-driven screen-redraw seems unaffected.
      //
      // UPDT: re-confirmed by accident!  I accidentally turned off the main
      //       thread's call (CmdFromKbdForExec()'s) to
      //       GiveUpGlobalVariableLock(), and the only way I
      //       noticed is because child compile processes which wrote to
      //       <compile> ("alt+m,u,t" is simplest example) had no screen
      //       updates.  Otherwise, there was ZERO noticable difference.
      //
      DispDoPendingRefreshes();
      LuaIdleGC();
      }
   }

void DetachIdleThread() {
   // IdleThread is quasi-related to JobQueues: it updates <compile> window when
   // writer is a spawned process...
   //
   std::thread idle( IdleThread ); idle.detach();
   s_pCompilePty = new Win32_pty( kszCompile );
   }

bool ARG::compile() {
   switch( d_argType ) {
      default:      return BadArg();
      case NOARG:   {
                    const auto fCompiling( IsCompileJobQueueThreadActive() );
                    Msg( "%scompile in progress", fCompiling ? "" : "no " );
                    return fCompiling;
                    }
      case NULLARG: CompilePty_KillAllJobs();
                    return false;
      case TEXTARG: {
                    g_CurFBuf()->SyncWriteIfDirty_Wrote();
                    DispDoPendingRefreshesIfNotInMacro();
                    const auto cmdCnt( CompilePty_CmdsAsyncExec( StringList( d_textarg.pText ), true ) );
                    Msg( "Queued %d commands", cmdCnt );
                    return cmdCnt > 0;
                    }
      }
   }

//###########################################################################################################
//###########################################################################################################
//###########################################################################################################

//
//  Function: StartConsoleProcess (ex-do_editfile from WINDIFF.C in WINDIFF sample from VC++ 4.1)
//
//  Purpose: start (as a separate process) the program spec'd
//
//  12-Aug-1997 klg works!
//

STATIC_FXN int StartChildProcess( PCChar pFullCommandLine, int extra_dwCreationFlags ) {
   Win32::STARTUPINFO          si = { sizeof si };
   Win32::PROCESS_INFORMATION  pi;
   // Launches the process and (optionally) waits for it to complete
   if( Win32::CreateProcessA(
         nullptr,                       // lpApplicationName        : PChar;
         PChar(pFullCommandLine), // lpCommandLine            : PChar;
         nullptr,                       // lpProcessAttributes      : PSecurityAttributes;
         nullptr,                       // lpThreadAttributes       : PSecurityAttributes;
         0,                       // bInheritHandles          : BOOL;
         ( NORMAL_PRIORITY_CLASS  // dwCreationFlags          : DWORD;
         | CREATE_DEFAULT_ERROR_MODE
         | extra_dwCreationFlags
         ),
         nullptr,                       // lpEnvironment            : Pointer;
         nullptr,                       // lpCurrentDirectory       : PChar;
         &si,                     // const lpStartupInfo      : TStartupInfo;
         &pi                      // var lpProcessInformation : TProcessInformation
        )
     ) {
      DBG( "'%s' -> CreateProcess SUCCEEDED: P=%ld, T=%ld", pFullCommandLine, pi.dwProcessId, pi.dwThreadId );
      CLOSEHANDLE( pi.hProcess );
      CLOSEHANDLE( pi.hThread  );
      return 0;
      }
   else {
      linebuf oseb;
      DBG( "'%s' -> CreateProcess FAILED: %s", pFullCommandLine, OsErrStr( BSOB(oseb) ) );
      return 1;
      }
   }

int  StartGuiProcess         ( PCChar pFullCmdLn )  { return       StartChildProcess( pFullCmdLn, DETACHED_PROCESS   ); }
int  StartConProcess         ( PCChar pFullCmdLn )  { return       StartChildProcess( pFullCmdLn, CREATE_NEW_CONSOLE ); }
void StartShellExecuteProcess( PCChar pFullCmdLn, PCChar pExeFile )  {
   // re 2nd param to ShellExecute: I'm using nullptr (NOT "open") per
   //    http://blogs.msdn.com/oldnewthing/archive/2007/04/30/2332224.aspx
   //    summary: nullptr gives the default action which is not always "open"
   // re 3rd, 4th params to ShellExecute: I want, under programmatically detected circumstances, to
   //    FORCE a URL to be opened in MSIE even though MSIE is not the dflt browser.
   //    http://stackoverflow.com/questions/8180661/force-opening-a-webpage-with-internet-explorer
   //    tells how; there is some 100% nonintuitive ShellExecute param swapping required...
   if( pExeFile && 0 != *pExeFile ) {
      Win32::ShellExecute( nullptr, nullptr, pExeFile, pFullCmdLn, nullptr, 1 );
      }
   else {
      Win32::ShellExecute( nullptr, nullptr, pFullCmdLn, nullptr, nullptr, 1 );
      }
   }

bool RunChildSpawnOrSystem( PCChar pCmdStr ) {
   const auto cmdStr( StrPastAnyBlanks( pCmdStr ) );
   if( *cmdStr == 0 ) {
      return true;
      }
   0 && DBG( "%s '%s'"   , "system", pCmdStr );
   const ConsoleInputModeRestorer cim;
   const auto rtnCode( system( cmdStr ) );
   ConsoleInputAcquire();
   cim.Set();
   DBG( "%s rtns %d", "system", rtnCode );
   if( rtnCode == -1 ) {
      ErrorDialogBeepf( "system(%s) failed: %s", cmdStr, strerror( errno ) );
      return true;
      }
   return false;
   }

#ifdef fn_shell

// 20091216 kgoodwin this seems superfluous so have disabled it

bool ARG::shell() {
   if( !d_fMeta ) {
      fExecute( "saveall" );
      }
   MsgClr();
   CursorLocnOutsideView_Set( DialogLine(), 0 );
   bool rv( false );
   switch( d_argType ) {
      case NOARG  : rv = RunChildSpawnOrSystem( "" );               break;
      case TEXTARG: rv = RunChildSpawnOrSystem( d_textarg.pText );  break;
      case LINEARG: ATTR_FALLTHRU;
      case BOXARG : for( ArgLineWalker aw( this ) ; !aw.Beyond() ; aw.NextLine() ) {
                       if( aw.GetLine() ) {
                          if( !RunChildSpawnOrSystem( aw.c_str() ) ) {
                             g_CurView()->MoveCursor( aw.Line(), aw.Col() );
                             return false;
                             }
                          }
                       }
                    return true;
      }
   g_CurFBuf()->SyncNoWrite();
   return rv;
   }

#endif

bool popen_rd_ok( std::string &dest, PCChar szcmdline ) {
   dest.clear();
   auto fp( _popen( szcmdline, "r" ) );
   if( fp != NULL ) {
      char buf[8192];
      while( fgets( buf, sizeof buf, fp ) != NULL ) {
         dest.append( buf );
         }
      const auto status( _pclose(fp) );
      if( status == -1 ) { /* Error reported by pclose() */
         }
      else {
         if( 0 == status ) {
            return true;
            }
         }
      }
   return false;
   }

bool cygpath_xlat( std::string &stbuf ) {
   std::string cmdline { "cygpath '" + stbuf + "'" };
   const auto rv( popen_rd_ok( stbuf, cmdline.c_str() ) );
   const auto ix( stbuf.find_first_of( '\n' ) );
   if( std::string::npos != ix ) {
      stbuf.resize( ix );
      }
   return rv;
   }
