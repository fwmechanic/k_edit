//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

#ifndef INT_MAX
#error undefined INT_MAX
#endif

#ifndef PTRDIFF_MAX
#error undefined PTRDIFF_MAX
#endif

// so far (20140209) I haven't been able to figure out a way to annotate
// a field width or field precision printf format sub-string such that a value
// of type larger than int (e.g.  ptrdiff_t in x64 compile mode) can be passed.
//
// Thus pd2Int (basically a saturating downcast) was written

#if (PTRDIFF_MAX==INT_MAX && PTRDIFF_MIN==INT_MIN)
    // Windows 32-bit:
STIL int pd2Int( ptrdiff_t pd ) { return pd; }

#else
    // Windows x64:
STIL int pd2Int( ptrdiff_t pd ) {
   if( pd >= 0 ) return (pd <= INT_MAX) ? static_cast<int>(pd) : INT_MAX;
   else          return (pd >= INT_MIN) ? static_cast<int>(pd) : INT_MIN;
   }

#endif

typedef int (CDECL__ * pfx_strcmp )( const char *, const char * );
typedef int (CDECL__ * pfx_strncmp)( const char *, const char *, size_t );

//--------------------------------------------------------------------------------------------

class BoolOneShot // simple utility functor
   {
   bool first;

   public:

   BoolOneShot() : first(true) {}
   int operator() () { const bool rv( first ); first = false; return rv; }
   };

//--------------------------------------------------------------------------------------------

               enum ePseudoBufType { GREP_BUF, SEL_BUF, };
PFBUF    PseudoBuf( ePseudoBufType PseudoBufType, int fNew );

extern bool merge_grep_buf( PFBUF dest, PFBUF src );

extern int  uint_log_10( int lmax );

extern int  KeyStr_full( PPChar ppDestBuf, size_t *bufBytesLeft, int keyNum_word );

extern int  DbgPopf( PCChar fmt, ... ) ATTR_FORMAT(1, 2);

extern void PutULC( PCChar msg );

extern void FreeAllMacroDefs();


extern CmdData CmdDataFromNextKey();
extern CmdData CmdDataFromNextKey_Keystr( PChar pKeyStringBuffer, size_t pKeyStringBufferBytes );


extern PCChar kszProgramVersion();
extern PCChar kszExecutableFormat();

extern PChar Getenv( PCChar pStart, int len );
extern PChar GetenvStrdup( PCChar pszEnvName );

extern Linebuf SwiErrBuf; // shared(!!!) buffer used to format err msg strings returned by swix functions
extern  void   swid_int( PChar dest, size_t sizeofDest, int val );
extern  void   swid_ch(  PChar dest, size_t sizeofDest, char ch );

//----------- Arg and Selection

extern   void ClearArgAndSelection();
extern   void ExtendSelectionHilite( const Point &pt );
extern   bool GetSelectionLineColRange( LINE *yMin, LINE *yMax, COL *xMin, COL *xMax );

         // GetTextargString flags bits
         enum { gts_fKbInputOnly      = BIT(0)
              , gts_DfltResponse      = BIT(1)
              , gts_OnlyNewlAffirms   = BIT(2)
              };
extern   PCCMD GetTextargString( PXbuf xb, PCChar pszPrompt, int xCursor, PCCMD pCmd, int flags, bool *pfGotAnyInputFromKbd );

extern   void  FetchAndExecuteCMDs( bool fCatchExecutionHaltRequests=false );

extern   bool Confirm( PCChar pszPrompt, ... ) ATTR_FORMAT(1, 2);

    enum ConfirmResponse { crYES, crNO, crCANCEL };
extern   ConfirmResponse Confirm_wCancel( PCChar pszPrompt, ... ) ATTR_FORMAT(1, 2);
extern   int chGetCmdPromptResponse( PCChar szAllowedResponses, int chDfltInteractiveResponse, int chDfltMacroResponse, PCChar pszPrompt, ... ) ATTR_FORMAT(4, 5);


namespace Video {
   void Startup( bool fForceNewConsole );
   Point ParentConsoleWindowSize();
   Point GetMaxConsoleSize();

   void GetScreenSize(PPoint rv);
   bool SetScreenSizeOk( int yHeight, int xWidth );

