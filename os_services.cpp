
#include "ed_main.h"

class OsEnv {
   Path::str_t d_exe_path;  // "C:\dir1\dir2\" (includes trailing '\')
   Path::str_t d_exe_name;  // "k"

   public: //===================================================================

   OsEnv();

   PCChar ExePath() const { return d_exe_path.c_str(); }  // includes trailing '\'
   PCChar ExeName() const { return d_exe_name.c_str(); }
   };

OsEnv::OsEnv() {
#if defined(_WIN32)
   pathbuf pb;
   const auto len( Win32::GetModuleFileName( nullptr, BSOB(pb) ) );
   char exe_all   [ _MAX_PATH+1 ];  // "C:\dir1\dir2\k.exe"
   if( len >= sizeof(exe_all) ) {
      DBG( "GetModuleFileName rv (%ld) >= sizeof(pb) (%" PR_SIZET "u)\n", len, sizeof(pb) );
      Win32::ExitProcess( 1 );
      }
   d_exe_path = Path::CpyDirnm( pb ); 0 && DBG( "d_exe_path=%s\n", d_exe_path.c_str() );
   d_exe_name = Path::CpyFnm  ( pb ); 0 && DBG( "d_exe_name=%s\n", d_exe_name.c_str() );
#else
   static const char s_link_nm[] = "/proc/self/exe";
   if( 0 ) {
      // this is pointless since there is a race condition: between (this)
      // lstat and the readlink call, the link content could be changed
      struct stat sb;
      if( lstat( s_link_nm, &sb ) == -1 ) {
         perror( "lstat(/proc/self/exe) failed" );
         }
      }
   size_t bufbytes = PATH_MAX;
   for(;;) {
      char *linkname = static_cast<PChar>( malloc( bufbytes ) );
      if( !linkname ) {
         perror( "readlink(/proc/self/exe) memory exhausted" );
         }
      ssize_t r = readlink( s_link_nm, linkname, bufbytes );
      if( r < 0 ) {
         free( linkname );
         perror( "readlink(/proc/self/exe) < 0" );
         }
      if( r < bufbytes ) {
         linkname[r] = '\0';
         d_exe_path = Path::CpyDirnm( linkname ); 0 && DBG( "d_exe_path=%s\n", d_exe_path.c_str() );
         d_exe_name = Path::CpyFnm  ( linkname ); 0 && DBG( "d_exe_name=%s\n", d_exe_name.c_str() );
         free( linkname );
         return;
         }
      free( linkname );
      bufbytes *= 2;
      }
#endif
   }

STATIC_VAR OsEnv *g_Process;

void ThisProcessInfo::Init() {
   g_Process = new OsEnv();
   DBG_init();
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


#if !NO_LOG

#if defined(_WIN32)

void DBG_init() {
   DBG( "DBGVIEWCLEAR" );    // clear DbgView buffer
   Win32::SetFileApisToOEM();
   }

int DBG( char const *kszFormat, ...  ) {
   va_list args;  va_start(args, kszFormat);

   STATIC_CONST char prefix[] = "K! ";
   enum { PFX_LEN = KSTRLEN(prefix) };

   char szBuffer[257];
   memcpy( szBuffer, prefix, PFX_LEN+1 );
   vsnprintf(szBuffer+PFX_LEN, (sizeof(szBuffer)-1)-PFX_LEN, kszFormat, args);

   DebugLog(szBuffer);

   va_end(args);
   return 1; // so we can use short-circuit bools like (DBG_X && DBG( "got here" ))
   }

#else

// Linux version

STATIC_VAR FILE *ofh_DBG;

void DBG_init() {
   if( !ofh_DBG ) {
      pathbuf buf;
      snprintf( BSOB( buf ), "%sDBG_%s.%d", ThisProcessInfo::ExePath(), ThisProcessInfo::ExeName(), getpid() );
      ofh_DBG = fopen( buf, "w" );
      if( ofh_DBG == nullptr ) {
         perror("DBG_init() fopen");
         exit(EXIT_FAILURE);
         }
      const auto tnow( time( nullptr ) );
      const auto lt( localtime( &tnow ) );
      if( lt == nullptr ) {
         perror("localtime");
         exit(EXIT_FAILURE);
         }
      char tmstr[100];
      if( strftime( BSOB(tmstr), "%Y%m%dT%H%M%S", lt ) == 0 ) {
         perror( "strftime returned 0" );
         exit(EXIT_FAILURE);
         }
      fprintf( stderr , "opened %s @ %s\n", buf, tmstr );
      fprintf( ofh_DBG, "opened %s @ %s\n", buf, tmstr );
      }
   }

int DBG( char const *kszFormat, ...  ) {
   auto ofh( ofh_DBG ? ofh_DBG : stdout );
   va_list args;  va_start(args, kszFormat);
   vfprintf( ofh, kszFormat, args );
   va_end(args);
   fputc( '\n', ofh );
   fflush( ofh );
   return 1; // so we can use short-circuit bools like (DBG_X && DBG( "got here" ))
   }

#endif

#endif
