//
// Copyright 2016-2022 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

//------------------------------------------------------------------------------
#if !defined(__GNUC__)
//
// duplicated from wincon.h for debug purposes
//
//  Input Mode flags:
//
#define ENABLE_PROCESSED_INPUT 0x0001
#define ENABLE_LINE_INPUT      0x0002
#define ENABLE_ECHO_INPUT      0x0004
#define ENABLE_WINDOW_INPUT    0x0008
#define ENABLE_MOUSE_INPUT     0x0010

// 20070129 kgoodwin recently-documented flags allow directly disabling QuickEdit mode!!!
//
// NB: bits ENABLE_INSERT_MODE and ENABLE_QUICK_EDIT, whether set or clear,
//     ***ONLY take effect*** if ENABLE_EXTENDED_FLAGS is also set.
//
#define ENABLE_INSERT_MODE     0x0020
#define ENABLE_QUICK_EDIT      0x0040
#define ENABLE_EXTENDED_FLAGS  0x0080
#define ENABLE_AUTO_POSITION   0x0100
//
//
// Output Mode flags:
//
#define ENABLE_PROCESSED_OUTPUT    0x0001
#define ENABLE_WRAP_AT_EOL_OUTPUT  0x0002
//
#endif

//------------------------------------------------------------------------------
//
// Application-level defs:
//
// To disable QuickEdit mode, set ENABLE_EXTENDED_FLAGS while keeping
// ENABLE_QUICK_EDIT cleared.  We don't want to ENABLE_INSERT_MODE, so keep it
// cleared too:
//
#define DISABLE_QUICK_EDIT  (ENABLE_EXTENDED_FLAGS)

// in order to use the mouse in a console app, QuickEdit mode MUST BE disabled
#define CIM_USE_MOUSE       (ENABLE_MOUSE_INPUT | DISABLE_QUICK_EDIT)

// this is apparently NOT defined in the MinGW version of windows.h
#define ENABLE_QUICK_EDIT      0x0040

// use ENABLE_QUICK_EDIT_INS_MODES when the editor is exited: the parent console
// inherits them, which in the case of shells, is a good thing.
//
#define ENABLE_QUICK_EDIT_INS_MODES  (ENABLE_EXTENDED_FLAGS|ENABLE_INSERT_MODE|ENABLE_QUICK_EDIT)
//
//------------------------------------------------------------------------------

namespace Win32 {
   #include "shellapi.h"
   }

#if defined(__GNUC__)
   #define K_STDCALL __stdcall
#else
   #define K_STDCALL
#endif

#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

// see http://cboard.cprogramming.com/windows-programming/102187-console-font-size.html
template<typename pfn_t>
inline bool LoadFuncOk( pfn_t &fn, Win32::HMODULE hmod, const char *name) {
   fn = reinterpret_cast<pfn_t>(Win32::GetProcAddress(hmod, name));
   return fn != nullptr;
   }

#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic pop
#endif

struct WhileHoldingGlobalVariableLock
   {
   WhileHoldingGlobalVariableLock()  ;
   ~WhileHoldingGlobalVariableLock() ;
   };

extern void MainThreadGiveUpGlobalVariableLock()  ;
extern void MainThreadWaitForGlobalVariableLock() ;

extern void ConsoleInputAcquire() ;

class ConsoleInputModeRestorer {
   const Win32::DWORD d_cim;
public:
   ConsoleInputModeRestorer() ;
   void Set() const           ;
   };

//--------------------------------------------------------------------------------------------

#include <mutex>

//--------------------------------------------------------------------------------------------

// this experiment FAILED: leaving here so I don't repeat it in a year or more.
//
// Fundamental problem: performing an exit with this code enabled KILLS the
// parent shell as well as this process!  WTF?!
//
// 20080331 kgoodwin

#define  EXPERIMENT_HANDLE_CTRL_CLOSE_EVENT  0
#if      EXPERIMENT_HANDLE_CTRL_CLOSE_EVENT
#define  HANDLE_CTRL_CLOSE_EVENT( x )   x
#else
#define  HANDLE_CTRL_CLOSE_EVENT( x )
#endif

extern const Win32::HANDLE g_hCurProc; // == Win32::GetCurrentProcess()
extern volatile bool g_fSystemShutdownOrLogoffRequested;
extern void Conin_Init();
extern void ConinRelease();
extern void VidInitApiError( PCChar errmsg );
extern void UpdateConsoleTitle_Init();
extern void UpdateConsoleTitle();
extern bool IsWin7();
extern bool IsWin7OrLater();
extern PChar GetCPName( PChar buf, size_t sizeofBuf, Win32::UINT cp );
