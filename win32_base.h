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

#pragma once

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

//-----------------------------------------------------------------------------
//
//  the following ARE defined by default by the compiler (VC71),
//  thus need NOT be defined by makefiles, etc.:
//      _X86_, WIN32, _WIN32, _WIN32_IE, WINVER
//
//  the following ARE NOT defined by default by the compiler
//      _WINNT        (s/b defined, no value)
//      _WIN32_WINNT  (s/b =0x0500)
//
//  optional defines (defined, no value) for smaller code (or faster compiles)
//      WIN32_LEAN_AND_MEAN
//      NOSERVICE
//      NOMCX
//      NOIME
//      NOSOUND
//      NOCOMM
//      NOKANJI
//      NORPC
//      NOPROXYSTUB
//      NOIMAGE
//      NOTAPE
//
// 20061004 klg
//
// control-defines that speed up windows compiles by inhibiting inclusion of
// seldom-used .h files.  MUST BE defined BEFORE windows.h is #included.
// http://support.microsoft.com/default.aspx?scid=kb;en-us;166474
//
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOIME
#define NOMCX
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE

// USING_MS_OEM_CHARSET should only change (from 1) when unix port is done
//
#define USING_MS_OEM_CHARSET  1

// MSVC uses certain values to identify certain states.
//
// CCCCCCCC means a local variable which was never initialized
// CDCDCDCD means a class variable which was never initialized
// DDDDDDDD means you're reading a value through a pointer to an object which was deleted.
//

//
// More modern way to export functions from a DLL: give them the DLLX attribute
//
#if defined(__GNUC__)
#define DLLX  EXTERNC __attribute__ ((dllexport,stdcall))
#else
#define DLLX  EXTERNC _declspec( dllexport )
#endif

namespace Win32 { // Win32::
   #include <windows.h>
   #include <winnls.h>

   // wrap various windows.h macros which reference typenames, etc. that
   // are influenced by 'namespace':
   //
   STIL HANDLE Invalid_Handle_Value()  { return INVALID_HANDLE_VALUE;  }
   STIL DWORD  Std_Input_Handle()      { return STD_INPUT_HANDLE;      }
   STIL DWORD  Std_Error_Handle()      { return STD_ERROR_HANDLE;      }
   STIL DWORD  Std_Output_Handle()     { return STD_OUTPUT_HANDLE;     }
   STIL DWORD  Wait_Object_0()         { return WAIT_OBJECT_0;         }
   STIL DWORD  Status_Control_C_Exit() { return STATUS_CONTROL_C_EXIT; }
   STIL int    MAKELANGID__LANG_NEUTRAL_SUBLANG_DEFAULT() { return MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ); }
   }

namespace Win32 { // Win32::
   ConfirmResponse Confirm_MsgBox( int MBox_uType, PCChar prompt );
   }

// Conditional-compile switch to en/disable PSAPI usage (NT doesn't
// intrinsically support PSAPI, and you never know when someone MIGHT need to
// run on NT!)
//
#define  USE_WINDOWS_PSAPI  1
#if      USE_WINDOWS_PSAPI
namespace Win32 { // Win32::
   namespace PSAPI { // Win32::PSAPI
      #include <psapi.h>
      }
   }
#endif

//--------------------------------------------------------------------------------------------

extern void WinClipGetFirstLine( std::string &xb );

//##############################################################################

class Win32ConsoleFontChanger {
   // 20090613 kgoodwin added per http://cboard.cprogramming.com/windows-programming/102187-console-font-size.html
   Win32::PCONSOLE_FONT_INFO d_pFonts; // does double duty as "valid" flag
   Win32::PCOORD             d_pFontSizes;

   Win32::DWORD              d_origFont;
   Win32::DWORD              d_setFont;
   Win32::DWORD              d_num_fonts;
   Win32::HWND               d_hwnd;
   Win32::HANDLE             d_hConout;
   // undocumented kernel32.dll ConsoleFont functions which are accessed @ runtime:
   Win32::BOOL     (WINAPI * d_SetConsoleFont)         (Win32::HANDLE, Win32::DWORD);
   Win32::BOOL     (WINAPI * d_GetConsoleFontInfo)     (Win32::HANDLE, Win32::BOOL, Win32::DWORD, Win32::PCONSOLE_FONT_INFO);
   Win32::DWORD    (WINAPI * d_GetNumberOfConsoleFonts)();

   bool validFontIdx( int idx ) const { const auto ix( idx ); return d_pFonts && ix >= 0 && ix < d_num_fonts; }

   public: //===================================================================

    Win32ConsoleFontChanger();
   ~Win32ConsoleFontChanger();
   Win32::DWORD    OrigFont() const { return d_pFonts ? d_origFont  : 0; }
   Win32::DWORD    NumFonts() const { return d_pFonts ? d_num_fonts : 0; }
   void            GetFontInfo();
   void            SetOrigFont() { if( d_pFonts && d_setFont != d_origFont ) SetFont( d_origFont ); }
   void            SetFont( Win32::DWORD idx );
   int             GetFontSizeX() const;
   int             GetFontSizeY() const;
   double          GetFontAspectRatio( int idx ) const;
   };


//##############################################################################

STIL void DebugLog( PCChar string ) { Win32::OutputDebugString( string ); }

extern bool IsFileReadonly( PCChar pFBufName );
extern bool FileOrDirExists( PCChar lpFBufName );

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
