//
//
// Copyright 2015 by Kevin L. Goodwin; All rights reserved
//
//

#include "ed_main.h"

// #include <functional>
#include <thread>
#include <mutex>

#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

enum { INVALID_dwProcessId = 0, INVALID_fd = -1, PIPE_RD=0, PIPE_WR=1 };

class piped_forker {
   int     fd  =  INVALID_fd;
   int     pid =  INVALID_dwProcessId;
   int     exit_status = -1;
public:
   piped_forker() {}
   bool    ForkChildOk( const char *command );
   int     ReapChild();
   int     Status() const { return exit_status; }
   ssize_t Read( void *dest, ssize_t sizeofDest );
   };

int piped_forker::ReapChild() {
   if( fd != INVALID_fd ) {
      close( fd );
      fd = INVALID_fd;
      }
   if( pid != INVALID_dwProcessId ) {
      int status;
      waitpid( pid, &status, 0 );
      alarm(0);
      exit_status = WEXITSTATUS( status );
      }
   return exit_status;
   }

ssize_t piped_forker::Read( void *dest, ssize_t sizeofDest ) {
   if( fd == INVALID_fd ) {
      return -1;
      }
   const auto rv( read( fd, dest, sizeofDest ) );  0 && DBG( "%s-[%d] %d", __func__, fd, rv );
   return rv;
   }

bool piped_forker::ForkChildOk( const char *command ) {  DBG( "%s+(from %d) '%s'", __func__, getpid(), command );
   pid = INVALID_dwProcessId;
   int pipefds[2];
   if( pipe( pipefds ) == -1 ) {
      perror( "pipe" );
      return false;
      }

   switch( (pid=fork()) ) {
      case -1: perror( "fork" );  /* fail */
               return false;

      case 0: {signal( SIGPIPE, SIG_DFL );  /* child */  // FIXME: close other opened descriptor
               close( pipefds[PIPE_RD] );
               close( 0 );
               const int nevdullfh( open("/dev/null", O_RDONLY) ); Assert( nevdullfh == 0 );
               dup2(  pipefds[PIPE_WR], 1 );
               dup2(  pipefds[PIPE_WR], 2 );
               close( pipefds[PIPE_WR] );
               exit( system( command ) );
              }return false; // keep compiler happy

      default: close(  pipefds[PIPE_WR] );  /* parent */
               // fcntl(  pipefds[PIPE_RD], F_SETFL, O_NONBLOCK );
               fd = pipefds[PIPE_RD];
               DBG( "%s-(from %d) fork parent; child=%d, fd=%d; '%s'", __func__, getpid(), pid, fd, command );
               return true;
      }
   }


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

STATIC_FXN PChar showTermReason( PChar dest, size_t sizeofDest, const int hProcessExitCode, const int unstartedJobCnt, const int failedJobsIgnored, double et ) {
   enum { USER_BREAK = 99999999 }; // bugbug this MADE UP
   if( 0 == hProcessExitCode ) {
      FmtStr<30> ets( "in %.3f S", et );
      if( failedJobsIgnored )    snprintf( dest, sizeofDest, "--- processing successful, %d job-failure%s ignored %s ---", failedJobsIgnored, Add_s( failedJobsIgnored ), ets.k_str() );
      else                       snprintf( dest, sizeofDest, "--- processing successful %s ---", ets.k_str() );
      }
   else if( USER_BREAK == hProcessExitCode ) {
      snprintf( dest, sizeofDest, "--- process TERMINATED with prejudice" );
      }
   else {
      STATIC_CONST char hdr[] = "--- process FAILED, last exit code=0x";
      if( unstartedJobCnt )   snprintf( dest, sizeofDest, "%s%X, %d job%s unstarted ---"     , hdr, hProcessExitCode       , unstartedJobCnt, Add_s( unstartedJobCnt ) );
      else                    snprintf( dest, sizeofDest, "%s%X ---"                         , hdr, hProcessExitCode       );
      }
   return dest;
   }

STATIC_FXN void PutLastLogLine( PFBUF d_pfLogBuf, stref s0, stref s1 ) {
   WhileHoldingGlobalVariableLock gvlock; // wait until we own the output resource
   STATIC_VAR std::vector<stref> s_refs; STATIC_VAR std::string tmp0, tmp1;
   s_refs.clear(); s_refs.emplace_back( s0 ); s_refs.emplace_back( s1 );
   d_pfLogBuf->PutLastLine( s_refs, tmp0, tmp1 );
   if( g_fViewsActivelyTailOutput ) {
      MoveCursorToEofAllWindows( d_pfLogBuf, true );
      }
   }