   bool GetCursorState( Point *pt, bool *pfVisible );

   bool SetCursorLocnOk( LINE yLine, COL xCol );
   bool SetCursorSizeOk( bool fBigCursor );
   bool SetCursorVisibilityChanged( bool fVisible );

   COL  BufferWriteString( PCChar pszStringToDisp, COL StringLen, LINE yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs );
   void BufferFlushToScreen();

   #define VIDEO_API_SUPPORTED_SCROLL  0
   #if     VIDEO_API_SUPPORTED_SCROLL
   void Scroll( LINE ulc_yLine, COL ulc_xCol, LINE lrc_yLine, COL lrc_xCol, LINE deltaLine );
   #endif
   bool WriteToFileOk( FILE *ofh );
   bool SetConsolePalette( const unsigned palette[16] );
   }

// Display

extern   void  UnhideCursor();
extern   void  HideCursor();

// class CursorHider
//    {
//    public:
//    CursorHider()  { HideCursor()  ; }
//    ~CursorHider() { UnhideCursor(); }
//    };

extern PCChar  SetCursorShape( bool fBlockCursor );
extern void    Bell();
extern void    WaitForKey();
extern void    WaitForKey( int secondsToWait );
extern bool    KbHit();
extern void    FlushKeyQueuePrimeScreenRedraw();
extern void    ConsoleReleaseOnExit();

//--------------------------------------------------------------------------------------------

// display-refresh API

extern void CursorLocnOutsideView_Set_( LINE y, COL x, PCChar from );
#define  CursorLocnOutsideView_Set( y, x )  CursorLocnOutsideView_Set_( y, x, __func__ )
extern bool CursorLocnOutsideView_Get( Point *pt );
extern void CursorLocnOutsideView_Unset();

extern void DispNeedsRedrawCursorMoved();

class ViewCursorRestorer
   {
public:
   ~ViewCursorRestorer() { CursorLocnOutsideView_Unset(); }
   };

extern void DispNeedsRedrawStatLn_()               ;
extern void DispNeedsRedrawVerticalCursorHilite_() ;
extern void DispNeedsRedrawAllLinesCurWin_()       ;
extern void DispNeedsRedrawAllLinesAllWindows_()   ;
extern void DispNeedsRedrawCurWin_()               ;
extern void DispNeedsRedrawTotal_()                ;
extern void DispDoPendingRefreshes_()              ;
extern void DispDoPendingRefreshesIfNotInMacro_()  ;
extern void DispRefreshWholeScreenNow_()           ;

#define  TRACE_DISP_NEEDS  0
#if      TRACE_DISP_NEEDS

#define  DispNeedsRedrawStatLn()                ( DBG( "%s by %s L %d", "DispNeedsRedrawStatLn"              , __func__, __LINE__ ), DispNeedsRedrawStatLn_()               )
#define  DispNeedsRedrawVerticalCursorHilite()  ( DBG( "%s by %s L %d", "DispNeedsRedrawVerticalCursorHilite", __func__, __LINE__ ), DispNeedsRedrawVerticalCursorHilite_() )
#define  DispNeedsRedrawAllLinesCurWin()        ( DBG( "%s by %s L %d", "DispNeedsRedrawAllLinesCurWin"      , __func__, __LINE__ ), DispNeedsRedrawAllLinesCurWin_()       )
#define  DispNeedsRedrawAllLinesAllWindows()    ( DBG( "%s by %s L %d", "DispNeedsRedrawAllLinesAllWindows"  , __func__, __LINE__ ), DispNeedsRedrawAllLinesAllWindows_()   )
#define  DispNeedsRedrawCurWin()                ( DBG( "%s by %s L %d", "DispNeedsRedrawCurWin"              , __func__, __LINE__ ), DispNeedsRedrawCurWin_()               )
#define  DispNeedsRedrawTotal()                 ( DBG( "%s by %s L %d", "DispNeedsRedrawTotal"               , __func__, __LINE__ ), DispNeedsRedrawTotal_()                )
#define  DispDoPendingRefreshes()               (                                                                                    DispDoPendingRefreshes_()               )
#define  DispDoPendingRefreshesIfNotInMacro()   ( DBG( "%s by %s L %d", "DispDoPendingRefreshesIfNotInMacro" , __func__, __LINE__ ), DispDoPendingRefreshesIfNotInMacro_()   )
#define  DispRefreshWholeScreenNow()            ( DBG( "%s by %s L %d", "DispRefreshWholeScreenNow"          , __func__, __LINE__ ), DispRefreshWholeScreenNow_()            )

