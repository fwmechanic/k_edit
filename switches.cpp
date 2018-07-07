//
// Copyright 2015-2017 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

// Disable a picky gcc-8 compiler warning
#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

//
//  Switch definition table defintions
//
typedef bool  (*TPfxBool) (stref);
typedef PCChar(*TPfxStr)  (stref);

#define Var2TPfx( var ) {TPfxBool( &var )}
#define Fxn2TPfx( fxn ) {TPfxBool(  fxn )}

#define   oColor( ofs ) {static_cast<TPfxBool>(  ofs )}

union USwiAct {             // switch location or routine
   TPfxBool pFunc;          // - routine for text
   TPfxStr  pFunc2;         // - routine for text
   uint8_t *bval;           // - byte value for NUMERIC
   int     *ival;           // - integer value for NUMERIC
   bool    *fval;           // - flag value for BOOLEAN
   };

struct SWI;

typedef bool (*pfxDefnswi)( const SWI *pSwi, stref newValue );
typedef void (*pfxDispswi)( PChar dest, size_t sizeofDest, void *src );

struct SWI {                 // switch definition entry
   PCChar     name;          // - pointer to name of switch
   USwiAct    act;           // - pointer to value or fcn
   pfxDefnswi pfxDefn;
   pfxDispswi pfxDisp;
   AHELP( PCChar kszHelp; )  // - pointer to name of switch
   bool NameMatch( PCChar str ) const { return Stricmp( str, name ) == 0; }
   };

GLOBAL_CONST char kszBackup    [] = "backup";

//--------------------------------------------------------------

STATIC_FXN int delimNorm( int ch ) {
   switch( ch ) {
      case ')'  : return '(';
      case '>'  : return '<';
      case chRSQ: return chLSQ;
      case '}'  : return '{';
      case TICK : return BACKTICK;
      default   : return ch;
      }
   }

STATIC_FXN int delimMirror( int ch ) {
   switch( ch ) {
      case '('  : return ')';
      case '<'  : return '>';
      case chLSQ: return chRSQ;
      case '{'  : return '}';
      case BACKTICK: return TICK;
      default : return ch;
      }
   }

typedef int (*chXlat)( int ch );

void xlatStr( PChar dest, size_t sizeofDest, PCChar src, chXlat fxn ) {
   const auto len( std::min( sizeofDest - 1, size_t( Strlen(src) )) );
   const auto end( src + len );
   while( src < end ) {
      *dest++ = fxn( *src++ );
      }
   *dest = '\0';
   }

GLOBAL_VAR linebuf g_delims, g_delimMirrors;

void swidDelims( PChar dest, size_t sizeofDest, void *src ) {
   safeSprintf( dest, sizeofDest, "%s -> %s", g_delims, g_delimMirrors );
   }

bool swixDelims( stref param ) {
   bcpy( g_delims, param );
   xlatStr(    BSOB(g_delims      ), g_delims, delimNorm   );
   xlatStr(    BSOB(g_delimMirrors), g_delims, delimMirror );
   return true;
   }

//--------------------------------------------------------------

GLOBAL_VAR CharMap g_WordChars;
GLOBAL_VAR CharMap g_HLJChars;

STATIC_CONST char s_dfltWordChars[] = "_0123456789abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ";

STATIC_FXN void swidCharMap( PChar dest, size_t sizeofDest, const CharMap &chMap ) {
   const PCChar p0( dest );
   for( auto ix(1) ; ix < ELEMENTS(chMap.is) && (dest - p0 - 1) < sizeofDest ; ++ix ) {
      if( chMap.is[ix] && !strchr( s_dfltWordChars, ix ) ) {
         *dest++ = ix;
         }
      }
   *dest = '\0';
   }

STATIC_FXN void swidWordchars  ( PChar dest, size_t sizeofDest, void *src ) { swidCharMap( dest, sizeofDest, g_WordChars ); }
STATIC_FXN void swidHLJoinchars( PChar dest, size_t sizeofDest, void *src ) { swidCharMap( dest, sizeofDest, g_HLJChars  ); }

