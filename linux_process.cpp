//
//
// Copyright 2015 by Kevin L. Goodwin; All rights reserved
//
//

#include "ed_main.h"

// #include <functional>
#include <thread>
#include <mutex>

#if 0

//##############################################################################

//--------------------------------------------------------------------------------------------

#define CLOSEHANDLE(hvar)  { const auto chrv( Win32::CloseHandle( hvar ) );  Assert( chrv ); hvar = nullptr; }

//#############################################################################################################################
//############################################  Win32 Child Process EXEC/MGMT  ################################################
//#############################################################################################################################

enum {
   CH_NO_ECHO_CMDLN = '@', NO_ECHO_CMDLN = BIT(0),
   CH_IGNORE_ERROR  = '-', IGNORE_ERROR  = BIT(1),
   CH_NOSHELL       = '^', NOSHELL       = BIT(2),
   };

STATIC_FXN PCChar analyze_cmdline_( PCChar pCmdln, int *flags ) {
   0 && DBG( "%s+ '%s'", __func__,  pCmdln );
   do {
      switch( *pCmdln ) {
         case ' ': case HTAB:                            break;
         case CH_NO_ECHO_CMDLN: *flags |= NO_ECHO_CMDLN; break;
         case CH_IGNORE_ERROR : *flags |= IGNORE_ERROR ; break;
         case CH_NOSHELL      : *flags |= NOSHELL      ; break;
         default:                           goto  PAST_OPTS;
         }
      } while( *(++pCmdln) );
PAST_OPTS: ;

   0 && DBG( "%s- '%s'", __func__,  pCmdln );
   return *pCmdln ? pCmdln : nullptr;
   }

TF_Ptr STIL Ptr analyze_cmdline( Ptr pCmdln, int *flags ) { return const_cast<Ptr>(analyze_cmdline_( pCmdln, flags )); }

STATIC_FXN void prep_cmdline_( PChar pc ) {
   // CMD shell cannot handle '/' dirsep in argv[0], so xlat to '\'
   const auto chDelim( Path::DelimChar( pc ) );
   const auto pEoArgv0( chDelim ? strchr(pc+1,chDelim) : StrToNextBlankOrEos( pc ) );
   for( ; pc < pEoArgv0 ; ++pc ) {
      if( Path::chDirSepPosix == *pc ) *pc = Path::chDirSepMS;
      }
   }

