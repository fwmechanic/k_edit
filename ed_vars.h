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

//############  MISC VARIABLES

extern ARG   noargNoMeta;

extern bool     g_fFuncRetVal           ;
extern bool     g_fMeta                 ;
extern bool     g_CLI_fUseRsrcFile      ;

extern int      g_fExecutingInternal    ;
extern int      g_ClipboardType         ;

extern char ** &g_envp                  ;

extern linebuf  g_delims, g_delimMirrors;

extern std::string  g_SnR_stSearch      ;
extern std::string  g_SnR_stReplacement ;

extern CMD         g_CmdTable[];  // this is ALMOST const; can't BE const because CMD contains (mutatable) fields d_argData, d_callCount & d_gCallCount

//############  global PFBUF's

extern PFBUF g_pFBufAssignLog    ;
extern PFBUF g_pFBufCmdlineFiles ;
extern PFBUF g_pFBufConsole      ;
extern PFBUF g_pFBufCwd          ;
extern PFBUF g_pFBufSearchLog    ;
extern PFBUF g_pFBufSearchRslts  ;
extern PFBUF g_pFBufTextargStack ;
extern PFBUF g_pFbufClipboard    ;
extern PFBUF g_pFbufRecord       ;
extern PFBUF s_pFbufLog          ;
extern PFBUF s_pFbufLuaLog       ;

//############  SWITCH VALUE VARIABLES

#ifdef _WIN32
enum { BIG_BULLET = 249, SMALL_BULLET = 250 };
#else
enum { BIG_BULLET = '^', SMALL_BULLET = '`' };
#endif

extern char  g_chTabDisp         ;
extern char  g_chTrailLineDisp   ;
extern char  g_chTrailSpaceDisp  ;

extern bool  g_fAllowBeep        ;
extern bool  g_fAskExit          ;
extern bool  g_fBoxMode          ;
extern bool  g_fCase             ;
extern bool  g_fDialogTop        ;
extern bool  g_fDvlogcmds        ;
extern bool  g_fEditReadonly     ;
extern bool  g_fErrPrompt        ;
extern bool  g_fFastsearch       ;
extern bool  g_fForcePlatformEol ;
extern bool  g_fLangHilites      ;
extern bool  g_fM4backtickquote  ;
extern bool  g_fMsgflush         ;
extern bool  g_fRealtabs         ;
extern bool  g_fReplaceCase      ;
extern bool  g_fShowFbufDetails  ;
extern bool  g_fShowMemUseInK    ;
extern bool  g_fSoftCr           ;
extern bool  g_fTabAlign         ;
extern bool  g_fTrailLineWrite   ;
extern bool  g_fTrailSpace       ;
#if MOUSE_SUPPORT
extern bool  g_fUseMouse         ;
#endif
extern bool  g_fViewOnly         ;
extern bool  g_fWordwrap         ;

extern int   g_iBackupMode       ;
extern int   g_iBlankAnnoDispSrcMask;
#if !defined(_WIN32)
extern int   g_iConin_nonblk_rd_tmout;
#endif
extern int   g_iCursorSize       ;
extern int   g_iHike             ;
extern int   g_iHscroll          ;
extern int   g_iLuaGcStep        ;
extern int   g_iMaxUndo          ;
extern int   g_iRmargin          ;
extern int   g_iTabWidth         ;
extern int   g_iVscroll          ;
extern int   g_iWucMinLen        ;
extern int   g_swiWBCidx         ;

//############################  const  ############################
//############################  const  ############################
//############################  const  ############################

extern const int  g_MaxKeyNameLen;

extern const char kszAsgnFile[];   // = "<CMD-SWI-Keys>";
extern const char kszAssignLog[];  // =  "<a!>"
extern const char kszBackup  [];   // "backup";    SWI name referred to in >1 locn (typically, has special input or output handling)
extern const char kszBakDirNm[];   // = ".kbackup";
extern const char kszClipboard[];  // = "<clipboard>";
extern const char kszCompileHdr[]; // = "+^-^+";
extern const char kszCompile[];    // = "<compile>"
extern const char kszConsole[];    // = "<console>";
extern const char kszCwdStk [];    // = "<cwd>";
extern const char kszEnvFile[];    // = "<env_>";
extern const char kszFiles[];      // = "<files>";
extern const char kszMacDefs[];    // = "<macdefs>";
extern const char kszMyEnvFile[];  // = "<env>";
extern const char kszNoFile [];    // = "*";
extern const char kszRecord[];     // = "<record>";
extern const char kszSearchLog[];  // = "<search-keys>"
extern const char kszSearchRslts[];// = "<search-results>"
extern const char kszStkname[];    // = "<stack>";
extern const char kszUsgFile[];    // = "<usage>";

extern "C" const char kszDtTmOfBuild[];