STATIC_FXN bool swixSetChars( CharMap &chMap, stref pS ) { 0 && DBG("%s+ %" PR_BSR, __func__, BSR(pS) );
   if( 0==cmpi( "nonwhite", pS ) ) {
      for( auto &ch : chMap.is ) { ch = true; }
      chMap.is[            0 ] = false;
      chMap.is[unsigned(' ') ] = false;
      chMap.is[unsigned(HTAB)] = false;
      bcpy( chMap.disp, "nonwhite" );
      }
   else {
      for( auto &ch : chMap.is ) { ch = false; }
      for( auto  ch : s_dfltWordChars ) { chMap.is[ UI(ch) ] = true; }
      if( !pS.empty() ) for( auto pC(pS.cbegin()); pC != pS.cend() ; ++pC ) { chMap.is[ UI(*pC) ] = true; }
      auto pWc( chMap.disp );
      for( auto ix(1) ; ix < 256 ; ++ix ) {
         if( chMap.is[ix] ) {
            *pWc++ = ix;
            }
         }
      *pWc = '\0';
      }                                       0 && DBG( "%s-", __func__ );
   return true;
   }

STATIC_FXN bool swixWordchars( stref pS ) {   0 && DBG("%s+ %" PR_BSR, __func__, BSR(pS) );
   return swixSetChars( g_WordChars, pS );
   }

STATIC_FXN bool swixHLJoinchars( stref pS ) { 0 && DBG("%s+ %" PR_BSR, __func__, BSR(pS) );
   return swixSetChars( g_HLJChars, pS );
   }

int isWordChar( int ch ) {
   if( !g_WordChars.is['a'] ) {
      swixWordchars( "" );
      }
// return g_WordChars.is[                      ch ];
   return g_WordChars.is[static_cast<unsigned>(ch)];
   }

int isHJChar( int ch ) {
   if( !g_HLJChars.is['a'] ) {
      swixWordchars( "" );
      }
// return g_HLJChars.is[                      ch ];
   return g_HLJChars.is[static_cast<unsigned>(ch)];
   }

sridx FirstNonWordOrEnd( stref src, sridx start ) {
   return ToNextOrEnd( notWordChar, src, start );
   }

sridx IdxFirstHJCh( stref src, sridx start ) {
   if( start >= src.length() ) { return stref::npos; }
   for( auto it( src.crbegin() + (src.length() - start - 1) ); it != src.crend() ; ++it ) { 0 && DBG("%c", *it );
      if( !isWordChar(*it) && !isHJChar(*it) )  { return src.length() - std::distance( src.crbegin(), it ); }
      }
   return 0;
   }

sridx IdxFirstWordCh( stref src, sridx start ) {
   if( start >= src.length() ) { return stref::npos; }
   for( auto it( src.crbegin() + (src.length() - start - 1) ); it != src.crend() ; ++it ) { 0 && DBG("%c", *it );
      if( !isWordChar(*it) )  { return src.length() - std::distance( src.crbegin(), it ); }
      }
   return 0;
   }

sridx StrLastWordCh( stref src ) {
   for( auto it( src.cbegin() ) ; it != src.cend() ; ++it ) {
      if( !isWordChar(*it) )  { return std::distance( src.cbegin(), it ) - 1; }
      }
   return src.length() - 1;
   }

//--------------------------------------------------------------

GLOBAL_VAR Linebuf SwiErrBuf; // shared buffer used to format err msg strings returned by swix functions

GLOBAL_VAR uint8_t g_colorInfo      = 0x1e;
GLOBAL_VAR uint8_t g_colorStatus    = 0x1e;
GLOBAL_VAR uint8_t g_colorWndBorder = 0xa0;
                                   // 0x6c;
GLOBAL_VAR uint8_t g_colorError     = 0x1e;
GLOBAL_VAR bool g_fBpEnabled;

// swin(e)s

