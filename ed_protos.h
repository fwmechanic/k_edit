//
// Copyright 2015-2022 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#ifndef INT_MAX
#error undefined INT_MAX
#endif

#ifndef PTRDIFF_MAX
#error undefined PTRDIFF_MAX
#endif

//------------------------------------------------------------------------------
extern void AssertDialog_( PCChar function, PCChar file, int line );
extern void GotHereDialog_( bool *dialogShown, PCChar file, PCChar fn, int lnum );
#define Assert( expr )  if( expr ) {} else { AssertDialog_( __func__, __FILE__, __LINE__ ); /*SW_BP;*/ }
#define BadParamIf( rv , expr )  if( !expr ) {} else { AssertDialog_( __func__, __FILE__, __LINE__ ); return rv ; }
#define GotHereDialog( pfDialogShown )  GotHereDialog_( pfDialogShown, __func__, __FILE__, __LINE__ )
//------------------------------------------------------------------------------

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
   if( pd >= 0 ) { return (pd <= INT_MAX) ? static_cast<int>(pd) : INT_MAX; }
   else          { return (pd >= INT_MIN) ? static_cast<int>(pd) : INT_MIN; }
   }
#endif

extern unsigned uint_log_10( unsigned lmax );

enum class ePseudoBufType { GREP, SEL, };
extern PFBUF  PseudoBuf( ePseudoBufType PseudoBufType, bool fNew );

extern bool merge_grep_buf( PFBUF dest, PFBUF src );

struct EditorFilesStatus_t {
   size_t dirtyFBufs = 0;
   size_t openFBufs  = 0;
   bool operator==( const EditorFilesStatus_t &rhs ) const { return dirtyFBufs == rhs.dirtyFBufs && openFBufs == rhs.openFBufs; }
   bool operator!=( const EditorFilesStatus_t &rhs ) const { return !(*this == rhs); }
   };
extern EditorFilesStatus_t EditorFilesStatus();
STIL bool EditorFilesystemNoneDirty() { return EditorFilesStatus().dirtyFBufs == 0; }

extern PCChar ProgramVersion();
extern PCChar ExecutableFormat();
extern stref  CompilerVersion();
extern stref  BoostVersion();

//----------- Arg and Selection

extern   bool IsSelectionActive();
extern   int  Get_g_ArgCount();
STIL     bool IsMacroRecordingActive()    { extern bool g_fMacroRecordingActive; return g_fMacroRecordingActive; }
STIL     bool IsCmdXeqInhibitedByRecord() { extern bool g_fCmdXeqInhibitedByRecord; return g_fCmdXeqInhibitedByRecord; }

extern   void ClearArgAndSelection();
extern   void ExtendSelectionHilite( const Point &pt );
extern   std::tuple<bool,LINE,LINE,COL,COL> CursorFuncPeekSeln();
         struct TCursorFuncPeekSeln { bool selnActive; LINE yMin, yMax; COL xMin, xMax; };
extern   TCursorFuncPeekSeln CursorFuncPeekSelnS();

extern   void TermNulleow( std::string &st );

         // GetTextargString flags bits
         enum { gts_fKbInputOnly      = BIT(0)
              , gts_DfltResponse      = BIT(1)
              , gts_OnlyNewlAffirms   = BIT(2)
              };
extern   PCCMD GetTextargString( std::string &xb, PCChar pszPrompt, int xCursor, PCCMD pCmd, int flags, bool *pfGotAnyInputFromKbd );

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
   virtual void  VWritePrompt()   {}
   virtual void  VUnWritePrompt() {}
           enum class eOnMacHalt { Continue, Return };
           PCCMD GetNextCMD_ExpandAnyMacros( eOnMacHalt fRtnOnMacroHalt );
public:
   CMD_reader() : d_fAnyInputFromKbd( false ) {}
   PCCMD GetNextCMD()               { return GetNextCMD_ExpandAnyMacros( eOnMacHalt::Return ); }
   bool  GotAnyInputFromKbd() const { return d_fAnyInputFromKbd; }
   virtual ~CMD_reader() {}
   };

