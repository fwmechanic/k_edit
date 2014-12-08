//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

//
//  Switch definition table defintions
//
typedef bool  (*TPfxBool) (PCChar);
typedef PCChar(*TPfxStr)  (PCChar);

#define Var2TPfx( var ) {TPfxBool( &var )}
#define Fxn2TPfx( fxn ) {TPfxBool(  fxn )}

#define   oColor( ofs ) {static_cast<TPfxBool>(  ofs )}

union USwiAct {             // switch location or routine
   TPfxBool pFunc;          // - routine for text
   TPfxStr  pFunc2;         // - routine for text
   U8      *bval;           // - byte value for NUMERIC
   int     *ival;           // - integer value for NUMERIC
   bool    *fval;           // - flag value for BOOLEAN
   };


struct SWI; STD_TYPEDEFS( SWI )

typedef bool (*pfxDefnswi)( PCSWI pSwi, PCChar pszNewValue );
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
GLOBAL_CONST char kszTabconv   [] = "tabconv";

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
   const auto len( Min( sizeofDest - 1, size_t( Strlen(src) )) );
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

bool swixDelims( PCChar param ) {
   SafeStrcpy( g_delims, param );
   xlatStr( BSOB(g_delims      ), g_delims, delimNorm   );
   xlatStr( BSOB(g_delimMirrors), g_delims, delimMirror );
   return true;
   }

//--------------------------------------------------------------

GLOBAL_VAR char g_szWordChars[257];
GLOBAL_VAR bool s_isWordChar_[256];

STATIC_CONST char dfltWordChars[] = "_0123456789abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ";

void swidWordchars( PChar dest, size_t sizeofDest, void *src ) {
   const PCChar p0( dest );
   for( auto ix(1) ; ix < 256 && (dest - p0 - 1) < sizeofDest ; ++ix )
      if( s_isWordChar_[ix] && !strchr( dfltWordChars, ix ) )
         *dest++ = ix;
   *dest = '\0';
   }

bool swixWordchars( PCChar pS ) { 0&&DBG("%s+ %s", __func__, pS );
   if( 0==Stricmp( "nonwhite", pS ) ) {
      memset( s_isWordChar_, true, sizeof s_isWordChar_ );
      s_isWordChar_[0]    = false;
      s_isWordChar_[' ']  = false;
      s_isWordChar_[unsigned(HTAB)] = false;
      SafeStrcpy( g_szWordChars, "nonwhite" );
      }
   else {
      memset( s_isWordChar_, 0, sizeof s_isWordChar_ );
      for(          auto pC(dfltWordChars); *pC ; ++pC )  s_isWordChar_[ UI(*pC) ] = true;
      if( pS ) for( auto pC(pS           ); *pC ; ++pC )  s_isWordChar_[ UI(*pC) ] = true;
      auto pWc( g_szWordChars );
      for( auto ix(1) ; ix < 256 ; ++ix )
         if( s_isWordChar_[ix] )
            *pWc++ = ix;

      *pWc = '\0';
      }
   0&&DBG( "%s-", __func__ );
   return true;
   }

PCChar StrPastWord( PCChar pszToSearch ) {
   for(; *pszToSearch && isWordChar(*pszToSearch) ; ++pszToSearch )
      ;
   return pszToSearch;
   }

PCChar StrPastWord( PCChar pszToSearch, PCChar eos ) {
   for(; pszToSearch < eos && isWordChar(*pszToSearch) ; ++pszToSearch )
      ;
   return pszToSearch;
   }

PCChar StrWordStart( PCChar bos, PCChar ps ) {
   for(; isWordChar(*ps) ; --ps )
      if( ps <= bos )
         return bos;

   return ps+1;
   }


boost::string_ref::size_type IdxLastWordCh( boost::string_ref src, boost::string_ref::size_type start ) {
   if( start >= src.length() ) return boost::string_ref::npos;
   for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
      if( !isWordChar(*it) )  { return std::distance( src.cbegin(), it-1 ); }
      }
   return src.length() - 1;
   }