#else

STIL void DispNeedsRedrawStatLn()               { DispNeedsRedrawStatLn_()              ; }
STIL void DispNeedsRedrawVerticalCursorHilite() { DispNeedsRedrawVerticalCursorHilite_(); }
STIL void DispNeedsRedrawAllLinesCurWin()       { DispNeedsRedrawAllLinesCurWin_()      ; }
STIL void DispNeedsRedrawAllLinesAllWindows()   { DispNeedsRedrawAllLinesAllWindows_()  ; }
STIL void DispNeedsRedrawCurWin()               { DispNeedsRedrawCurWin_()              ; }
STIL void DispNeedsRedrawTotal()                { DispNeedsRedrawTotal_()               ; }
STIL void DispDoPendingRefreshes()              { DispDoPendingRefreshes_()             ; }
STIL void DispDoPendingRefreshesIfNotInMacro()  { DispDoPendingRefreshesIfNotInMacro_() ; }
STIL void DispRefreshWholeScreenNow()           { DispRefreshWholeScreenNow_()          ; }

#endif

#define DISP_LL_STATS  0
#if     DISP_LL_STATS

extern int DispCursorMoves();
extern int DispStatLnUpdates();
extern int DispScreenRedraws();

#endif//DISP_LL_STATS

//--------------------------------------------------------------------------------------------

// Display Driver

struct DisplayDriverApi
   {
   void  (*DisplayNoise)( PCChar buffer );
   void  (*DisplayNoiseBlank)();
   COL   (*VidWrStrColor      )( LINE yLine, COL xCol, PCChar pszStringToDisp, int StringLen, int colorAttribute, bool fPadWSpcsToEol );
   COL   (*VidWrStrColors     )( LINE yLine, COL xCol, PCChar pszStringToDisp, COL maxCharsToDisp, PCLineColors alc, bool fUserSeesNow );
   };

extern DisplayDriverApi ddi;
extern void ddi_console();

#define DisplayNoise( buffer )\
   (ddi.DisplayNoise( buffer ))
#define DisplayNoiseBlank()\
   (ddi.DisplayNoiseBlank())
#define VidWrStrColor( yLine, xCol, pszStringToDisp, StringLen, colorIndex, fPadWSpcsToEol )\
   (ddi.VidWrStrColor( yLine, xCol, pszStringToDisp, StringLen, colorIndex, fPadWSpcsToEol ))
#define VidWrStrColors( yLine, xCol, pszStringToDisp, maxCharsToDisp, alc, fUserSeesNow )\
   (ddi.VidWrStrColors( yLine, xCol, pszStringToDisp, maxCharsToDisp, alc, fUserSeesNow ))

extern int   DispRawDialogStr( PCChar lbuf );
extern int   VMsg( PCChar pszFormat, va_list val );
extern bool  Msg(  PCChar pszFormat, ... ) ATTR_FORMAT(1, 2);
extern void  MsgClr();
extern void  VErrorDialogBeepf( PCChar format, va_list args );
extern bool  ErrorDialogBeepf(  PCChar format, ... ) ATTR_FORMAT(1, 2);
extern int   CanResizeScreen();
extern bool  ResizeScreen( int newX, int newY );
extern void  EditorSetScreenSize( Point newSize );

extern bool  VideoSwitchModeToXY( int newWidth, int newHeight );
extern COL   VidWrStrColorFlush( LINE yLine, COL xCol, PCChar pszStringToDisp, size_t StringLen, int colorIndex, bool fPadWSpcsToEol );

class VideoFlusher
   {
   bool d_fWantToFlush;
   public:
   VideoFlusher( bool fWantToFlush_=true ) : d_fWantToFlush(fWantToFlush_) {}
   ~VideoFlusher();
   };

//------------ Hi-level file and view APIs

extern   bool  fChangeFile( PCChar pszName, bool fCwdSave=true );
extern   bool  fChangeFileIfOnlyOneFilespecInCurWcFileSwitchToIt( PCChar pbuf );


