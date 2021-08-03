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

//############  MISC VARIABLES

extern ARG   noargNoMeta;

GLOBAL_VAR inline bool g_fFuncRetVal      = false ;
GLOBAL_VAR inline bool g_fMeta            = false ;
GLOBAL_VAR inline bool g_CLI_fUseRsrcFile = true  ;

GLOBAL_VAR inline int  g_fExecutingInternal = 0 ; // to support correct recording-to-macro
GLOBAL_VAR inline int  g_ClipboardType      = 0 ; // 0 == EMPTY

extern char ** &g_envp                  ;

extern linebuf  g_delims, g_delimMirrors;

extern std::string  g_SnR_stSearch      ;
extern std::string  g_SnR_stReplacement ;

extern CMD         g_CmdTable[];  // this is ALMOST const; can't BE const because CMD contains (mutatable) fields d_argData, d_callCount & d_gCallCount

//############  global PFBUF's

GLOBAL_VAR inline PFBUF g_pFBufAssignLog    ;
GLOBAL_VAR inline PFBUF g_pFBufMsgLog       ;
GLOBAL_VAR inline PFBUF g_pFBufCmdlineFiles ;
GLOBAL_VAR inline PFBUF g_pFBufConsole      ;
GLOBAL_VAR inline PFBUF g_pFBufCwd          ;
GLOBAL_VAR inline PFBUF g_pFBufSearchLog    ;
GLOBAL_VAR inline PFBUF g_pFBufSearchRslts  ;
GLOBAL_VAR inline PFBUF g_pFBufTextargStack ;
GLOBAL_VAR inline PFBUF g_pFbufClipboard    ;
GLOBAL_VAR inline PFBUF g_pFbufRecord       ;
GLOBAL_VAR inline PFBUF s_pFbufLog          ;
GLOBAL_VAR inline PFBUF s_pFbufLuaLog       ;

//############  SWITCH VALUE VARIABLES

#ifdef _WIN32
// the chars are only valid for Win32::GetOEMCP() == 437
#define BIG_BULLET   '\xF9'
#define SMALL_BULLET '\xFA'
#define DFLT_G_CHTABDISP BIG_BULLET
#else
#define DFLT_G_CHTABDISP '>'
#endif

GLOBAL_VAR inline char  g_chTabDisp        = DFLT_G_CHTABDISP;
GLOBAL_VAR inline char  g_chTrailSpaceDisp = ' ';
GLOBAL_VAR inline char  g_chTrailLineDisp  = '~';

GLOBAL_VAR inline bool  g_fAllowBeep        = true ;
GLOBAL_VAR inline bool  g_fAskExit          = false;
GLOBAL_VAR inline bool  g_fBoxMode          = true ;
GLOBAL_VAR inline bool  g_fBrightFg         = false;
GLOBAL_VAR inline bool  g_fCase             = false;
GLOBAL_VAR inline bool  g_fDialogTop        = true ;
GLOBAL_VAR inline bool  g_fEditReadonly     = false;
GLOBAL_VAR inline bool  g_fErrPrompt        = true ;
GLOBAL_VAR inline bool  g_fFastsearch       = true ;
GLOBAL_VAR inline bool  g_fForcePlatformEol = false;
GLOBAL_VAR inline bool  g_fLangHilites      = true ;
GLOBAL_VAR inline bool  g_fLogFlush         = false;
GLOBAL_VAR inline bool  g_fLogcmds          = false;
GLOBAL_VAR inline bool  g_fM4backtickquote  = false;
GLOBAL_VAR inline bool  g_fMfgrepNoise      = false;
GLOBAL_VAR inline bool  g_fMfgrepRunning    = false; STATIC_FXN bool show_noise() { return !g_fMfgrepRunning || g_fMfgrepNoise; }
GLOBAL_VAR inline bool  g_fMsgflush         = false;
GLOBAL_VAR inline bool  g_fPcreAlways       = false;
GLOBAL_VAR inline bool  g_fRealtabs         = false;
GLOBAL_VAR inline bool  g_fReplaceCase      = false;
GLOBAL_VAR inline bool  g_fSelKeymapEnabled = false;
GLOBAL_VAR inline bool  g_fShowFbufDetails  = false;
GLOBAL_VAR inline bool  g_fShowMemUseInK    = true ;
GLOBAL_VAR inline bool  g_fSoftCr           = true ;
GLOBAL_VAR inline bool  g_fTabAlign         = false;
GLOBAL_VAR inline bool  g_fTrailLineWrite   = false;
GLOBAL_VAR inline bool  g_fTrailSpace       = false;
#if MOUSE_SUPPORT
GLOBAL_VAR inline bool  g_fUseMouse         = false;
#endif
GLOBAL_VAR inline bool  g_fViewOnly         = false;
GLOBAL_VAR inline bool  g_fWordwrap         = false;
GLOBAL_VAR inline bool  g_fWcShowDotDir     = false;


extern int   g_iBackupMode       ;
extern int   g_iBlankAnnoDispSrcMask;
#if !defined(_WIN32)
extern int   g_iConin_nonblk_rd_tmout;
#endif
extern int   g_iLuaGcStep        ;
extern int   g_swiWBCidx         ;

GLOBAL_VAR inline int   g_iCursorSize        =      -1;
GLOBAL_VAR inline int   g_iWucMinLen         =       2;
GLOBAL_VAR inline int   g_iTabWidth          =       8;
GLOBAL_VAR inline int   g_iHike              =      25;
GLOBAL_VAR inline int   g_iHscroll           =      10;
GLOBAL_VAR inline int   g_iRmargin           =      80;
GLOBAL_VAR inline int   g_iMaxUndo           =  100000;
GLOBAL_VAR inline int   g_iVscroll           =       1;

//############################  const  ############################
//############################  const  ############################
//############################  const  ############################

extern const Point g_PtInvalid;    // a Point that will never match a Point validly located within a file (FBUF, View)

extern const int  g_MaxKeyNameLen;

GLOBAL_CONST inline char kszAsgnFile    [] = "<CMD-SWI-Keys>";
GLOBAL_CONST inline char kszAssignLog   [] = "<a!>";
GLOBAL_CONST inline char kszBackup      [] = "backup";  // SWI name referred to in >1 locn (typically, has special input or output handling)
GLOBAL_CONST inline char kszBakDirNm    [] = ".kbackup";
GLOBAL_CONST inline char kszClipboard   [] = "<clipboard>";
GLOBAL_CONST inline char kszCompile     [] = "<compile>";
GLOBAL_CONST inline char kszCompileHdr  [] = "+^-^+";
GLOBAL_CONST inline char kszConsole     [] = "<console>";
GLOBAL_CONST inline char kszCwdStk      [] = "<cwd>";
GLOBAL_CONST inline char kszEnvFile     [] = "<env_>";
GLOBAL_CONST inline char kszFiles       [] = "<files>";
GLOBAL_CONST inline char kszMacDefs     [] = "<macdefs>";
GLOBAL_CONST inline char kszMasterRepo  [] = "https://github.com/fwmechanic/k_edit.git";
GLOBAL_CONST inline char kszMyEnvFile   [] = "<env>";
GLOBAL_CONST inline char kszNoFile      [] = "*";
GLOBAL_CONST inline char kszRecord      [] = "<record>";
GLOBAL_CONST inline char kszSearchLog   [] = "<search-keys>";
GLOBAL_CONST inline char kszSearchRslts [] = "<search-results>";
GLOBAL_CONST inline char kszUsgFile     [] = "<usage>";


extern "C" const char kszDtTmOfBuild[];
