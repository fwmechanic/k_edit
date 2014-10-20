//
//
// Copyright 1991 - 2014 by Kevin L. Goodwin; All rights reserved
//
//

#include "ed_main.h"

#include "win32_pvt.h"


// OsVerStr WinXP Pro = "W.5.1"
// OsVerStr Win7      = "W.6.1"
// OsVerStr Win8      = "W.6.2"
// OsVerStr Win8.1    = "W.6.2"

PCChar OsVerStr() {
   STATIC_VAR char dest[32];
   if( 0==dest[0] ) {
      Win32::OSVERSIONINFO osvi = { sizeof osvi };
      Win32::GetVersionEx(&osvi);
      _snprintf( dest, sizeof dest, "W.%u.%u", osvi.dwMajorVersion, osvi.dwMinorVersion );
      }
   return dest;
   }

bool IsWin7() {
   Win32::OSVERSIONINFO osvi = { sizeof osvi };
   Win32::GetVersionEx(&osvi);
   const auto major( osvi.dwMajorVersion );
   const auto minor( osvi.dwMinorVersion );
   return ((major == 6) && (minor == 1));
   }


bool IsWin7OrLater() {
   Win32::OSVERSIONINFO osvi = { sizeof osvi };
   Win32::GetVersionEx(&osvi);
   const auto major( osvi.dwMajorVersion );
   const auto minor( osvi.dwMinorVersion );
   return (major > 6) || ((major == 6) && (minor >= 1));
   }


STATIC_FXN PChar GetCPName( PChar buf, size_t sizeofBuf, Win32::UINT cp ) {
   Win32::CPINFOEX cpix;
   if( Win32::GetCPInfoEx( cp, 0, &cpix ) ) {
      safeStrcpy( buf, sizeofBuf, cpix.CodePageName );
      }
   else {
      safeStrcpy( buf, sizeofBuf, "?" );
      }
   return buf;
   }

void FBufRead_Assign_Win32( PFBUF pFBuf ) {
   pFBuf->FmtLastLine( "OsVerStr                 = %s" , OsVerStr() );

   // To set a console's input code page, use the SetConsoleCP function. To set and query a console's output code page, use the SetConsoleOutputCP and GetConsoleOutputCP functions.
   pathbuf pbuf;
   pFBuf->FmtLastLine( "Console INPUT  Code Page = Win32::GetConsoleCP()       = %s" , GetCPName( BSOB(pbuf), Win32::GetConsoleCP() ) );
   pFBuf->FmtLastLine( "Console OUTPUT Code Page = Win32::GetConsoleOutputCP() = %s" , GetCPName( BSOB(pbuf), Win32::GetConsoleOutputCP() ) );
   pFBuf->FmtLastLine( "Misc Code Pages:" );
   pFBuf->FmtLastLine( "  Win32::GetOEMCP()           = %s" , GetCPName( BSOB(pbuf), Win32::GetOEMCP() ) );
   pFBuf->FmtLastLine( "  Win32::GetACP() (ANSI)      = %s" , GetCPName( BSOB(pbuf), Win32::GetACP() )   );
   }


PCChar OsErrStr( PChar dest, size_t sizeofDest, int errorCode ) {
   dest[0] = '\0';
   if( errorCode )
      Win32::FormatMessage(
           0x1000 // Win32::FORMAT_MESSAGE_FROM_SYSTEM
         | 0x0200 // Win32::FORMAT_MESSAGE_IGNORE_INSERTS
         | 0x00FF // Win32::FORMAT_MESSAGE_MAX_WIDTH_MASK
         , nullptr, errorCode, 0, dest, sizeofDest, nullptr
         );

   return dest;
   }

PCChar OsErrStr( PChar dest, size_t sizeofDest ) {
   return OsErrStr( dest, sizeofDest, Win32::GetLastError() );
   }


//
//#############################################################################################################################
//###########################################  Win32 DIALOG BOXES, ETC  #######################################################
//#############################################################################################################################
//