extern   void  KillTheCurrentView();
extern   bool  FbufKnown( PFBUF pFBuf );
extern  PFBUF  FindFBufByName( PCChar pName );

extern   void  CreateWindow0();
extern   void  InitNewWin( PCChar pC );
extern   void  SetWindow0();

enum { MIN_WIN_WIDTH = 10, MIN_WIN_HEIGHT = 5 };
extern   int   cmp_win( PCWin pw1, PCWin pw2 );

extern   PWin  SplitCurWnd( bool fSplitVertical, int ColumnOrLineToSplitAt );
extern   void  SetWindowSetValidView( int widx );
extern   void  SetWindowSetValidView_( PWin pWin );

extern   void  RefreshCheckAllWindowsFBufs();
extern   bool  SwitchToWinContainingPointOk( const Point &pt );

//------------ Assign

extern   bool  SetSwitch( PCChar pszSwitchName, PCChar pszNewValue );
extern   void  FBufRead_Assign_Switches( PFBUF pFBuf );
extern   void  FBufRead_Assign_Win32( PFBUF pFBuf );

extern   void  AssignLogTag( PCChar tag );
#define        AssignStrOk( str )   AssignStrOk_( str, __FUNCTION__ )
extern   bool  AssignStrOk_( PCChar pszStringToAssign, CPCChar __function__ );
extern   bool  AssignLineRangeHadError( PCChar title, PFBUF pFBuf, LINE yStart, LINE yEnd=-1, int *pAssignsDone=nullptr, Point *pErrorPt=nullptr );

extern   void  UnbindMacrosFromKeys();
extern   int   edkcFromKeyname( PCChar pszKeyStr );
extern   bool  FlushKeyQueueAnythingFlushed();

extern   bool  SetKeyOk( PCChar pszCmdName, PCChar pszKeyName );
extern   void  PAssignShowKeyAssignment( const CMD &Cmd, PFBUF pFBufToWrite );
extern   void  StrFromEdkc( PChar pKeyStringBuf, size_t pKeyStringBufBytes, int EdKC );
extern   void  StrFromCmd( PChar pKeyStringBuf, size_t pKeyStringBufBytes, const CMD &CmdToFind );
extern   void  EventCmdSupercede( PCMD pOldCmd, PCMD pNewCmd );
extern   int   ShowAllUnassignedKeys( PFBUF pFBuf );

extern  PCCMD  CmdFromKbdForExec();
extern  PCCMD  CmdFromKbdForInfo( PChar pKeyStringBuffer, size_t pKeyStringBufferBytes );

//------------ FileExtensionSettings

extern void InitFileExtensionSettings();
extern void Reread_FileExtensionSettings();
extern void CloseFileExtensionSettings();

//------------ CmdIdx

extern  void   CmdIdxInit();
extern  void   CmdIdxClose();

extern  PCMD   CmdIdxFindByName( PCChar name );
extern  void   CmdIdxAddLuaFunc( PCChar name, funcCmd pFxn, int argType  _AHELP( PCChar helpStr ) );
extern  void   CmdIdxAddMacro( PCChar name, PCChar macroDef );
extern  int    CmdIdxRmvCmdsByFunction( funcCmd pFxn );
extern  PCMD   CmdFromName( PCChar name );

typedef PRbNode PCmdIdxNd;

extern  PCmdIdxNd CmdIdxFirst()                ;
extern  PCmdIdxNd CmdIdxLast()                 ;
extern  PCmdIdxNd CmdIdxAddinFirst()           ;
extern  PCmdIdxNd CmdIdxAddinLast()            ;
extern  PCmdIdxNd CmdIdxNext(   PCmdIdxNd pNd );
extern  PCCMD     CmdIdxToPCMD( PCmdIdxNd pNd );

typedef void (*CmdVisit)( PCCMD pCmd, void *pCtxt );
extern  void WalkAllCMDs( void *pCtxt, CmdVisit visit );

extern  void cmdusage_updt();
//------------

extern   char  CharAsciiFromKybd();
extern   void  FlushKeyQueue();

extern  PChar  ArgTypeNames( PChar buf, size_t sizeofBuf, int argval );