STATIC_FXN void prep_cmdline( PChar pc, int cmdFlags, PCChar func__ ) {
   prep_cmdline_( pc );
   0 && DBG( "%s: %s%s%s: %s"
      , func__
      , (cmdFlags & NO_ECHO_CMDLN) ? "@" : ""
      , (cmdFlags & IGNORE_ERROR ) ? "-" : ""
      , (cmdFlags & NOSHELL      ) ? "^" : ""
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

   enum { EMPTY = -1 };

public:

   TPipeReader( Win32::HANDLE hReadPipe ) : d_RdPipeHandle( hReadPipe ), d_bytesInRawBuffer( 0 ), d_pRawBuffer( d_rawBuffer ) {}
   ~TPipeReader() { Win32::CloseHandle( d_RdPipeHandle ); }

   int GetFilteredLine( PXbuf xb );
   };

int TPipeReader::RdChar() { // see http://support.microsoft.com/kb/q190351/
   if( d_bytesInRawBuffer == 0 ) {
      if( 0 == Win32::ReadFile(
                    d_RdPipeHandle
                  , BSOB(d_rawBuffer)
                  , &d_bytesInRawBuffer
                  , nullptr
                  )
        ) {
         // all write handles to the pipe are closed
         //
         d_bytesInRawBuffer = 0;
         return EMPTY;
         }

      if( d_bytesInRawBuffer == 0 )
         return EMPTY;

      d_pRawBuffer = d_rawBuffer;
      }

   --d_bytesInRawBuffer;
   const char rv( *d_pRawBuffer++ );
   return rv;
   }


int TPipeReader::GetFilteredLine( PXbuf xb ) {
   xb->clear();
   auto lastCh(0);
   while( 1 ) {
      lastCh = RdChar();
      switch( lastCh ) {
         case 0x0D:  break;            // drop CR
         case 0x0A:  goto END_OF_LINE; // LF signifies EOL
         case EMPTY: goto END_OF_LINE; // no more data available (for now)

         case HTAB:  { // expand to spaces            01234567
                     STATIC_CONST char tabspaces[] = "        ";
                     xb->cat( tabspaces+( xb->length() & (MAX_TAB_WIDTH-1)) );
                     }break;

         default:    xb->push_back( lastCh );
                     break;
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
      if( failedJobsIgnored )    _snprintf( dest, sizeofDest, "--- processing successful, %d job-failure%s ignored %s ---", failedJobsIgnored, Add_s( failedJobsIgnored ), ets.k_str() );
      else                       _snprintf( dest, sizeofDest, "--- processing successful %s ---", ets.k_str() );
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
         if( unstartedJobCnt )   _snprintf( dest, sizeofDest, "%s%lX (%s), %d job%s unstarted ---", hdr, hProcessExitCode, erbuf, unstartedJobCnt, Add_s( unstartedJobCnt ) );
         else                    _snprintf( dest, sizeofDest, "%s%lX (%s) ---"                    , hdr, hProcessExitCode, erbuf );
         }
      else
     #endif
         {
         if( unstartedJobCnt )   _snprintf( dest, sizeofDest, "%s%lX, %d job%s unstarted ---"     , hdr, hProcessExitCode       , unstartedJobCnt, Add_s( unstartedJobCnt ) );
         else                    _snprintf( dest, sizeofDest, "%s%lX ---"                         , hdr, hProcessExitCode       );
         }
      }
   return dest;
   }


#define  USE_ConsoleInputModeRestorer  0

STATIC_FXN void PutLastLogLine( PFBUF d_pfLogBuf, PCChar msg ) {
   WhileHoldingGlobalVariableLock gvlock; // wait until we own the output resource
   d_pfLogBuf->PutLastLine( msg );
   if( g_fViewsActivelyTailOutput ) {
      MoveCursorToEofAllWindows( d_pfLogBuf, true );
      }
   }

STATIC_FXN PCChar getComspec() {
   CPCChar comspec( getenv( "COMSPEC" ) );
   return  comspec ? comspec : "CMD.EXE";
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
   , PChar pS
   , int cmdFlags
   , PXbuf CommandLine
   , PXbuf xb
   ) {
   const auto shell( !(cmdFlags & NOSHELL) );
   const auto comspec( getComspec() );
   STATIC_CONST char comspecTransient[] = " /c ";
   CommandLine->FmtStr( "-%s%s%s", shell?comspec:"", shell?comspecTransient:"", pS ); // leading '-' is (at most) for PutLastLogLine _only_
   0 && DBG( "%s: CommandLine='%s'", __func__, CommandLine->c_str() );
   const auto pXeq(      CommandLine->wbuf() + 1 );  // skip the '-' always (stupid Win32::CreateProcessA takes PChar cmdline param)
   const auto pXeqConst( CommandLine->c_str() + 1 );  // skip the '-' always (for internal use)
   if( !(cmdFlags & NO_ECHO_CMDLN) ) {
      PutLastLogLine( pfLogBuf, CommandLine->c_str() + ((cmdFlags & IGNORE_ERROR) ? 0 : 1 ) );
      }

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
   STATIC_VAR Win32_pty             *s_Win32_pty_ListHead;

   NO_COPYCTOR(Win32_pty);
   NO_ASGN_OPR(Win32_pty);

   Win32::ManualClrEvent         d_AllJobsDone;

   DLinkHead<Win32pty_job_Q_el>  d_jobQHead;
   int                           d_numJobRequestsPending;
   Mutex                         d_jobQueueMtx;

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

   STATIC_FXN Win32::DWORD K_STDCALL ChildProcessCtrlThread( Win32::LPVOID pThreadParam );
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
   for( auto pQ(s_Win32_pty_ListHead) ; pQ ; pQ=pQ->d_pNext )
      pQ->KillAllJobsInBkgndProcessQueue();
   }

int Win32_pty::ActiveQueues() {
   auto sum(0);
   for( auto pQ(s_Win32_pty_ListHead) ; pQ ; pQ=pQ->d_pNext )
      if( pQ->IsThreadActive() )
         ++sum;

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
   auto PutLastLine_( [=]( PCChar pline ) { PutLastLogLine( d_pfLogBuf, pline ); } );

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
      AutoMutex LockTheJobQueue( d_jobQueueMtx ); // ##################### LockTheJobQueue ######################
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

Win32::DWORD Win32_pty::ChildProcessCtrlThread( Win32::LPVOID pThreadParam ) {
   0 && DBG( cpct_start_fmts, "W32pty" );
                                          static_cast<Win32_pty *>( pThreadParam )->ThreadFxnRunAllJobs();
   0 && DBG( cpct_exit_fmts , "W32pty" );
   return 0; // equivalent to ExitThread( 0 );
   }

bool Win32_pty::EnqueueJobPrimeThread_nolock() {
   if( !IsThreadActive() ) {
      Assert( d_pfLogBuf );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
      if( 0 == (d_hThread=Win32::CreateThread( nullptr, 4*1024, Win32_pty::ChildProcessCtrlThread, this, 0L, nullptr )) ) {
#pragma GCC diagnostic pop
         DBG( "%s Win32::CreateThread FAILED!", __func__  );
         return false;
         }
      return true;
      }
   return false;
   }


int Win32_pty::EnqueueJobsAndRun( const StringList &sl ) {
   auto numAdded(0);
   AutoMutex LockTheJobQueue( d_jobQueueMtx );
   const auto fNeedToPrime( 0 == d_numJobRequestsPending );
   DLINKC_FIRST_TO_LASTA( sl.d_head, dlink, pCur ) {
      DBG( "%s '%s'", __func__,  pCur->string );
      auto flags(0);
      const auto pC( analyze_cmdline_( pCur->string, &flags ) );
      if( pC ) {
         const auto pCD( Strdup( pC ) );
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
   AutoMutex LockTheJobQueue( d_jobQueueMtx );
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

   if( IsThreadActive() && ConIO::Confirm( "Kill background %s process (PID=%ld)?", d_pfLogBuf->Name(), d_processInfo.dwProcessId ) ) {
      PCChar msg;
      switch( Win32::TerminateApp( d_processInfo.dwProcessId, 2000 ) ) {
         default                      : msg = "WTF!?"                    ;  break;
         case TA_FAILED               : msg = "TA_FAILED"                ;  break;
         case TA_SUCCESS_CTRL_BREAK   : msg = "TA_SUCCESS_CTRL_BREAK"    ;  break;
         case TA_SUCCESS_WM_CLOSE     : msg = "TA_SUCCESS_WM_CLOSE"      ;  break;
         case TA_SUCCESS_TERM_PROCESS : msg = "TA_SUCCESS_TERM_PROCESS"  ;  break;
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
   StringList                 *d_pSL;
   const size_t                d_numJobsRequested;

   Win32::PROCESS_INFORMATION  d_processInfo;
   Win32::HANDLE               d_hThread;
   Win32::DWORD                d_hProcessExitCode;

   Mutex                       d_jobQueueMtx;

   Win32::ManualClrEvent       d_AllJobsDone;

   STATIC_FXN Win32::DWORD K_STDCALL ChildProcessCtrlThread( Win32::LPVOID pThreadParam );

public:

   InternalShellJobExecutor( PFBUF pfb, StringList *sl, bool fViewsActivelyTailOutput );
   ~InternalShellJobExecutor();

   void GetJobStatus( size_t *pNumRequested, size_t *pNumNotStarted ) const
      {
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
   if( 0 == (d_hThread=Win32::CreateThread( nullptr, 4*1024, InternalShellJobExecutor::ChildProcessCtrlThread, this, 0L, nullptr )) ) {
#pragma GCC diagnostic pop
      DBG( "%s Win32::CreateThread FAILED!", __func__  );
      }
   }

InternalShellJobExecutor::~InternalShellJobExecutor() {
   KillAllJobsInBkgndProcessQueue();
   Delete0( d_pSL );
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
      AutoMutex LockTheJobQueue( d_jobQueueMtx ); // ##################### LockTheJobQueue ######################
      d_processInfo.dwProcessId = INVALID_dwProcessId;
      DispNeedsRedrawStatLn(); // ???
      if( d_pSL->empty() ) { // ONLY EXIT FROM THREAD IS HERE!!!
         linebuf buf;
         PutLastLogLine( d_pfLogBuf, showTermReason( BSOB(buf), d_hProcessExitCode, unstartedJobCnt, failedJobsIgnored, pc.Capture() ) );
         d_hProcessExitCode = 0;
         d_hThread = nullptr;
         return; // ##################### LockTheJobQueue ######################
         }
      DLINK_REMOVE_FIRST( d_pSL->Head(), pEl, dlink );
      } // ##################### LockTheJobQueue ######################

      auto cmdFlags(0);
      PChar pS( analyze_cmdline( pEl->string, &cmdFlags ) );
      if( *pS ) { prep_cmdline( pS, cmdFlags, __func__ ); }

      const auto cp_rc( CreateProcess_piped( &d_processInfo, &d_hProcessExitCode, d_pfLogBuf, pS, cmdFlags, &x1, &x2 ) );
      if( CP_PIPED_RC_OK == cp_rc ) {
         if( d_hProcessExitCode ) {
            if( cmdFlags & IGNORE_ERROR ) {  ++failedJobsIgnored;  }
            else                          {  unstartedJobCnt = DeleteAllEnqueuedJobs_locks();  }
            }
         }
      FreeStringListEl( pEl );
      } //**************** outerthreadloop ****************
   ConOut::Bell();
   }


Win32::DWORD InternalShellJobExecutor::ChildProcessCtrlThread( Win32::LPVOID pThreadParam ) {
   0 && DBG( cpct_start_fmts, "ISJE" );
                                        static_cast<InternalShellJobExecutor *>( pThreadParam )->ThreadFxnRunAllJobs();
   0 && DBG( cpct_exit_fmts , "ISJE" );
   return 0; // equivalent to ExitThread( 0 );
   }


PFBUF StartInternalShellJob( StringList *sl, bool fAppend ) {
   STATIC_VAR size_t s_nxt_shelljob_output_FBUF_num;
   if( !fAppend ) {
NEXT_OUTBUF:
      ++s_nxt_shelljob_output_FBUF_num;
      }

   char fnm[30];
   auto pFB( FBOP::FindOrAddFBuf( safeSprintf( BSOB(fnm), "<shell_output-%03Iu>", s_nxt_shelljob_output_FBUF_num ) ) );
   if( pFB ) {
      if( pFB->d_pInternalShellJobExecutor ) goto NEXT_OUTBUF; // don't want to append to an FBUF currently in use by a d_pInternalShellJobExecutor
      pFB->PutFocusOn();
      pFB->d_pInternalShellJobExecutor = new InternalShellJobExecutor( pFB, sl, true );
      }
   return pFB;
   }


int InternalShellJobExecutor::DeleteAllEnqueuedJobs_locks() {
   AutoMutex LockTheJobQueue( d_jobQueueMtx );
   auto &d_jobQHead = d_pSL->d_head;
   const auto rmCnt( d_jobQHead.length() );
   while( auto pEl = d_jobQHead.front() ) {
      DLINK_REMOVE_FIRST( d_jobQHead, pEl, dlink );
      Delete0( pEl );
      }
   return rmCnt;
   }


int InternalShellJobExecutor::KillAllJobsInBkgndProcessQueue() {
   DeleteAllEnqueuedJobs_locks();

   if(   INVALID_dwProcessId != d_processInfo.dwProcessId
      && ConIO::Confirm( "Kill background %s process (PID=%ld)?", d_pfLogBuf->Name(), d_processInfo.dwProcessId )
      && INVALID_dwProcessId != d_processInfo.dwProcessId
      ) {
      PCChar msg;
      switch( Win32::TerminateApp( d_processInfo.dwProcessId, 2000 ) ) {
         default                      : msg = "WTF!?"                    ;  break;
         case TA_FAILED               : msg = "TA_FAILED"                ;  break;
         case TA_SUCCESS_CTRL_BREAK   : msg = "TA_SUCCESS_CTRL_BREAK"    ;  break;
         case TA_SUCCESS_WM_CLOSE     : msg = "TA_SUCCESS_WM_CLOSE"      ;  break;
         case TA_SUCCESS_TERM_PROCESS : msg = "TA_SUCCESS_TERM_PROCESS"  ;  break;
         }

      d_processInfo.dwProcessId = INVALID_dwProcessId;
      Msg( "Win32::TerminateApp returned %s", msg );
      return 1;
      }

   return 1; // !IsThreadActive();
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
   if( *cmdStr == 0 )
      return true;

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
   if( !d_fMeta )
      fExecute( "saveall" );

   MsgClr();

   CursorLocnOutsideView_Set( DialogLine(), 0 );

   bool rv( false );
   switch( d_argType ) {
      case NOARG  : rv = RunChildSpawnOrSystem( "" );               break;
      case TEXTARG: rv = RunChildSpawnOrSystem( d_textarg.pText );  break;

      case LINEARG: //lint -fallthrough
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

#endif


STATIC_FXN bool EditorFilesystemNoneDirty() {
   auto dirtyFBufs(0);
   auto openFBufs (0);
  #if FBUF_TREE
   RbNode *pNd;
   rb_traverse( pNd, g_FBufIdx )
  #else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
  #endif
      {
  #if FBUF_TREE
      PCFBUF pFBuf = IdxNodeToFBUF( pNd );
  #endif
      if( pFBuf->HasLines() && pFBuf->FnmIsDiskWritable() ) {
         ++openFBufs;
         if( pFBuf->IsDirty() )
            ++dirtyFBufs;
         }
      }
   return dirtyFBufs == 0;
   }

STATIC_VAR bool g_fSystemShutdownOrLogoffRequested = false;

STATIC_FXN int IdleThread() {
   1 && DBG( "*** %s STARTING***", __func__ );
   while( true ) {
      std::this_thread::sleep_for( std::chrono::milliseconds(100) ); // was 50 @ 20130101

      WhileHoldingGlobalVariableLock gvlock;

      if( g_fSystemShutdownOrLogoffRequested && EditorFilesystemNoneDirty() ) // silently exit if no harm would be done
         EditorExit( 0, true );

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
   return 0; // suppress warning
   }

void InitJobQueues() {
   // IdleThread is quasi-related to JobQueues: it updates <compile> window when
   // writer is a spawned process...
   //
   std::thread idle( IdleThread ); idle.detach();
   // s_pCompilePty = new Win32_pty( szCompile );
   }

const auto mainThreadId( std::this_thread::get_id() );

STIL void ASSERT_MAIN_THREAD()     { Assert(mainThreadId == std::this_thread::get_id()); }
STIL void ASSERT_NOT_MAIN_THREAD() { Assert(mainThreadId != std::this_thread::get_id()); }

STATIC_VAR std::mutex s_GlobalVariableLock;
STIL void GiveUpGlobalVariableLock()  { /* DBG( "rls baton" ); */ s_GlobalVariableLock.unlock(); }
STIL void WaitForGlobalVariableLock() {                           s_GlobalVariableLock.lock(); /*  DBG( "got baton" ); */ }

WhileHoldingGlobalVariableLock::WhileHoldingGlobalVariableLock()  { WaitForGlobalVariableLock(); }
WhileHoldingGlobalVariableLock::~WhileHoldingGlobalVariableLock() { GiveUpGlobalVariableLock();  }

void MainThreadGiveUpGlobalVariableLock()  { ASSERT_MAIN_THREAD();  /* MainThreadPerfCounter::PauseAll() ; */  GiveUpGlobalVariableLock() ; }
void MainThreadWaitForGlobalVariableLock() { ASSERT_MAIN_THREAD();  WaitForGlobalVariableLock()       ;  /* MainThreadPerfCounter::ResumeAll()   ; */ }