int DbgPopf( PCChar fmt, ... ) {
   FixedCharArray<512> buf;
   va_list val;
   va_start( val, fmt );
   buf.Vsprintf( fmt, val );
   va_end( val );

   DebugLog( buf );

   MainThreadPerfCounter::PauseAll();

   const auto retq( Win32::MessageBox(
      nullptr,                           // handle of owner window
      buf,                               // address of text in message box
      "Kevin's _awesome_ editor DEBUG",  // address of title of message box
      MB_TASKMODAL | MB_SETFOREGROUND
      ) );

   MainThreadPerfCounter::ResumeAll();

   return retq == IDYES;
   }


void AssertDialog_( PCChar function, int line ) {
   DbgPopf( "Assertion failed, %s L %d", function, line );
   }

#if DEBUG_LOGGING

void GotHereDialog_( bool *dialogShown, PCChar fn, int lnum ) {
   FmtStr<100> msg( "GotHere: %s line %d", fn, lnum );
   DBG( "%s", msg.k_str() );

   if( !*dialogShown )
      DbgPopf( "%s", msg.k_str() );

   *dialogShown = true;
   }

#endif//DEBUG_LOGGING


//
// Confirm with "Cancel" button
//
// 20050910 klg wrote
//

STATIC_FXN ConfirmResponse Confirm_( int MBox_uType, PCChar pszPrompt, va_list val ) {
   linebuf b;
   vsnprintf( BSOB(b), pszPrompt, val );
   DBG( "Confirm_: %s", b );

   MainThreadPerfCounter::PauseAll();

   const auto mboxrv = Win32::MessageBox(
        nullptr,                       // handle of owner window
        b,                             // address of text in message box
        "Kevin's _awesome_ editor!",   // address of title of message box
        MBox_uType | MB_ICONWARNING |  // styles of message box
        MB_TASKMODAL | MB_SETFOREGROUND
      );

   MainThreadPerfCounter::ResumeAll();

   switch( mboxrv ) {
      case IDYES:    return crYES;
      default:       DBG( "unknown MessageBox rv!!!" ); //lint -fallthrough
      case IDNO:     return crNO;
      case IDCANCEL: return crCANCEL;
      }
   }

bool Confirm( PCChar pszPrompt, ... ) {
   va_list val;
   va_start( val, pszPrompt );
   const ConfirmResponse rv( Confirm_( MB_YESNO, pszPrompt, val ) );
   va_end( val );
   return rv == crYES;
   }

ConfirmResponse Confirm_wCancel( PCChar pszPrompt, ... ) {
   va_list val;
   va_start( val, pszPrompt );
   const ConfirmResponse rv( Confirm_( MB_YESNOCANCEL, pszPrompt, val ) );
   va_end( val );
   return rv;
   }


#ifdef fn_dvlog

GLOBAL_VAR bool g_fDvlogcmds = true; // global/switchval

bool ARG::dvlog() {
   switch( d_argType ) { // arg "DBGVIEWCLEAR" dvlog
      default:      return BadArg();
      case TEXTARG: Win32::OutputDebugString( d_textarg.pText );
                    return true;
      }
   }

#endif


typedef unsigned long
//      double         using double here raises size of DLL by 10K bytes
        EditorTime;
//
// DLLX EditorTime EdApiTimeMsNow()
//    {
//    Win32::FILETIME   ftnow;
//    Win32::SYSTEMTIME stnow;
//    Win32::GetLocalTime( &stnow );
//    if( Win32::SystemTimeToFileTime( &stnow, &ftnow ) )
//       { // FILETIME unit is 100nS
//       // Copies the FILETIME structure into the ULARGE_INTEGER structure;
//       // (64-bit value for arithmetic purposes)
//       Win32::ULARGE_INTEGER uliNow;
//       uliNow.LowPart  = ftnow.dwLowDateTime;
//       uliNow.HighPart = ftnow.dwHighDateTime;
//
//       return EditorTime( U32( uliNow.QuadPart / 10000ul ) );
//       }
//    return EditorTime(0);
//    }


//###########################################################################################################
//###########################################################################################################
//###########################################################################################################
