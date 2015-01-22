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

#include "ed_main.h"
#include "win32_pvt.h"


//================================================================================================================
//
// s_GlobalVariableLock is a Win32 CRITICAL_SECTION
//
// We use it to allow multiple threads to exclusively access the "MainThread"
// variables (the vast majority of global editor data structures).  When the
// main thread waits for user input, it passes the lock to other threads that
// might be waiting for it.
//
// Due to the organization of the code, there are two circumstances:
//
// 1) (main thread): assumed to hold the lock except during small regions of
//    the code during which the lock is relinquished.  The code construct used
//    "releases ownership during a particular scope".  Due to the conditionals
//    involved, I have not created a class/object for this case.
//
// 2) (aux threads): assumed to NOT hold the lock except during small regions
//    of the code during which the lock is held.  The code construct used
//    "takes ownership during a particular scope": construct then destroy a
//    WhileHoldingGlobalVariableLock object.
//
//================================================================================================================

const Win32::DWORD mainThreadId( Win32::GetCurrentThreadId() );

#define  ASSERT_MAIN_THREAD()      Assert(mainThreadId == Win32::GetCurrentThreadId())
#define  ASSERT_NOT_MAIN_THREAD()  Assert(mainThreadId != Win32::GetCurrentThreadId())


STATIC_VAR AcquiredMutex s_GlobalVariableLock;
#define GiveUpGlobalVariableLock()  ( /* DBG( "rls baton" ), */ s_GlobalVariableLock.Release() )
#define WaitForGlobalVariableLock() (                           s_GlobalVariableLock.Acquire() /* , DBG( "got baton" ) */ )

WhileHoldingGlobalVariableLock::WhileHoldingGlobalVariableLock()  { WaitForGlobalVariableLock(); }
WhileHoldingGlobalVariableLock::~WhileHoldingGlobalVariableLock() { GiveUpGlobalVariableLock();  }

void MainThreadGiveUpGlobalVariableLock()  { ASSERT_MAIN_THREAD();  MainThreadPerfCounter::PauseAll() ;  GiveUpGlobalVariableLock() ; }
void MainThreadWaitForGlobalVariableLock() { ASSERT_MAIN_THREAD();  WaitForGlobalVariableLock()       ;  MainThreadPerfCounter::ResumeAll()   ; }

#undef GiveUpGlobalVariableLock
#undef WaitForGlobalVariableLock

//
// do NOT reference s_GlobalVariableLock by name below this line!
//================================================================================================================

#pragma GCC diagnostic pop