boost::string_ref::size_type IdxFirstWordCh( boost::string_ref src, boost::string_ref::size_type start ) {
   if( start >= src.length() ) return boost::string_ref::npos;
   for( auto it( src.crbegin() + (src.length() - start - 1) ); it != src.crend() ; ++it ) { 0 && DBG("%c", *it );
      if( !isWordChar(*it) )  { return src.length() - std::distance( src.crbegin(), it ); }
      }
   return 0;
   }

//--------------------------------------------------------------

GLOBAL_VAR Linebuf SwiErrBuf; // shared buffer used to format err msg strings returned by swix functions

#define EXT_SWID(nm)  extern void swid##nm( PChar dest, size_t sizeofDest, void *src )

#define EXT_SWI_FX_BOOL(nm)  extern bool   swix##nm ( PCChar param );  EXT_SWID(nm);
#define EXT_SWI_FX_STR(nm)   extern PCChar swix##nm ( PCChar param );  EXT_SWID(nm);

EXT_SWI_FX_STR(  Cursorsize    )
EXT_SWI_FX_STR(  Backup        )
EXT_SWI_FX_STR(  Tabwidth      )
EXT_SWI_FX_STR(  Tabconv       )

EXT_SWI_FX_BOOL( Hscroll       )
EXT_SWI_FX_BOOL( Vscroll       )
EXT_SWI_FX_BOOL( Tabdisp       )
EXT_SWI_FX_BOOL( Traildisp     )
EXT_SWI_FX_BOOL( TrailLinedisp )

EXT_SWI_FX_BOOL( WordChars     )
EXT_SWI_FX_BOOL( Delims        )

EXT_SWID( Hscroll );
EXT_SWID( Vscroll );


GLOBAL_VAR U8 g_colorInfo      = 0x1e;
GLOBAL_VAR U8 g_colorStatus    = 0x1e;
GLOBAL_VAR U8 g_colorWndBorder = 0xa0;
                              // 0x6c;
GLOBAL_VAR U8 g_colorError     = 0x1e;
GLOBAL_VAR bool g_fBpEnabled;

extern int  g_swiWBCidx;

extern int  g_iLuaGcStep;
extern bool g_fDialogTop;
extern bool g_fInsertMode;
extern bool g_fLangHilites;
extern bool g_fM4backtickquote;

// swin(e)s

STATIC_FXN bool swinVAR_BOOL( PCSWI pSwi, PCChar pszNewValue ) {
   0 && DBG( "VAR_BOOL nm=%s, val=%s'", pSwi->name, pszNewValue );
   if(   (Stricmp( "no", pszNewValue ) == 0)
      || (Strcmp (  "0" , pszNewValue ) == 0)
     ) {
      *pSwi->act.fval = false;
      return true;
      }

   if(   (Stricmp( "yes", pszNewValue ) == 0)
      || (Strcmp (  "1"  , pszNewValue ) == 0)
     ) {
      *pSwi->act.fval = true;
      return true;
      }

   if(   (Stricmp( "invert", pszNewValue ) == 0)
      || (Strcmp (  "-" , pszNewValue ) == 0)
     ) {
      *pSwi->act.fval = !*pSwi->act.fval;
      return true;
      }

   return ErrorDialogBeepf( "Boolean switch '%s' needs 'yes', 'no', or 'invert' (0/1/-) value", pSwi->name );
   }

STATIC_FXN bool swinCOLOR( PCSWI pSwi, PCChar pszNewValue ) {
   const auto conv_base( 16 );
   const auto newVal( StrToInt_variable_base( pszNewValue, conv_base ) );
   if( newVal == -1 )
      return ErrorDialogBeepf( "Numeric switch '%s': bad value", pSwi->name );

   *pSwi->act.bval = newVal & 0xFF;

   DispNeedsRedrawTotal();  // if color is changed interactively or in a startup macro the change did not affect all lines w/o this change
   return true;
   }

