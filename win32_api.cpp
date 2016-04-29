//
// Copyright 2015-2016 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

PChar GetCPName( PChar buf, size_t sizeofBuf, Win32::UINT cp ) {
   Win32::CPINFOEX cpix;
   if( Win32::GetCPInfoEx( cp, 0, &cpix ) ) {
      scpy( buf, sizeofBuf, cpix.CodePageName );
      }
   else {
      scpy( buf, sizeofBuf, "?" );
      }
   return buf;
   }

void FBufRead_Assign_OsInfo( PFBUF pFBuf ) {
   pFBuf->FmtLastLine( "#-------------------- %s", "Win32 Status" );
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
   if( errorCode ) {
      Win32::FormatMessage(
           0x1000 // Win32::FORMAT_MESSAGE_FROM_SYSTEM
         | 0x0200 // Win32::FORMAT_MESSAGE_IGNORE_INSERTS
         | 0x00FF // Win32::FORMAT_MESSAGE_MAX_WIDTH_MASK
         , nullptr, errorCode, 0, dest, sizeofDest, nullptr
         );
      }
   return dest;
   }

PCChar OsErrStr( PChar dest, size_t sizeofDest ) {
   return OsErrStr( dest, sizeofDest, Win32::GetLastError() );
   }

//#############################################################################################################################
//###########################################  Win32 DIALOG BOXES, ETC  #######################################################
//#############################################################################################################################

int ConIO::DbgPopf( PCChar fmt, ... ) {
   FixedCharArray<512> buf;
   va_list val;
   va_start( val, fmt );
   buf.Vsprintf( fmt, val );
   va_end( val );
   DebugLog( buf.k_str() );
   MainThreadPerfCounter::PauseAll();
   const auto retq( Win32::MessageBox(
      nullptr,                           // handle of owner window
      buf.c_str(),                       // address of text in message box
      "Kevin's _awesome_ editor DEBUG",  // address of title of message box
      MB_TASKMODAL | MB_SETFOREGROUND
      ) );
   MainThreadPerfCounter::ResumeAll();
   return retq == IDYES;
   }

void AssertDialog_( PCChar function, int line ) {
   ConIO::DbgPopf( "Assertion failed, %s L %d", function, line );
   }

#if DEBUG_LOGGING

void GotHereDialog_( bool *dialogShown, PCChar fn, int lnum ) {
   FmtStr<100> msg( "GotHere: %s line %d", fn, lnum );
   DBG( "%s", msg.k_str() );
   if( !*dialogShown ) {
      ConIO::DbgPopf( "%s", msg.k_str() );
      }
   *dialogShown = true;
   }

#endif//DEBUG_LOGGING

ConfirmResponse Win32::Confirm_MsgBox( int MBox_uType, PCChar prompt ) {
   const auto mboxrv = Win32::MessageBox(
        nullptr,                       // handle of owner window
        prompt,                        // address of text in message box
        "Kevin's _awesome_ editor!",   // address of title of message box
        MBox_uType | MB_ICONWARNING |  // styles of message box
        MB_TASKMODAL | MB_SETFOREGROUND
      );
   switch( mboxrv ) {
      case IDYES:    return crYES;
      default:       DBG( "unknown MessageBox rv!!!" ); //lint -fallthrough
      case IDNO:     return crNO;
      case IDCANCEL: return crCANCEL;
      }
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
//       return EditorTime( uint32_t( uliNow.QuadPart / 10000ul ) );
//       }
//    return EditorTime(0);
//    }

//###########################################################################################################
//###########################################################################################################
//###########################################################################################################