STATIC_FXN bool swinVAR_BOOL( const SWI *pSwi, stref newValue ) { 0 && DBG( "VAR_BOOL nm=%s, val=%" PR_BSR "'", pSwi->name, BSR(newValue) );
   if(   0==cmpi( "no", newValue ) || ("0" == newValue ) ) {
      *pSwi->act.fval = false;
      return true;
      }
   if(   0==cmpi( "yes", newValue ) || ("1" == newValue) ) {
      *pSwi->act.fval = true;
      return true;
      }
   if(   0==cmpi( "invert", newValue ) || ("-" == newValue) ) {
      *pSwi->act.fval = !*pSwi->act.fval;
      return true;
      }
   return ErrorDialogBeepf( "Boolean switch '%s' needs 'yes', 'no', or 'invert' (0/1/-) value", pSwi->name );
   }

STATIC_FXN bool swinCOLOR( const SWI *pSwi, stref newValue ) {
   const auto newVal( StrToInt_variable_base( newValue, 16 ) );
   if( newVal == -1 ) {
      return ErrorDialogBeepf( "Numeric switch '%s': bad value", pSwi->name );
      }
   *pSwi->act.bval = newVal & 0xFF;
   DispNeedsRedrawTotal();  // if color is changed interactively or in a startup macro the change did not affect all lines w/o this change
   return true;
   }

STATIC_FXN bool swinVAR_INT( const SWI *pSwi, stref newValue ) {
   const auto newVal( StrToInt_variable_base( newValue, 10 ) );
   if( newVal == -1 ) {
      return ErrorDialogBeepf( "Numeric switch: bad value %" PR_BSR "", BSR(newValue) );
      }
   *pSwi->act.ival = newVal;
   return true;
   }

#if VARIABLE_WINBORDER
bool swinWBC_INT( const SWI *pSwi, stref newValue ) {
   const auto newVal( StrToInt_variable_base( newValue, 10 ) );
   if( newVal == -1 ) {
      return ErrorDialogBeepf( "Numeric switch: bad value %" PR_BSR "", BSR(newValue) );
      }
   const auto max_wbc_idx( Max_wbc_idx() );
   if( newVal < 0 || newVal > max_wbc_idx ) {
      return ErrorDialogBeepf( "Numeric switch: out of range [0..%d]", max_wbc_idx );
      }
   *pSwi->act.ival = newVal;
   DispNeedsRedrawTotal();
   return true;
   }
#endif

STATIC_FXN bool swinFXN_BOOL( const SWI *pSwi, stref newValue ) {
   if( pSwi->act.pFunc( newValue ) ) {
      return true;
      }
   return ErrorDialogBeepf( "%s: Invalid value '%" PR_BSR "'", pSwi->name, BSR(newValue) );
   }

STATIC_FXN bool swinFXN_STR( const SWI *pSwi, stref newValue ) {
   const auto msg( pSwi->act.pFunc2( newValue ) );
   if( !msg ) {
      return true;
      }
   return ErrorDialogBeepf( "%s", msg );
   }

//----------- SWIX's  extern decls put here so there's no doubt that these shouldn't be called except via s_SwiTable

void swidBool(      PChar dest, size_t sizeofDest, void *src )  { scpy(  dest, sizeofDest, (*static_cast<bool *>(src)) ? "yes" : "no" ); }
void swid_int(      PChar dest, size_t sizeofDest, int   val )  { safeSprintf( dest, sizeofDest, "%d", val ); }
void swidInt(       PChar dest, size_t sizeofDest, void *src )  { swid_int(    dest, sizeofDest, (*static_cast<int *>(src)) ); }
void swidColorvarx( PChar dest, size_t sizeofDest, void *src )  { safeSprintf( dest, sizeofDest, "%02X", *static_cast<uint8_t *>(src) ); }
// void swidColorx(    PChar dest, size_t sizeofDest, void *src )  { safeSprintf( dest, sizeofDest, "%02X", g_CurView()->ColorIdx2Attr( int(src) ) ); }

#if    defined(_WIN32)
#define kszHelpPlatEoL "always save modified files to disk w/CRLF (\"DOS\") line endings"
#else
#define kszHelpPlatEoL "always save modified files to disk w/LF (\"Unix\") line endings"
#endif