STATIC_FXN bool swinVAR_INT( PCSWI pSwi, PCChar pszNewValue ) {
   const auto conv_base( 10 );
   const auto newVal( StrToInt_variable_base( pszNewValue, conv_base ) );
   if( newVal == -1 )
      return ErrorDialogBeepf( "Numeric switch: bad value" );

   *pSwi->act.ival = newVal;
   return true;
   }

bool swinWBC_INT( PCSWI pSwi, PCChar pszNewValue ) {
   const auto conv_base( 10 );
   const auto newVal( StrToInt_variable_base( pszNewValue, conv_base ) );
   if( newVal == -1 )
      return ErrorDialogBeepf( "Numeric switch: bad value" );

   extern int              Max_wbc_idx();
   const auto max_wbc_idx( Max_wbc_idx() );
   if( newVal < 0 || newVal > max_wbc_idx )
      return ErrorDialogBeepf( "Numeric switch: out of range [0..%d]", max_wbc_idx );

   *pSwi->act.ival = newVal;
   DispNeedsRedrawTotal();
   return true;
   }


STATIC_FXN bool swinFXN_BOOL( PCSWI pSwi, PCChar pszNewValue ) {
   if( pSwi->act.pFunc( pszNewValue ) )
      return true;

   return ErrorDialogBeepf( "%s: Invalid value '%s'", pSwi->name, pszNewValue );
   }

STATIC_FXN bool swinFXN_STR( PCSWI pSwi, PCChar pszNewValue ) {
   const auto msg( pSwi->act.pFunc2( pszNewValue ) );
   if( !msg )
      return true;

   return ErrorDialogBeepf( "%s", msg );
   }

//----------- SWIX's  extern decls put here so there's no doubt that these shouldn't be called except via s_SwiTable

void swidBool(      PChar dest, size_t sizeofDest, void *src )  { safeStrcpy(  dest, sizeofDest, (*static_cast<bool *>(src)) ? "yes" : "no" ); }
void swid_int(      PChar dest, size_t sizeofDest, int   val )  { safeSprintf( dest, sizeofDest, "%d", val ); }
void swidInt(       PChar dest, size_t sizeofDest, void *src )  { swid_int(    dest, sizeofDest, (*static_cast<int *>(src)) ); }
void swidColorvarx( PChar dest, size_t sizeofDest, void *src )  { safeSprintf( dest, sizeofDest, "%02X", *PU8(src) ); }
// void swidColorx(    PChar dest, size_t sizeofDest, void *src )  { safeSprintf( dest, sizeofDest, "%02X", g_CurView()->ColorIdx2Attr( int(src) ) ); }


#if    defined(_WIN32)
#define kszHelpPlatEoL "always save modified files to disk w/CRLF (\"DOS\") line endings"
#else
#define kszHelpPlatEoL "always save modified files to disk w/LF (\"Unix\") line endings"
#endif

