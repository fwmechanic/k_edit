//
// Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

void ConOut::Bell() {
   if( g_fAllowBeep ) { fputc( '\a', stdout ); } // write RTL's stdout, which hasn't been touched since startup
   }

// takes counted-string param so *pStart doesn't have to be forcibly
// NUL-terminated (it may be a const string)
//
PCChar Getenv( stref varnm ) {
   ALLOCA_STRDUP( buf, slen, varnm.data(), varnm.length() )    0 && DBG("Getenv '%s'", buf );
   return getenv( buf );
   }

//------------------------------------------------

bool PutEnvOk( PCChar varName, PCChar varValue ) { // params canNOT be stref since non-_WIN32 API which takes ASCIZ strings is called directly
   // nullptr == varValue || !*varValue result in unsetenv (variable undefine) operation being performed
   if( nullptr == varValue ) { varValue = ""; }  // promote nullptr to empty string
   0 && DBG( "*** %s(%s=%s)", FUNC, varName, varValue );
#if defined(_WIN32)
   // MSVC-specific _putenv COPIES params (therefore SAFE to use with auto buffer params); if empty string follows "=", unsetenv is performed
   const auto ln( Strlen(varName) );
   const auto lv( Strlen(varValue) );
   const auto bs( ln + lv + (1+1) ); // (1+1) = ('=' + '\0')
   auto bp( PChar( alloca( bs ) ) );
   scat( bp, bs, stref(varValue,lv), scat( bp, bs, "=", scpy( bp, bs, stref(varName,ln) ) ) );
   const auto ok( 0 == _putenv( bp ) );
#else
   // posix putenv NOT USED because parameter becomes part of process-duration environment of process (therefore should never pass auto buffers to it)
   // posix setenv     USED because it COPIES params into process-duration environment (therefore SAFE to use with auto buffer params)
   const auto ok( 0 == (*varValue ? setenv( varName, varValue, 1 ) : unsetenv( varName )) );
#endif
   if( !ok ) {
      ErrorDialogBeepf( "%s(%s=%s) FAILED: %s", FUNC, varName, varValue, strerror( errno ) );
      }
   return ok;
   }

STATIC_FXN bool PutEnvOk( PCChar szNameEqualsVal ) {
   0 && DBG( "*** %s(%s)", FUNC, szNameEqualsVal );
   const auto pEQ( strchr( szNameEqualsVal, '=' ) );
   if( pEQ == szNameEqualsVal ) { // no name?
      return false; // not OK
      }
   if( pEQ == nullptr ) {
      return PutEnvOk( szNameEqualsVal, "" );
      }
   ALLOCA_STRDUP( nm, nmLen, szNameEqualsVal, pEQ - szNameEqualsVal - 1 );
   return PutEnvOk( nm, pEQ + 1 );
   }


bool PutEnvChkOk( PCChar szNameEqualsVal ) {
   if( strchr( szNameEqualsVal, '=' ) ) {
      return PutEnvOk( szNameEqualsVal );
      }
   else {
      DBG( "%s '%s' missing '='", FUNC, szNameEqualsVal );
      return false;
      }
   }

#ifdef fn_setenv
bool ARG::setenv() {
   switch( d_argType ) {
      default:      return BadArg();
      case TEXTARG: return PutEnvChkOk( d_textarg.pText );
      case LINEARG: ATTR_FALLTHRU;
      case BOXARG:  for( ArgLineWalker aw( this ) ; !aw.Beyond() ; aw.NextLine() ) {
                       if( aw.GetLine() && !PutEnvChkOk( aw.c_str() ) ) {
                          return false;
                          }
                       }
                    return true;
      }
   }
#endif// fn_setenv

//------------------------------------------------

class OsEnv {
   Path::str_t d_exe_path;  // "C:\dir1\dir2\" (includes trailing '\')
   Path::str_t d_exe_name;  // "k"
   std::string d_euname;
   std::string d_hostname;
   int         d_euid;