STATIC_CONST SWI s_SwiTable[] = {
//  name                        act                        pfxDefn       pfxDisp        kszHelp
 { "askexit"        , Var2TPfx( g_fAskExit              ), swinVAR_BOOL, swidBool       _AHELP( "enable last-chance prompt before terminating the editor session" ) },
 {  kszBackup       , Fxn2TPfx( swixBackup              ), swinFXN_STR , swidBackup     _AHELP( "choices are 'undel', 'bak' or 'none'; see online help for details" ) },
 { "beep"           , Var2TPfx( g_fAllowBeep            ), swinVAR_BOOL, swidBool       _AHELP( "beeping allowed (yes) or not (no)" ) },
 { "blankdispmask"  , Var2TPfx( g_iBlankAnnoDispSrcMask ), swinVAR_INT , swidInt        _AHELP( "bitmask: tabdisp, traildisp honored only when 1 (dirty) | 2 (arg-selecting) | 4 (all files)" ) },
 { "boxmode"        , Var2TPfx( g_fBoxMode              ), swinVAR_BOOL, swidBool       _AHELP( "selects BOXARGs (yes) or STREAMARGs (no)" ) },
 { "bpen"           , Var2TPfx( g_fBpEnabled            ), swinVAR_BOOL, swidBool       _AHELP( "enables conditional breakpoints" ) },
 { "case"           , Var2TPfx( g_fCase                 ), swinVAR_BOOL, swidBool       _AHELP( "searches are case sensitive (yes) or insensitive (no)" ) },
 { "colorerr"       , Var2TPfx( g_colorError            ), swinCOLOR   , swidColorvarx  _AHELP( "the color of error messages" ) },
 { "colorinf"       , Var2TPfx( g_colorInfo             ), swinCOLOR   , swidColorvarx  _AHELP( "the color of informative text" ) },
 { "colorsta"       , Var2TPfx( g_colorStatus           ), swinCOLOR   , swidColorvarx  _AHELP( "the color of most status-bar information" ) },
 { "colorwbc"       , Var2TPfx( g_colorWndBorder        ), swinCOLOR   , swidColorvarx  _AHELP( "the color of window borders" ) },
#if !defined(_WIN32)
 { "conin_tmout"    , Var2TPfx( g_iConin_nonblk_rd_tmout), swinVAR_INT , swidInt        _AHELP( "value passed to ncurses::timeout( value ) when nonblocking ncurses::getch() is configured" ) },
#endif
 { "cursorsize"     , Fxn2TPfx( swixCursorsize          ), swinFXN_STR , swidCursorsize _AHELP( "0:small, 1:large" ) },
 { "delims"         , Fxn2TPfx( swixDelims              ), swinFXN_BOOL, swidDelims     _AHELP( "string containing delimiters" ) },
 { "dialogtop"      , Var2TPfx( g_fDialogTop            ), swinVAR_BOOL, swidBool       _AHELP( "dialog & status lines placed at top (yes) or bottom (no) of screen" ) },
#if defined(_WIN32)
 { "dvlogcmds"      , Var2TPfx( g_fDvlogcmds            ), swinVAR_BOOL, swidBool       _AHELP( "log non-cursor-movement cmds to DbgView using Windows' OutputDebugString()" ) },
#endif
 { "editreadonly"   , Var2TPfx( g_fEditReadonly         ), swinVAR_BOOL, swidBool       _AHELP( "allow (yes) or prevent (no) editing of files which are not writable on disk" ) },
 { "entab"          , Fxn2TPfx( swixEntab               ), swinFXN_STR , swidEntab      _AHELP( "when lines are modified, convert 0/none,1/leading,2/exoquote,3/all spaces to tabs" ) },
 { "errprompt"      , Var2TPfx( g_fErrPrompt            ), swinVAR_BOOL, swidBool       _AHELP( "error message display pauses with \"Press any key...\" prompt" ) },
 { "fastsearch"     , Var2TPfx( g_fFastsearch           ), swinVAR_BOOL, swidBool       _AHELP( "use fast search algorithm (when key contains no spaces)" ) },
 { "forceplateol"   , Var2TPfx( g_fForcePlatformEol     ), swinVAR_BOOL, swidBool       _AHELP(  kszHelpPlatEoL ) },
 { "ftype"          , Fxn2TPfx( swixFtype               ), swinFXN_STR , swidFtype      _AHELP( "set ftype" ) },
 { "hike"           , Var2TPfx( g_iHike                 ), swinVAR_INT , swidInt        _AHELP( "the distance from the cursor to the top/bottom of the window if you move the cursor out of the window by more than the number of lines specified by vscroll, as percent of window size" ) },
 { "hljoinchars"    , {         swixHLJoinchars         }, swinFXN_BOOL, swidHLJoinchars _AHELP( "Hierarchial Left Join charset: chars that, when seen to the left of the cursor, join other identifiers further left to the word under cursor for WUC highlighting purposes; [_a-zA-Z0-9] are always members" ) },
 { "hscroll"        , {         swixHscroll             }, swinFXN_BOOL, swidHscroll    _AHELP( "the number of columns that the editor scrolls the text left or right when you move the cursor out of the window" ) },
 { "langhilites"    , Var2TPfx( g_fLangHilites          ), swinVAR_BOOL, swidBool       _AHELP( "enable (yes) partial language-aware hilighting" ) },
 { "luagcstep"      , Var2TPfx( g_iLuaGcStep            ), swinVAR_INT , swidInt        _AHELP( "in the idle thread, if $luagcstep > 0 then lua_gc( L, LUA_GCSTEP, $luagcstep )" ) },
 { "m4backtickquote", Var2TPfx( g_fM4backtickquote      ), swinVAR_BOOL, swidBool       _AHELP( "spanning backtick quoting right ends with ' (yes) or ` (no)" ) },
 { "maxundo"        , Var2TPfx( g_iMaxUndo              ), swinVAR_INT , swidInt        _AHELP( "maximum number of major undo-steps allowed before oldest undo-step is discarded" ) },
 { "memusgink"      , Var2TPfx( g_fShowMemUseInK        ), swinVAR_BOOL, swidBool       _AHELP( "Show memory usage message in Kbytes (yes) or Mbytes (no)" ) },
 { "mfgrepnoise"    , Var2TPfx( g_fMfgrepNoise          ), swinVAR_BOOL, swidBool       _AHELP( "during mfgrep and mfreplace execution: display (yes) or hide (no) each filename & fio-phase display" ) },
 { "msgflush"       , Var2TPfx( g_fMsgflush             ), swinVAR_BOOL, swidBool       _AHELP( "<compile> is flushed (yes) or retained (no) when a new job is started" ) },
 { "realtabs"       , Var2TPfx( g_fRealtabs             ), swinVAR_BOOL, swidBool       _AHELP( "see online help" ) },
 { "replacecase"    , Var2TPfx( g_fReplaceCase          ), swinVAR_BOOL, swidBool       _AHELP( "replace operations are case sensitive (yes) or insensitive (no)" ) },
 { "rmargin"        , Var2TPfx( g_iRmargin              ), swinVAR_INT , swidInt        _AHELP( "see online help" ) },
 { "showfbufdetails", Var2TPfx( g_fShowFbufDetails      ), swinVAR_BOOL, swidBool       _AHELP( "show FBUF status details in <winN> sysbufs" ) },
 { "softcr"         , Var2TPfx( g_fSoftCr               ), swinVAR_BOOL, swidBool       _AHELP( "see online help" ) },
 { "tabalign"       , Var2TPfx( g_fTabAlign             ), swinVAR_BOOL, swidBool       _AHELP( "within tab fields, cursor can be positioned (yes) only on tab char (no) in any column" ) },
 { "tabdisp"        , {         swixTabdisp             }, swinFXN_BOOL, swidTabdisp    _AHELP( "the numeric ASCII code of the character used to display tab characters; if 0, the space character is used" ) },
 { "tabwidth"       , Fxn2TPfx( swixTabwidth            ), swinFXN_STR , swidTabwidth   _AHELP( "the width of a 'tab-column'; set PER FILE" ) },
 { "traildisp"      , {         swixTraildisp           }, swinFXN_BOOL, swidTraildisp  _AHELP( "the numeric ASCII code of the character used to display trailing spaces on a line; if 0, the space character is used" ) },
 { "traillinedisp"  , {         swixTrailLinedisp       }, swinFXN_BOOL, swidTrailLinedisp _AHELP( "the numeric ASCII code of the character used to display trailing lines at the end of file; if 0, the space character is used" ) },
 { "traillinewrite" , Var2TPfx( g_fTrailLineWrite       ), swinVAR_BOOL, swidBool       _AHELP( "write to disk file (yes) or discard (no) trailing empty lines" ) },
 { "trailspace"     , Var2TPfx( g_fTrailSpace           ), swinVAR_BOOL, swidBool       _AHELP( "preserve (yes) or remove (no) trailing spaces on lines that are modified" ) },
 #if MOUSE_SUPPORT
 { "usemouse"       , Var2TPfx( g_fUseMouse             ), swinVAR_BOOL, swidBool       _AHELP( "accept (yes) or ignore (no) mouse input" ) },
 #endif
 { "viewonly"       , Var2TPfx( g_fViewOnly             ), swinVAR_BOOL, swidBool       _AHELP( "files subsequently opened will be editable (no) or no-edit (yes)" ) },
 { "vscroll"        , {         swixVscroll             }, swinFXN_BOOL, swidVscroll    _AHELP( "the number of lines scrolled when the mlines and plines functions move the cursor out of the window" ) },
#if VARIABLE_WINBORDER
 { "wbcidx"         , Var2TPfx( g_swiWBCidx             ), swinWBC_INT , swidInt        _AHELP( "select window border charset (0..7)" ) },
#endif
 { "wordchars"      , {         swixWordchars           }, swinFXN_BOOL, swidWordchars  _AHELP( "the set of valid word-component characters; [_a-zA-Z0-9] are always members" ) },
 { "wordwrap"       , Var2TPfx( g_fWordwrap             ), swinVAR_BOOL, swidBool       _AHELP( "the editor performs automatic word-wrapping as you type past rmargin" ) },
 { "wucminlen"      , Var2TPfx( g_iWucMinLen            ), swinVAR_INT , swidInt        _AHELP( "minimum length of a word for it to qualify for 'word under the cursor' status" ) },
 { "wcshowdotdir"   , Var2TPfx( g_fWcShowDotDir         ), swinVAR_BOOL, swidBool       _AHELP( "in recursive wildcard buffers show (yes) or hide (NO) dir (subtrees) whose names start with '.'" ) },
 };