STATIC_CONST SWI s_SwiTable[] = {
 { "askexit"        , Var2TPfx( g_fAskExit              ), swinVAR_BOOL, swidBool       _AHELP( "enable last-chance prompt before terminating the editor session" ) },
 {  kszBackup       , Fxn2TPfx( swixBackup              ), swinFXN_STR , swidBackup     _AHELP( "choices are 'undel', 'bak' or 'none'; see online help for details" ) },
 { "beep"           , Var2TPfx( g_fAllowBeep            ), swinVAR_BOOL, swidBool       _AHELP( "beeping allowed (yes) or not (no)" ) },
 { "boxmode"        , Var2TPfx( g_fBoxMode              ), swinVAR_BOOL, swidBool       _AHELP( "selects BOXARGs (yes) or STREAMARGs (no)" ) },
 { "bpen"           , Var2TPfx( g_fBpEnabled            ), swinVAR_BOOL, swidBool       _AHELP( "enables conditional breakpoints" ) },
 { "case"           , Var2TPfx( g_fCase                 ), swinVAR_BOOL, swidBool       _AHELP( "searches are case sensitive (yes) or insensitive (no)" ) },
 { "colorerr"       , Var2TPfx( g_colorError            ), swinCOLOR   , swidColorvarx  _AHELP( "the color of error messages" ) },
 { "colorinf"       , Var2TPfx( g_colorInfo             ), swinCOLOR   , swidColorvarx  _AHELP( "the color of informative text" ) },
 { "colorsta"       , Var2TPfx( g_colorStatus           ), swinCOLOR   , swidColorvarx  _AHELP( "the color of most status-bar information" ) },
 { "colorwbc"       , Var2TPfx( g_colorWndBorder        ), swinCOLOR   , swidColorvarx  _AHELP( "the color of window borders" ) },
 { "cursorsize"     , Fxn2TPfx( swixCursorsize          ), swinFXN_STR , swidCursorsize _AHELP( "0:small, 1:large" ) },
 { "delims"         , Fxn2TPfx( swixDelims              ), swinFXN_BOOL, swidDelims     _AHELP( "string containing delimiters" ) },
 { "dialogtop"      , Var2TPfx( g_fDialogTop            ), swinVAR_BOOL, swidBool       _AHELP( "dialog & status lines placed at top (yes) or bottom (no) of screen" ) },
#if defined(_WIN32)
 { "dvlogcmds"      , Var2TPfx( g_fDvlogcmds            ), swinVAR_BOOL, swidBool       _AHELP( "log non-cursor-movement cmds to DbgView using Windows' OutputDebugString()" ) },
#endif
 { "editreadonly"   , Var2TPfx( g_fEditReadonly         ), swinVAR_BOOL, swidBool       _AHELP( "allow (yes) or prevent (no) editing of files which are not writable on disk" ) },
 { "errprompt"      , Var2TPfx( g_fErrPrompt            ), swinVAR_BOOL, swidBool       _AHELP( "error message display pauses with \"Press any key...\" prompt" ) },
 { "fastsearch"     , Var2TPfx( g_fFastsearch           ), swinVAR_BOOL, swidBool       _AHELP( "use fast search algorithm (when key contains no spaces)" ) },
 { "forceplateol"   , Var2TPfx( g_fForcePlatformEol     ), swinVAR_BOOL, swidBool       _AHELP(  kszHelpPlatEoL ) },
 { "hike"           , Var2TPfx( g_iHike                 ), swinVAR_INT , swidInt        _AHELP( "the distance from the cursor to the top/bottom of the window if you move the cursor out of the window by more than the number of lines specified by vscroll, as percent of window size" ) },
 { "hscroll"        , {         swixHscroll             }, swinFXN_BOOL, swidHscroll    _AHELP( "the number of columns that the editor scrolls the text left or right when the you move the cursor out of the window" ) },
 { "insmode"        , Var2TPfx( g_fInsertMode           ), swinVAR_BOOL, swidBool       _AHELP( "'graphic' inserts (yes) or overwrites (no) characters" ) },
 { "langhilites"    , Var2TPfx( g_fLangHilites          ), swinVAR_BOOL, swidBool       _AHELP( "enable (yes) partial language-aware hilighting" ) },
 { "luagcstep"      , Var2TPfx( g_iLuaGcStep            ), swinVAR_INT , swidInt        _AHELP( "in the idle thread, if $luagcstep > 0 then lua_gc( L, LUA_GCSTEP, $luagcstep )" ) },
 { "m4backtickquote", Var2TPfx( g_fM4backtickquote      ), swinVAR_BOOL, swidBool       _AHELP( "spanning backtick quoting right ends with ' (yes) or ` (no)" ) },
 { "maxfilehist"    , Var2TPfx( g_iMaxfilehist          ), swinVAR_INT , swidInt        _AHELP( "the maximum number of files kept in the file history between sessions; 0=no limit" ) },
 { "maxundel"       , Var2TPfx( g_iMaxUndel             ), swinVAR_INT , swidInt        _AHELP( "The maximum number of backup copies of a given file saved by the editor when when the Backup switch is set to 'undel'" ) },
 { "maxundo"        , Var2TPfx( g_iMaxUndo              ), swinVAR_INT , swidInt        _AHELP( "maximum number of major undo-steps allowed before oldest undo-step is discarded" ) },
 { "memusgink"      , Var2TPfx( g_fShowMemUseInK        ), swinVAR_BOOL, swidBool       _AHELP( "Show memory usage message in Kbytes (yes) or Mbytes (no)" ) },
 { "msgflush"       , Var2TPfx( g_fMsgflush             ), swinVAR_BOOL, swidBool       _AHELP( "<compile> is flushed (yes) or retained (no) when a new job is started" ) },
 { "realtabs"       , Var2TPfx( g_fRealtabs             ), swinVAR_BOOL, swidBool       _AHELP( "see online help" ) },
 { "rmargin"        , Var2TPfx( g_iRmargin              ), swinVAR_INT , swidInt        _AHELP( "see online help" ) },
 { "showfbufdetails", Var2TPfx( g_fShowFbufDetails      ), swinVAR_BOOL, swidBool       _AHELP( "show FBUF status details in <winN> sysbufs" ) },
 { "softcr"         , Var2TPfx( g_fSoftCr               ), swinVAR_BOOL, swidBool       _AHELP( "see online help" ) },
 { "tabalign"       , Var2TPfx( g_fTabAlign             ), swinVAR_BOOL, swidBool       _AHELP( "constrains the positioning of the cursor within tab fields: yes: cursor stays at beginning of the tab field; no: cursor can be moved to any column in the tab field" ) },
 {  kszTabconv      , Fxn2TPfx( swixTabconv             ), swinFXN_STR , swidTabconv    _AHELP( "when lines are modified, convert spaces to tabs: 0=never, 1=only leading spaces, 2=only outside quotes, or 3=everywhere" ) },
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
 { "wbcidx"         , Var2TPfx( g_swiWBCidx             ), swinWBC_INT , swidInt        _AHELP( "select window border charset (0..7)" ) },
 { "wordchars"      , {         swixWordchars           }, swinFXN_BOOL, swidWordchars  _AHELP( "the set of valid word-component characters; [_a-zA-Z0-9] are always members" ) },
 { "wordwrap"       , Var2TPfx( g_fWordwrap             ), swinVAR_BOOL, swidBool       _AHELP( "the editor performs automatic word-wrapping as you type past rmargin" ) },
 { "wucminlen"      , Var2TPfx( g_iWucMinLen            ), swinVAR_INT , swidInt        _AHELP( "minimum length of a word for it to qualify for 'word under the cursor' status" ) },
 };