extern   void  FetchAndExecuteCMDs( bool fCatchExecutionHaltRequests );

// Display

extern   void  UnhideCursor();
extern   void  HideCursor();

// class CursorHider
//    {
//    public:
//    CursorHider()  { HideCursor()  ; }
//    ~CursorHider() { UnhideCursor(); }
//    };

extern void    WaitForKey( int secondsToWait );
extern void    FlushKeyQueuePrimeScreenRedraw();

//--------------------------------------------------------------------------------------------
// display-refresh API

extern void CursorLocnOutsideView_Set_( LINE y, COL x, PCChar from );
#define     CursorLocnOutsideView_Set( y, x )  CursorLocnOutsideView_Set_( y, x, __func__ )
extern bool CursorLocnOutsideView_Get( Point *pt );
extern void CursorLocnOutsideView_Unset();

extern void DispNeedsRedrawCursorMoved();

class ViewCursorRestorer {
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

extern void Display_hilite_regex_err( PCChar errMsg, PCChar pszSearchStr, int errOffset );

#define  TRACE_DISP_NEEDS  0
#if      TRACE_DISP_NEEDS
#define  DispNeedsRedrawStatLn()                ( DBG( "%s by %s L %d", "DispNeedsRedrawStatLn"              , __func__, __LINE__ ), DispNeedsRedrawStatLn_()               )
#define  DispNeedsRedrawVerticalCursorHilite()  ( DBG( "%s by %s L %d", "DispNeedsRedrawVerticalCursorHilite", __func__, __LINE__ ), DispNeedsRedrawVerticalCursorHilite_() )
#define  DispNeedsRedrawAllLinesCurWin()        ( DBG( "%s by %s L %d", "DispNeedsRedrawAllLinesCurWin"      , __func__, __LINE__ ), DispNeedsRedrawAllLinesCurWin_()       )
#define  DispNeedsRedrawAllLinesAllWindows()    ( DBG( "%s by %s L %d", "DispNeedsRedrawAllLinesAllWindows"  , __func__, __LINE__ ), DispNeedsRedrawAllLinesAllWindows_()   )
#define  DispNeedsRedrawCurWin()                ( DBG( "%s by %s L %d", "DispNeedsRedrawCurWin"              , __func__, __LINE__ ), DispNeedsRedrawCurWin_()               )
#define  DispNeedsRedrawTotal()                 ( DBG( "%s by %s L %d", "DispNeedsRedrawTotal"               , __func__, __LINE__ ), DispNeedsRedrawTotal_()                )
#define  DispDoPendingRefreshes()               (                                                                                    DispDoPendingRefreshes_()              )
#define  DispDoPendingRefreshesIfNotInMacro()   ( DBG( "%s by %s L %d", "DispDoPendingRefreshesIfNotInMacro" , __func__, __LINE__ ), DispDoPendingRefreshesIfNotInMacro_()  )
#define  DispRefreshWholeScreenNow()            ( DBG( "%s by %s L %d", "DispRefreshWholeScreenNow"          , __func__, __LINE__ ), DispRefreshWholeScreenNow_()           )
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

enum class eFlush { noFlush, doFlush };
enum class ePad { noPad, padWSpcsToEol };
struct DisplayDriverApi {
   void  (*DisplayNoise)( PCChar buffer );
   void  (*DisplayNoiseBlank)();
   COL   (*VidWrStrColor    )( LINE yLine, COL xCol, PCChar pszStringToDisp, int StringLen, colorval_t colorAttribute, ePad pad );
   COL   (*VidWrStrColors   )( LINE yLine, COL xCol, PCChar pszStringToDisp, COL maxCharsToDisp, const LineColorvals * alc, eFlush doFlush );
   };

extern DisplayDriverApi g_DDI;
extern bool ConIO_InitOK( bool fForceNewConsole );
extern void ConIO_Shutdown();

#define DisplayNoise( buffer )   (g_DDI.DisplayNoise( buffer ))
#define DisplayNoiseBlank()      (g_DDI.DisplayNoiseBlank())
#define VidWrStrColor(  yLine, xCol, pszStringToDisp, StringLen, colorIndex, pad )\
 (g_DDI.VidWrStrColor(  yLine, xCol, pszStringToDisp, StringLen, colorIndex, pad ))
#define VidWrStrColors( yLine, xCol, pszStringToDisp, maxCharsToDisp, alc, fFlushNow )\
 (g_DDI.VidWrStrColors( yLine, xCol, pszStringToDisp, maxCharsToDisp, alc, fFlushNow ))

class ColoredStref_ {
   const ePad    d_pad;
   const uint8_t d_color;
   const stref   d_sr;
public:
   ColoredStref_( uint8_t color_, stref sr_, ePad pad_=ePad::noPad ) : d_pad(pad_), d_color(color_), d_sr(sr_) {}
   COL VidWr( LINE yLine, COL xCol ) const {
      VidWrStrColor( yLine, xCol, d_sr.data(), d_sr.length(), d_color, d_pad );
      return d_sr.length();
      }
   };
typedef std::vector<ColoredStref_> ColoredStrefs;
extern COL VidWrColoredStrefs( LINE yLine, COL xCol, const ColoredStrefs &csrs, eFlush fFlushNow=eFlush::doFlush );

extern   int chGetCmdPromptResponse( PCChar szAllowedResponses, int chDfltInteractiveResponse, int chDfltMacroResponse, PCChar pszPrompt, ... ) ATTR_FORMAT(4, 5);
extern   int chGetCmdPromptResponse( PCChar szAllowedResponses, int chDfltInteractiveResponse, int chDfltMacroResponse, const ColoredStrefs &csrs );

extern int   DispRawDialogStr( PCChar lbuf );
extern int   VMsg( PCChar pszFormat, va_list val );
extern bool  Msg(  PCChar pszFormat, ... ) ATTR_FORMAT(1, 2);
extern void  MsgClr();
extern void  VErrorDialogBeepf( PCChar format, va_list args );
extern bool  ErrorDialogBeepf(  PCChar format, ... ) ATTR_FORMAT(1, 2);

extern void  Event_ScreenSizeChanged( const Point &newSize );

struct hl_rgn_t {
   const ColorTblIdx color;
   const Rect rgn;
   hl_rgn_t( ColorTblIdx color_, LINE yulc_, COL xulc_, LINE ylrc_, COL xlrc_ ) : color( color_ ), rgn( yulc_, xulc_, ylrc_, xlrc_ ) {}
   };

// if newSize is not supported, and a supported size can be switched to:
//    it will be switched to, newSize will be updated, "OK" status will be returned
extern bool  VideoSwitchModeToXYOk( Point &newSize );

extern COL   VidWrStrColorFlush(       LINE yLine, COL xCol, PCChar pszStringToDisp, size_t StringLen, colorval_t attr, ePad pad );
extern void  DirectVidWrStrColorFlush( LINE yLine, COL xCol, stref sr, colorval_t attr );
extern void  DirectVidClear();

class VideoFlusher {
   bool d_fWantToFlush;
public:
   VideoFlusher( bool fWantToFlush_=true ) : d_fWantToFlush(fWantToFlush_) {}
   ~VideoFlusher();
   };

STIL   COL       ScreenCols()    { extern COL  s_iWidth ; return s_iWidth    ; }
STIL   COL   EditScreenCols()    {                        return ScreenCols(); }

STIL   LINE      ScreenLines()   { extern LINE s_iHeight; return s_iHeight  ; } // RARELY USED (so far, only in WriteStateFile(), ScreenSizeChanger)
STIL   LINE  EditScreenLines()   { extern LINE s_iHeight; return s_iHeight-2; }

extern LINE  DialogLine();
extern LINE  StatusLine();
extern LINE  MinDispLine();
extern int   Max_wbc_idx();

//------------ Hi-level file and view APIs

extern   bool  fChangeFile( PCChar pszName, bool fCwdSave=true );
extern   bool  fChangeFileIfOnlyOneFilespecInCurWcFileSwitchToIt( PCChar pbuf );

extern   void  KillTheCurrentView();
extern   bool  FbufKnown( PFBUF pFBuf );
extern  PFBUF  FindFBufByName( stref name );

//------------ Window APIs

extern   void  CreateWindow0();
extern   void  InitNewWin( PCChar pC );
extern   void  SetWindow0();

constexpr int MIN_WIN_WIDTH = 10, MIN_WIN_HEIGHT = 10;
extern   int   cmp_win( PCWin pw1, PCWin pw2 );

extern   PWin  SplitCurWnd( bool fSplitVertical, int columnOrLineToSplitAt );
extern   void  SetWindowSetValidView( int widx );
extern   void  SetWindowSetValidView_( PWin pWin );

extern   void  RefreshCheckAllWindowsFBufs();
extern   bool  SwitchToWinContainingPointOk( const Point &pt );

extern   bool  Wins_CanResizeContent( const Point &newSize );
extern   void  Wins_ScreenSizeChanged( const Point &newSize );
extern   void  Wins_WriteStateFile( FILE *ofh );

//------------ Assign

extern   void  SwitblInit();
extern   bool  SetSwitch( stref pszSwitchName, stref pszNewValue );

extern void  SetCurDelims( stref param );

extern   void  AssignLogTag( PCChar tag );
constexpr char RSRCFILE_COMMENT_DELIM = '#';
#define        AssignStrOk( str )   AssignStrOk_( str, __FUNCTION__ )
extern   bool  AssignStrOk_( stref src, CPCChar __function__ );
#define        TruncComment_AssignStrOk( str )  TruncComment_AssignStrOk_( str, __FUNCTION__ )
extern   bool  TruncComment_AssignStrOk_( stref src, CPCChar caller );
extern   bool  DefineMacro( stref pszMacroName, stref pszMacroCode );
extern   void  FreeAllMacroDefs();
enum class SetKeyRV { SetKeyRV_OK, SetKeyRV_BADKEY, SetKeyRV_BADCMD };
extern   SetKeyRV BindKeyToCMD( stref cmdName, stref keyName );

extern   void  UnbindMacrosFromKeys();
extern   void  AssignShowKeyAssignment( const CMD &Cmd, PFBUF pFBufToWrite, std::string &tmp1, std::string &tmp2 );
extern   void  AssignSubstituteCmd( PCMD pOldCmd, PCMD pNewCmd );
extern   int   ShowAllUnassignedKeys( PFBUF pFBuf );
extern   std::string  KeyNmAssignedToCmd_all( PCCMD pCmdToFind, PCChar nmSep );
extern   std::string  KeyNmAssignedToCmd_first( const CMD &CmdToFind );

extern   int          EdkcOfKeyNm( stref keyNm );
extern   std::string  KeyNmOfEdkc(                    int edKC );
extern   std::string &KeyNmOfEdkc( std::string &dest, int edKC );

extern  EdKc_t ModalKeyRemap( EdKc_t edKC );
extern  PCCMD  CmdFromKbdForExec();
extern  PCCMD  CmdFromKbdForInfo( std::string &dest );
extern   char  CharAsciiFromKybd();

extern  std::string ArgTypeNames( int argval );

extern  void WordCharSet_push();
extern  bool WordCharSet_pop() ;

//------------ Macro execution

namespace Interpreter {
   bool  Interpreting();