// extern   PChar StringOfAllKeyNamesFnIsAssignedTo( PCMD pCmdToFind, PChar pDestBuf, size_t sizeofDest );
// extern   PChar NameToKeys( PCChar pszName, PChar pDestBuf, size_t sizeofDest );

namespace Interpreter
   {
   bool  Interpreting();
   bool  Interpreting_VariableMacro();

   enum  { AskUser = -1, UseDflt = 0 }; // special cases rtnd by chGetAnyMacroPromptResponse(), must not be [1..0xFF] (valid/useful ascii chars)
   int   chGetAnyMacroPromptResponse();

   enum macroRunFlags
      { insideDQuotedString   = BIT(0)
      , variableMacro         = BIT(1)
      , breakOutHere          = BIT(2)
      };

   bool  PushMacroStringOk( PCChar pszMacroString, int macroFlags );
   }

extern   bool  fExecute( PCChar strToExecute, bool fInternalExec=true );
extern   bool  fExecuteSafe( PCChar str );
extern   bool  PushVariableMacro( PCChar strToExecute );

extern   void  CleanupAnyExecutionHaltRequest(); // must NOT be declared _within_ Main, where it would get a DLLX attribute

//-------------------------------------------------------------------------------------------------
//
// Mark module (deprecated)
//
extern   void  MarkDefineAtCurPos( PCChar pszNewMarkName );
extern   bool  MarkGoto( PCChar pszMarkname );
extern   void  AdjMarksForInsertion( PCFBUF pFBufSrc,PFBUF pFBufDest,int xLeft,int yTop,int xRight,int yBottom,int xDest,int yDest );
extern   void  AdjustMarksForLineDeletion( PFBUF pFBuf,int xLeft,int yTop,int xRight,int yBottom );
extern   void  AdjMarksForBoxDeletion( PFBUF pFBuf, COL xLeft, LINE yTop, COL xRight, LINE yBottom );
extern   void  AdjustMarksForLineInsertion( LINE Line,int LineDelta,PFBUF pFBuf );
//
//-------------------------------------------------------------------------------------------------


//
// CMD_reader embeds a VWritePrompt() hook in GetNextCMD_ExpandAnyMacros for writing
// screen prompt if the keyboard is going to be read.  This (base) class defines
// a empty VWritePrompt(), so it better not read the keyboard!
//
// This is largely guaranteed by calling GetNextCMD_ExpandAnyMacros( true ), and
// will largely NOT be guaranteed by calling GetNextCMD_ExpandAnyMacros( false ),
// which is why, for clients of this (base) class, we only publicly export
// GetNextCMD(), which is a wrapper around GetNextCMD_ExpandAnyMacros( true )
//
class CMD_reader {
protected:

   bool d_fAnyInputFromKbd;

   virtual void  VWritePrompt();
   virtual void  VUnWritePrompt();
           PCCMD GetNextCMD_ExpandAnyMacros( bool fRtnOnMacroHalt );

public:

   CMD_reader() : d_fAnyInputFromKbd( false ) {}

   PCCMD GetNextCMD()               { return GetNextCMD_ExpandAnyMacros( true ); }
   bool  GotAnyInputFromKbd() const { return d_fAnyInputFromKbd; }
   };

//------------ Pseudofile readers (ONLY call from ReadPseudoFileOk system!)

typedef void (*FbufReaderFxn)( PFBUF, int );

extern   bool  ReadPseudoFileOk( PFBUF pFBuf );
extern   int   IsWFilesName( PCChar pszName );

extern   void  FBufRead_Assign_SubHd( PFBUF pFBuf, PCChar subhd, int count );

//------------ Pseudofile writers

extern   void  AddToTextargStack( PCChar str );
extern   void  AddToSearchLog( PCChar str );

//------------ Editor startup and shutdown

extern   void  WriteAllDirtyFBufs();
extern  PChar  StateFilename( PChar dest, size_t sizeofDest, PCChar ext );
extern   void  WriteStateFile();
extern   void  EditorExit( int processExitCode, bool fWriteStateFile );

//------------ rsrc file section processing

extern  PChar  RsrcFilename( PChar dest, size_t sizeofDest, PCChar ext );
extern   bool  LoadRsrcSectionFound( PCChar pszSectionName, PInt pAssignCountAccumulator );
extern  PChar  IsolateTagStr( PChar pszText );
extern   bool  LoadFileExtRsrcIniSection( PCChar pszSectionName );
extern PCChar  LastExtTagLoaded();

