//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

#include "os_services.h"

#include "stringlist.h"

//--------------------------------------------------------------------------------------------

extern int    EditorLoadCount();
extern bool   EditorLoadCountChanged();

extern bool   RunChildSpawnOrSystem( PCChar pCmdStr );
extern bool   KillAllBkgndProcesses();

extern void   InitJobQueues();

extern size_t GetProcessMem();

extern bool   IsCompileJobQueueThreadActive();

extern bool   CompileJobQueueWaitExeDoneTimedout( int timeoutMS );

extern int    CompilePty_CmdsAsyncExec( const StringList &sl, bool fAppend );
extern PFBUF  StartInternalShellJob( PStringList sl, bool fAppend );

extern int    CompilePty_KillAllJobs();

extern void   win_fully_on_desktop();


extern int    StartGuiProcess( PCChar pFullCommandLine );
extern int    StartConProcess( PCChar pFullCommandLine );
extern void   StartShellExecuteProcess( PCChar pFullCmdLn, PCChar pExeFile=nullptr );
// extern int    StartProcess( PCChar pFullCommandLine, int fWaitForProcessDone, int fDetachedProcess );

extern PCChar OsErrStr( PChar dest, size_t sizeofDest, int errorCode );
extern PCChar OsErrStr( PChar dest, size_t sizeofDest );