   enum  { AskUser = -1, UseDflt = 0 }; // special cases rtnd by chGetAnyMacroPromptResponse(), must not be [1..0xFF] (valid/useful ascii chars)
   int   chGetAnyMacroPromptResponse();

   enum  macroRunFlags { variableMacro=BIT(0), breakOutHere=BIT(1) };
   bool  PushMacroStringOk       ( PCChar pszMacroString, int macroRunFlags );
   bool  PushMacroStringLiteralOk( PCChar pszMacroString, int macroRunFlags );
   }

extern   bool  fExecute( PCChar strToExecute, bool fInternalExec=true );
extern   bool  fExecuteSafe( PCChar str );
extern   bool  PushVariableMacro( PCChar strToExecute );
extern   std::string DupTextMacroValue( PCChar macroName );
extern   stref ExtractAssignableText( stref src, bool &continues );
extern   void  CleanupAnyExecutionHaltRequest(); // must NOT be declared _within_ Main, where it would get a DLLX attribute

//------------ FTypeSettings

extern  void   InitFTypeSettings();
extern  void   Reread_FTypeSettings();
extern  void   CloseFTypeSettings();

//------------ CmdIdx

extern  void   CmdIdxInit();
extern  void   CmdIdxClose();

extern  void   CmdIdxAddLuaFunc( PCChar name, funcCmd pFxn, int argType  _AHELP( PCChar helpStr ) );
extern  void   CmdIdxAddMacro( stref name, stref macroDef );
extern  int    CmdIdxRmvCmdsByFunction( funcCmd pFxn );
extern  PCMD   CmdFromName( stref name );

typedef const RbNode *PCmdIdxNd;
extern  PCmdIdxNd CmdIdxAddinFirst()           ;
extern  PCmdIdxNd CmdIdxAddinNil()             ;
extern  PCmdIdxNd CmdIdxNext(   PCmdIdxNd pNd );
extern  PCCMD  CmdIdxToPCMD( PCmdIdxNd pNd );

typedef void   (*CmdVisit)( PCCMD pCmd, void *pCtxt );
extern  void   WalkAllCMDs( void *pCtxt, CmdVisit visit );

extern  void   cmdusage_updt();

//------------ Mark module (deprecated)

extern   void  MarkDefineAtCurPos( PCChar pszNewMarkName );
extern   bool  MarkGoto( PCChar pszMarkname );
extern   void  AdjMarksForInsertion( PCFBUF pFBufSrc,PFBUF pFBufDest,int xLeft,int yTop,int xRight,int yBottom,int xDest,int yDest );
extern   void  AdjustMarksForLineDeletion( PFBUF pFBuf,int xLeft,int yTop,int xRight,int yBottom );
extern   void  AdjMarksForBoxDeletion( PFBUF pFBuf, COL xLeft, LINE yTop, COL xRight, LINE yBottom );
extern   void  AdjustMarksForLineInsertion( LINE Line,int LineDelta,PFBUF pFBuf );

//------------ Pseudofile readers (ONLY call from ReadPseudoFileOk system!)

extern   bool  ReadPseudoFileOk( PFBUF pFBuf );
extern   void  FBufRead_Assign_intrinsicCmds( PFBUF pFBuf, std::string &tmp1, std::string &tmp2 );
extern   void  FBufRead_Assign_OsInfo( PFBUF pFBuf );
extern   void  FBufRead_Assign_SubHd( PFBUF pFBuf, PCChar subhd, int count );
extern   void  FBufRead_Assign_Switches( PFBUF pFBuf );

//------------ Pseudofile writers

extern   void  AddToTextargStack( stref str );
extern   void  AddToSearchLog   ( stref str );

//------------ tempfile management

// wrappers
//
#define     unlinkOk( filename )  unlinkOk_( (filename), __func__ )
extern bool unlinkOk_( PCChar filename, PCChar caller );

class tempfile {
   std::string d_name;
   FILE       *d_fh;
public:
   tempfile( PCChar mode );
   FILE * fh() { return d_fh; }  // d_name fopened in "wx" mode (or nullptr if closed)
   PCChar name() { return d_name.c_str(); }
   bool   close() {
      if( d_fh ) {
         fclose( d_fh );
         d_fh = nullptr;
         return true;
         }
      return false;
      }
   bool   unlink() {
      if( !d_name.empty() ) {
         close();
         unlinkOk( d_name.c_str() );
         d_name.clear();
         return true;
         }
      return false;
      }
   ~tempfile() { unlink(); }
   };

//------------ Editor startup and shutdown

extern   void  WriteAllDirtyFBufs();
extern Path::str_t StateFilename( PCChar ext );
extern PCChar  EditorStateDir();
extern   void  WriteStateFile();
extern   void  EditorExit( int processExitCode, bool fWriteStateFile );
extern   void  IdleIntegrityCheck();

//------------ rsrc file section processing

extern Path::str_t RsrcFilename( PCChar ext );
extern bool    RsrcFileLdAllNamedSections( stref pszSectionName, int *pAssignCountAccumulator=nullptr );
extern bool    RsrcFileLineRangeAssignFailed( PCChar title, PFBUF pFBuf, LINE yStart, LINE yEnd=-1, int *pAssignsDone=nullptr, Point *pErrorPt=nullptr );
extern PCChar  LastRsrcFileLdSectionFtypeNm();
extern PCChar  LastRsrcFileLdSectionFtypeSectionNm();
extern int     ReinitializeMacros( bool fEraseExistingMacros );

//------------ misc edit helpers

extern  bool   PutCharIntoCurfileAtCursor( char theChar, std::string &tmp1, std::string &tmp2 );
extern  void   SearchEnvDirListForFile( Path::str_t &st, bool fKeepNameWildcard=false );
extern  Path::str_t CompletelyExpandFName_wEnvVars( PCChar pszSrc );
extern  FileStat GetFileStat( PCChar fname );
extern  bool   mkdir_failed( PCChar dirname );

// call around FBUF::PutLastLine() calls to effect (smart) cursor-tailing
extern  void   CapturePrevLineCountAllWindows( PFBUF pFBuf, bool fIncludeCurWindow=false );
extern  bool   MoveCursorToEofAllWindows     ( PFBUF pFBuf, bool fIncludeCurWindow=false );

extern  stref  GetWordUnderPoint( PCFBUF pFBuf, Point *cursor );

// the distinction between the following two is small...
extern  void   Clipboard_PutText( stref sr );
extern  void   Clipboard_PutText_Multiline( stref sr );
extern  void   WinClipGetFirstLine( std::string &xb );

//#####################  functionality implemented in  Lua  #####################
//#####################  functionality implemented in  Lua  #####################
//#####################  functionality implemented in  Lua  #####################

//
// Lua functions callable from C++:
//
extern bool    Lua_ConfirmYes( PCChar prompt );

extern size_t  LuaHeapSize();

extern void    LuaIdleGC();
extern void    LuaClose();

namespace LuaCtxt_ALL {
   extern void call_EventHandler( PCChar eventName );
   }

namespace LuaCtxt_State {
   extern bool InitOk( PCChar filename );
   }

namespace LuaCtxt_Edit {
   extern bool InitOk( PCChar filename );  // constructor
   extern bool ExecutedURL( stref strToExecute, bool fNullarg );  // if string matches "http[s]?//:", start a browser on it and return true
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
   extern bool  ExpandEnvVarsOk( Path::str_t &st );

   extern bool  from_C_lookup_glock( std::string &st );
   extern bool  FnmToFType         ( Path::str_t &st );
   extern bool  ShebangToFType     ( Path::str_t &st );
   extern void  LocnListInsertCursor();  // intf into Lua locn-list subsys

   extern bool  nextmsg_setbufnm     ( PCChar src );  // for mfgrep
   extern bool  nextmsg_newsection_ok( PCChar src );  // for mfgrep
   extern bool  ReadPseudoFileOk     ( PFBUF src );
   extern bool  RegisterAllLuaEdFxns ();

   //###                 ###
   //###   DATA ACCESS   ###
   //###                 ###

   // table readers
   extern bool  TblKeyExists( PCChar tableDescr );
   extern PChar Tbl2S(   PChar dest, size_t sizeof_dest, PCChar tableDescr, PCChar pszDflt ); // if any errors, return pszDflt or empty string
   extern int   Tbl2Int( PCChar tableDescr, int dfltVal );                  // if any errors, returns dfltVal
   }

//#####################  functionality implemented in  Lua  #####################
//#####################  functionality implemented in  Lua  #####################
//#####################  functionality implemented in  Lua  #####################