STATIC_FXN void PutLastLogLine( PFBUF d_pfLogBuf, stref s0 ) {
   WhileHoldingGlobalVariableLock gvlock; // wait until we own the output resource
   STATIC_VAR std::string tmp0;
   d_pfLogBuf->PutLastLine( s0, tmp0 );
   if( g_fViewsActivelyTailOutput ) {
      MoveCursorToEofAllWindows( d_pfLogBuf, true );
      }
   }

class TPipeReader {
   piped_forker &d_piper;
   ssize_t       d_bytesInRawBuffer;
   PChar         d_pRawBuffer;
   char          d_rawBuffer[1024];

   int           RdChar();

   enum { EMPTY = -1 };

public:

   TPipeReader( piped_forker &piper ) : d_piper( piper ), d_bytesInRawBuffer( 0 ), d_pRawBuffer( d_rawBuffer ) {}
   ~TPipeReader() { }

   int GetFilteredLine( PXbuf xb );
   };

int TPipeReader::RdChar() {
   if( d_bytesInRawBuffer == 0 ) {
      d_bytesInRawBuffer = d_piper.Read( BSOB(d_rawBuffer) );
      if( 0 == d_bytesInRawBuffer ) {
         return EMPTY;
         }
      d_pRawBuffer = d_rawBuffer;
      }

   --d_bytesInRawBuffer;
   const char rv( *d_pRawBuffer++ );
   return rv;
   }