STATIC_FXN PCSWI FindSwitch( PCChar pszSwiName ) {
   for( const auto &Swi : s_SwiTable )
      if( Swi.NameMatch( pszSwiName ) )
         return &Swi;

   return nullptr;
   }

void FBufRead_Assign_Switches( PFBUF pFBuf ) {
   FBufRead_Assign_SubHd( pFBuf, "Switches", ELEMENTS(s_SwiTable) );

   Xbuf xb;
   for( const auto &Swi : s_SwiTable ) {
      linebuf lbuf; lbuf[0] = '\0';
      Swi.pfxDisp( BSOB(lbuf), PVoid(Swi.act.fval) );
   #if AHELPSTRINGS
      pFBuf->xFmtLastLine( &xb, "%-20s: %-*s # %s", Swi.name, g_MaxKeyNameLen, lbuf, Swi.kszHelp );
   #else
      pFBuf->xFmtLastLine( &xb, "%-20s: %s", Swi.name, lbuf );
   #endif//AHELPSTRINGS
      }
   }

bool SetSwitch( PCChar pszSwitchName, PCChar pszNewValue ) { 0 && DBG( "SetSwitch '%s' ", pszSwitchName );
   auto pSwi( FindSwitch( pszSwitchName ) );
   if( !pSwi )
      return Msg( "'%s' is not an editor switch", pszSwitchName );

   if( g_pFBufAssignLog )  g_pFBufAssignLog->FmtLastLine( "ASGN  %s='%s'", pszSwitchName, pszNewValue );

   if( !pszNewValue || !*pszNewValue )
      return Msg( "'%s': empty switch value", pszSwitchName );

   return pSwi->pfxDefn( pSwi, pszNewValue );
   }