//------------ misc edit helpers

extern   COL   FirstNonBlankCh( PCChar str, int chars );
extern   bool  PutCharIntoCurfileAtCursor( int theChar, PXbuf pxb );

extern   void  SearchEnvDirListForFile( PXbuf dest, const PCChar pszSrc, bool fKeepNameWildcard=false );
extern   void  SearchEnvDirListForFile( PXbuf dest, bool fKeepNameWildcard=false );
extern   void  SearchEnvDirListForFile( std::string &st, bool fKeepNameWildcard=false );

extern  std::string CompletelyExpandFName_wEnvVars( PCChar pszSrc );

extern  FileStat GetFileStat( PCChar fname );

extern  bool   PutEnvOk( PCChar varName, PCChar varValue );
extern  bool   PutEnvOk( PCChar szNameEqualsVal );

extern  void   SetCwdChanged( PCChar newName );

extern  bool   MoveCursorToEofAllWindows( PFBUF pFBuf, bool fIncludeCurWindow=false );

extern  PCChar GetWordUnderPoint( PCFBUF pFBuf, Point *cursor, COL *len );

//------------------------------------------------------------------------------
//
#define Assert( expr )  if( expr ) {} else { AssertDialog_( __func__, __LINE__ ); /*SW_BP;*/ }
#define BadParamIf( rv , expr )  if( !expr ) {} else { AssertDialog_( __func__, __LINE__ ); return rv ; }
extern void AssertDialog_( PCChar function, int line );
#define GotHereDialog( pfDialogShown )  GotHereDialog_( pfDialogShown, __func__, __LINE__ )
extern void GotHereDialog_( bool *dialogShown, PCChar fn, int lnum );
extern void ShowWinViewCnt( PCChar tag );  // debug!
//
//------------------------------------------------------------------------------



//#####################  functionality implemented in  Lua  #####################
//#####################  functionality implemented in  Lua  #####################
//#####################  functionality implemented in  Lua  #####################

//
// Lua functions callable from C++:
//

extern size_t LuaHeapSize();

extern void LuaIdleGC();
extern void LuaClose();

namespace LuaCtxt_ALL {

   extern void call_EventHandler( PCChar eventName );

   }

namespace LuaCtxt_State {

   extern bool InitOk( PCChar filename );

   }

namespace LuaCtxt_Edit {

   extern bool InitOk( PCChar filename );  // constructor

   extern bool ExecutedURL( PCChar strToExecute );  // if string matches "http[s]?//:", start a browser on it and return true

   //
   // String transformation functions have standard signature:
   //
   // These can behave as either:
   //
   // src==0: single buffer (dest is input and output)
   // src!=0: double buffer
   //
   // there is no efficiency difference, since the Lua result has to be copied into
   // dest regardless.
   //
   extern bool  ExpandEnvVarsOk( PXbuf pxb );
   extern bool  ExpandEnvVarsOk( std::string &st );

   extern bool  from_C_lookup_glock( PXbuf pxb );
   extern void  LocnListInsertCursor();  // intf into Lua locn-list subsys

   extern bool  nextmsg_setbufnm     ( PCChar src );  // for mfgrep
   extern bool  nextmsg_newsection_ok( PCChar src );  // for mfgrep
   extern bool  ReadPseudoFileOk     ( PFBUF src, int *pRvBool );

   //###                 ###
   //###   DATA ACCESS   ###
   //###                 ###

   // table readers

   extern PChar Tbl2S0(  PChar dest, size_t sizeof_dest, PCChar tableDescr );  // if any errors, returns nullptr
   extern PChar Tbl2S(   PChar dest, size_t sizeof_dest, PCChar tableDescr, PCChar pszDflt ); // if any errors, return pszDflt or empty string
   extern PChar Tbl2DupS0( PCChar tableDescr, PCChar pszDflt=nullptr );             // returns dup of pszDflt or nullptr if any errors
   extern int   Tbl2Int( PCChar tableDescr, int dfltVal );                  // if any errors, returns dfltVal

   }


//#####################  functionality implemented in  Lua  #####################
//#####################  functionality implemented in  Lua  #####################
//#####################  functionality implemented in  Lua  #####################
