//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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
