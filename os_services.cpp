
#include "ed_main.h"

class OsEnv {
   Path::str_t d_exe_path;  // "C:\dir1\dir2\" (includes trailing '\')
   Path::str_t d_exe_name;  // "k"

   public: //===================================================================

   OsEnv( PCChar argv0 );

   PCChar ExePath() const { return d_exe_path.c_str(); }  // includes trailing '\'
   PCChar ExeName() const { return d_exe_name.c_str(); }
   };

OsEnv::OsEnv( PCChar argv0 ) {
#if defined(_WIN32)
   argv0 = argv0; // suppress  warning: parameter 'argv0' set but not used
   DBG( "DBGVIEWCLEAR" );    // clear DbgView buffer
   Win32::SetFileApisToOEM();
   pathbuf pb;
   const auto len( Win32::GetModuleFileName( nullptr, BSOB(pb) ) );
   char exe_all   [ _MAX_PATH+1 ];  // "C:\dir1\dir2\k.exe"
   if( len >= sizeof(exe_all) ) {
      DBG( "GetModuleFileName rv (%ld) >= sizeof(pb) (%" PR_SIZET "u)\n", len, sizeof(pb) );
      Win32::ExitProcess( 1 );
      }
   argv0 = pb;
#else
#endif
   d_exe_path = Path::CpyDirnm( argv0 ); DBG( "d_exe_path=%s\n", d_exe_path.c_str() );
   d_exe_name = Path::CpyFnm  ( argv0 ); DBG( "d_exe_name=%s\n", d_exe_name.c_str() );
   }

STATIC_VAR OsEnv *g_Process;

void ThisProcessInfo::Init( PCChar argv0 ) {
   g_Process = new OsEnv( argv0 );
   }

PCChar ThisProcessInfo::ExePath() { return g_Process->ExePath(); }  // includes trailing '\'
PCChar ThisProcessInfo::ExeName() { return g_Process->ExeName(); }

volatile InterlockedExchangeOperand s_fHaltExecution;

PCChar ExecutionHaltReason() {
   switch( s_fHaltExecution ) {
      default              : Assert( 0 )  ; return "???";
      case NEVER_REQUESTED                : return "NEVER REQUESTED";
      case USER_INTERRUPT                 : return "USER INTERRUPT";
      case CMD_ABEND                      : return "unrecoverable CMD error";
      case USER_CHOSE_EARLY_CMD_TERMINATE : return "User chose to quit CMD";
      }
   }

static_assert( NEVER_REQUESTED == 0, "NEVER_REQUESTED == 0" );

HaltReason ExecutionHaltRequested_( PCChar ident ) {
   if( s_fHaltExecution ) {
      if( ident ) {
         DBG( "ExecutionHaltRequest '%s' seen by %s", ExecutionHaltReason(), ident );
         }
      return HaltReason(s_fHaltExecution);
      }

   return NEVER_REQUESTED;
   }


#if defined(_WIN32)

PerfCounter::TCapture PerfCounter::FxnQueryPerformanceFrequency() {
   TCapture rv;
   /* Win32::*/QueryPerformanceFrequency( &rv );
   return rv;
   }

const PerfCounter::TCapture PerfCounter::s_PcFreq( FxnQueryPerformanceFrequency() );

#endif

DLinkHead <MainThreadPerfCounter> MainThreadPerfCounter::dhead;

void MainThreadPerfCounter::PauseAll() {
   TCapture now;
   GetTOD( &now );
   DLINKC_FIRST_TO_LASTA(MainThreadPerfCounter::dhead,dlink,pCur) {
      pCur->Capture_( now, paused );
      }
   }

void MainThreadPerfCounter::ResumeAll() {
   TCapture now;
   GetTOD( &now );
   DLINKC_FIRST_TO_LASTA(MainThreadPerfCounter::dhead,dlink,pCur) {
      if( pCur->state() == paused )
          pCur->Start_( now );
      }
   }