STATIC_FXN const SWI *FindSwitch( stref pszSwiName ) {
   for( const auto &Swi : s_SwiTable ) {
      if( 0==cmpi( Swi.name, pszSwiName ) ) {
         return &Swi;
         }
      }
   return nullptr;
   }

void FBufRead_Assign_Switches( PFBUF pFBuf ) {
   FBufRead_Assign_SubHd( pFBuf, "Switches", ELEMENTS(s_SwiTable) );
   Xbuf xb;
   for( const auto &Swi : s_SwiTable ) {
      linebuf lbuf; lbuf[0] = '\0';
      Swi.pfxDisp( BSOB(lbuf), PVoid(Swi.act.fval) );
      pFBuf->xFmtLastLine( &xb, "%-20s: %-*s # %s", Swi.name, g_MaxKeyNameLen, lbuf, Swi.kszHelp );
      }
   }

bool SetSwitch( stref switchName, stref newValue ) {
   trim( switchName ); trim( newValue );  0 && DBG( "SetSwitch '%" PR_BSR "' ", BSR(switchName) );
   auto pSwi( FindSwitch( switchName ) ); if( !pSwi ) { return Msg( "'%" PR_BSR "' is not an editor switch", BSR(switchName) ); }
   if( g_pFBufAssignLog ) { g_pFBufAssignLog->FmtLastLine( "ASGN  %" PR_BSR "='%" PR_BSR "'", BSR(switchName), BSR(newValue) ); }
   if( newValue.empty() ) { return Msg( "%" PR_BSR ": empty switch value", BSR(switchName) ); }
   return pSwi->pfxDefn( pSwi, newValue );
   }