   public: //===================================================================

   OsEnv();

   PCChar ExePath()   const { return d_exe_path.c_str(); }  // includes trailing '\'
   PCChar ExeName()   const { return d_exe_name.c_str(); }
#if !defined(_WIN32)
   int    euid()      const { return d_euid; }
   PCChar euname()    const { return d_euname.c_str(); }
   PCChar hostname()  const { return d_hostname.c_str(); }
#endif
   };

#if !defined(_WIN32)
#  include <pwd.h>
#endif

OsEnv::OsEnv() {
#if defined(_WIN32)
   pathbuf pb;
   const auto len( Win32::GetModuleFileName( nullptr, BSOB(pb) ) );
   char exe_all   [ _MAX_PATH+1 ];  // "C:\dir1\dir2\k.exe"
   if( len >= sizeof(exe_all) ) {
      DBG( "GetModuleFileName rv (%ld) >= sizeof(pb) (%" PR_SIZET ")\n", len, sizeof(pb) );
      Win32::ExitProcess( 1 );
      }
   d_exe_path.assign( sr2st( Path::RefDirnm( pb ) ) ); 0 && DBG( "d_exe_path=%s\n", d_exe_path.c_str() );
   d_exe_name.assign( sr2st( Path::RefFnm  ( pb ) ) ); 0 && DBG( "d_exe_name=%s\n", d_exe_name.c_str() );
#else
   const auto pw( getpwuid( d_euid=geteuid() ) );
   if( pw ) { d_euname.assign( pw->pw_name ); }
   { char hnbuf[257]; hnbuf[0] = '\0';
   if( 0 == gethostname( BSOB( hnbuf ) ) && hnbuf[0] ) {
      d_hostname.assign( hnbuf );
      }
   }

   STATIC_CONST char s_link_nm[] = "/proc/self/exe";
   if( 0 ) {
      // this is pointless since there is a race condition: between (this)
      // lstat and the readlink call, the link content could be changed
      struct stat sb;
      if( lstat( s_link_nm, &sb ) == -1 ) {
         perror( "lstat(/proc/self/exe) failed" );
         return;
         }
      }
   size_t bufbytes = PATH_MAX;
   for(;;) {
      char *linkname = static_cast<PChar>( malloc( bufbytes ) );
      if( !linkname ) {
         perror( "readlink(/proc/self/exe) memory exhausted" );
         free( linkname );
         return;
         }
      ssize_t r = readlink( s_link_nm, linkname, bufbytes );
      if( r < 0 ) {
         perror( "readlink(/proc/self/exe) < 0" );
         free( linkname );
         return;
         }
      if( r < bufbytes ) {
         linkname[r] = '\0';
         d_exe_path.assign( sr2st( Path::RefDirnm( linkname ) ) ); 0 && DBG( "d_exe_path=%s\n", d_exe_path.c_str() );
         d_exe_name.assign( sr2st( Path::RefFnm  ( linkname ) ) ); 0 && DBG( "d_exe_name=%s\n", d_exe_name.c_str() );
         free( linkname );
         return;
         }
      free( linkname );
      bufbytes *= 2;
      }
#endif
   }

COMPLEX_STATIC_VAR OsEnv *g_Process;

void ThisProcessInfo::Init() {
   g_Process = new OsEnv();
   }

PCChar ThisProcessInfo::ExePath()  { return g_Process->ExePath(); }  // includes trailing '\'
PCChar ThisProcessInfo::ExeName()  { return g_Process->ExeName(); }
#if !defined(_WIN32)
int    ThisProcessInfo::euid()     { return g_Process->euid(); }
PCChar ThisProcessInfo::euname()   { return g_Process->euname(); }
PCChar ThisProcessInfo::hostname() { return g_Process->hostname(); }
#endif

InterlockedExchangeOperand s_fHaltExecution;

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
      if( pCur->state() == paused ) {
          pCur->Start_( now );
          }
      }
   }
