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

#include "os_services.h"

#include "stringlist.h"

//--------------------------------------------------------------------------------------------

#if defined(_WIN32)
extern int    EditorLoadCount();
extern bool   EditorLoadCountChanged();

extern bool   RunChildSpawnOrSystem( PCChar pCmdStr );
extern bool   KillAllBkgndProcesses();


extern size_t GetProcessMem();

extern bool   IsCompileJobQueueThreadActive();

extern bool   CompileJobQueueWaitExeDoneTimedout( int timeoutMS );

extern int    CompilePty_CmdsAsyncExec( const StringList &sl, bool fAppend );

extern int    CompilePty_KillAllJobs();

extern void   win_fully_on_desktop();

extern int    StartGuiProcess( PCChar pFullCommandLine );
extern int    StartConProcess( PCChar pFullCommandLine );
extern void   StartShellExecuteProcess( PCChar pFullCmdLn, PCChar pExeFile=nullptr );
// extern int    StartProcess( PCChar pFullCommandLine, int fWaitForProcessDone, int fDetachedProcess );
#else
STIL int    EditorLoadCount() { return 1; }
STIL bool   EditorLoadCountChanged() { return false; }
STIL bool   RunChildSpawnOrSystem( PCChar pCmdStr ) { return true; }
STIL bool   KillAllBkgndProcesses() { return true; }
STIL size_t GetProcessMem() { return 8 * 1024 * 1024; }
STIL bool   IsCompileJobQueueThreadActive() { return false; }
STIL bool   CompileJobQueueWaitExeDoneTimedout( int timeoutMS ) { return true; }
STIL int    CompilePty_CmdsAsyncExec( const StringList &sl, bool fAppend ) { return 0; }
STIL int    CompilePty_KillAllJobs() { return 1; }
STIL void   win_fully_on_desktop() {}
STIL int    StartGuiProcess( PCChar pFullCommandLine ) { return 1; }
STIL int    StartConProcess( PCChar pFullCommandLine ) { return 1; }
STIL void   StartShellExecuteProcess( PCChar pFullCmdLn, PCChar pExeFile=nullptr ) {}
#endif

extern PFBUF  StartInternalShellJob( StringList *sl, bool fAppend );
extern void   InitJobQueues();

extern PCChar OsErrStr( PChar dest, size_t sizeofDest, int errorCode );
extern PCChar OsErrStr( PChar dest, size_t sizeofDest );