int TPipeReader::GetFilteredLine( PXbuf xb ) { enum { DB=0 };
   xb->clear();
   auto lastCh(0);
   while( 1 ) {
      lastCh = RdChar();               DB && DBG("   %d", lastCh );
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

enum CP_PIPED_RC {
   CP_PIPED_RC_OK = 0,
   CP_PIPED_RC_EPIPE,
   CP_PIPED_RC_ECREATEPROCESS,
   CP_PIPED_RC_EGETEXITCODE,
   };

STATIC_FXN CP_PIPED_RC CreateProcess_piped
   ( piped_forker &piper
   , int  *pd_hProcessExitCode
   , PFBUF pfLogBuf
   , PChar pS
   , int cmdFlags
   , PXbuf CommandLine
   , PXbuf xb
   ) {
   CommandLine->FmtStr( "-%s", pS ); // leading '-' is (at most) for PutLastLogLine _only_
   0 && DBG( "%s: CommandLine='%s'", __func__, CommandLine->c_str() );
   const auto pXeq(      CommandLine->wbuf () + 1 );  // skip the '-' always (stupid system takes PChar cmdline param)
   const auto pXeqConst( CommandLine->c_str() + 1 );  // skip the '-' always (for internal use)
   if( !(cmdFlags & NO_ECHO_CMDLN) ) {
      PutLastLogLine( pfLogBuf, CommandLine->c_str() + ((cmdFlags & IGNORE_ERROR) ? 0 : 1 ) );
      }

      //  ^c:\klg\bin\unxutils\ls.exe
      //  -ls -l && sleep 2 && echo hello world
      //  -ls -l
      //  -ls -l && sleep 10

   if( !piper.ForkChildOk( pXeqConst ) ) {
      char erbuf[265];
      OsErrStr( BSOB(erbuf) );
      ErrorDialogBeepf( "%s: piper.ForkChildOk '%s' FAILED: %s!!!", __func__, pXeqConst, erbuf );
      return CP_PIPED_RC_ECREATEPROCESS;
      }
   auto rv( CP_PIPED_RC_OK );
   TPipeReader pipeReader( piper );
   while( 1 ) {
      if( pipeReader.GetFilteredLine( xb ) > 0 ) {
         PutLastLogLine( pfLogBuf, xb->c_str() );
         } // as long as data isn't exhausted, don't even check for process death
      else {
         // process has terminated;
         const auto status( piper.ReapChild() );
         if( status && (cmdFlags & IGNORE_ERROR) ) {
            PutLastLogLine( pfLogBuf, FmtStr<64>( "-   process exit code=%d ignored", status ).k_str() );
            }
         return rv;
         }
      }
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

   int                         d_Pid;
   int                         d_hProcessExitCode;

   std::mutex                  d_jobQueueMtx;

   // Win32::ManualClrEvent       d_AllJobsDone;  BUGBUG

   std::thread                 d_hThread;  // should be LAST!!!

   STATIC_FXN void ChildProcessCtrlThread( InternalShellJobExecutor *pIsjx );

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
   , d_hProcessExitCode ( 0 )
   , d_hThread          ( InternalShellJobExecutor::ChildProcessCtrlThread, this )
   {
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
   // Win32::AutoSignalEvent amce( d_AllJobsDone );
   while( true ) { //**************** outerthreadloop ****************
      StringListEl *pEl;
      {
      // AutoMutex LockTheJobQueue( d_jobQueueMtx ); // ##################### LockTheJobQueue ######################
      d_Pid = INVALID_dwProcessId;
      DispNeedsRedrawStatLn(); // ???
      if( d_pSL->empty() ) { // ONLY EXIT FROM THREAD IS HERE!!!
         linebuf buf;
         PutLastLogLine( d_pfLogBuf, showTermReason( BSOB(buf), d_hProcessExitCode, unstartedJobCnt, failedJobsIgnored, pc.Capture() ) );
         d_hProcessExitCode = 0;
         // d_hThread = nullptr;
         return; // ##################### LockTheJobQueue ######################
         }
      DLINK_REMOVE_FIRST( d_pSL->Head(), pEl, dlink );
      } // ##################### LockTheJobQueue ######################

      auto cmdFlags(0);
      PChar pS( analyze_cmdline( pEl->string, &cmdFlags ) );
      if( *pS ) { prep_cmdline( pS, cmdFlags, __func__ ); }

      piped_forker piper;
      const auto cp_rc( CreateProcess_piped( piper, &d_hProcessExitCode, d_pfLogBuf, pS, cmdFlags, &x1, &x2 ) );
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


STATIC_CONST auto cpct_start_fmts = "%s::CPCT vvvvvvvvvvvvvvvvvv THREAD STARTS vvvvvvvvvvvvvvvvvv";
STATIC_CONST auto cpct_exit_fmts  = "%s::CPCT ^^^^^^^^^^^^^^^^^^ THREAD EXITS  ^^^^^^^^^^^^^^^^^^";

void InternalShellJobExecutor::ChildProcessCtrlThread( InternalShellJobExecutor *pIsjx ) {
   0 && DBG( cpct_start_fmts, "ISJE" );
                                        pIsjx->ThreadFxnRunAllJobs();
   0 && DBG( cpct_exit_fmts , "ISJE" );
   // equivalent to ExitThread( 0 );
   }


PFBUF StartInternalShellJob( StringList *sl, bool fAppend ) {
   STATIC_VAR size_t s_nxt_shelljob_output_FBUF_num;
   if( !fAppend ) {
NEXT_OUTBUF:
      ++s_nxt_shelljob_output_FBUF_num;
      }

   char fnm[30];
   auto pFB( FBOP::FindOrAddFBuf( safeSprintf( BSOB(fnm), "<shell_output-%03" PR_SIZET "u>", s_nxt_shelljob_output_FBUF_num ) ) );
   if( pFB ) {
      if( pFB->d_pInternalShellJobExecutor ) goto NEXT_OUTBUF; // don't want to append to an FBUF currently in use by a d_pInternalShellJobExecutor
      pFB->PutFocusOn();
      pFB->d_pInternalShellJobExecutor = new InternalShellJobExecutor( pFB, sl, true );
      }
   return pFB;
   }


int InternalShellJobExecutor::DeleteAllEnqueuedJobs_locks() {
   // AutoMutex LockTheJobQueue( d_jobQueueMtx );
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

   if(   INVALID_dwProcessId != d_Pid
      && ConIO::Confirm( "Kill background %s process (PID=%d)?", d_pfLogBuf->Name(), d_Pid )
      && INVALID_dwProcessId != d_Pid
      ) {
      PCChar msg = "WTF!?";
#if 0
      switch( Win32::TerminateApp( d_Pid, 2000 ) ) {
         default                      : msg = "WTF!?"                    ;  break;
         case TA_FAILED               : msg = "TA_FAILED"                ;  break;
         case TA_SUCCESS_CTRL_BREAK   : msg = "TA_SUCCESS_CTRL_BREAK"    ;  break;
         case TA_SUCCESS_WM_CLOSE     : msg = "TA_SUCCESS_WM_CLOSE"      ;  break;
         case TA_SUCCESS_TERM_PROCESS : msg = "TA_SUCCESS_TERM_PROCESS"  ;  break;
         }
#endif

      d_Pid = INVALID_dwProcessId;
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

STATIC_FXN void IdleThread() {
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
   }

void InitJobQueues() {
   // IdleThread is quasi-related to JobQueues: it updates <compile> window when
   // writer is a spawned process...
   //
   std::thread idle( IdleThread ); idle.detach();
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
