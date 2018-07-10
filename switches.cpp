//
// Copyright 2015-2018 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

class SWI_intf {
 public:
   SWI_intf() {}
   virtual std::string defn( stref newValue ) = 0; // { return "not implemented!"; }
   virtual std::string disp()                 = 0; // { return "not implemented!"; }
   virtual ~SWI_intf() {}

   // toolbox:
   static std::string dispBool( bool val ) { return val ? "yes" : "   no"; }
   static std::string defnBool( bool &fChanged, bool &val, stref newValue ) {
      const auto oldVal( val );
      if(   0==cmpi( "no", newValue ) || ("0" == newValue ) ) {
         val = false;
         fChanged = val!=oldVal;
         return "";
         }
      if(   0==cmpi( "yes", newValue ) || ("1" == newValue) ) {
         val = true;
         fChanged = val!=oldVal;
         return "";
         }
      if(   0==cmpi( "invert", newValue ) || ("-" == newValue) ) {
         val = !val;
         fChanged = val!=oldVal;
         return "";
         }
      return FmtStr<200>( "Boolean switch needs 'yes', 'no', or 'invert' (0/1/-) value, not '%" PR_BSR "'", BSR(newValue) ).k_str();
      }

   static std::string dispInt( int val ) { return FmtStr<20>( "%d", val ).k_str(); }
   static std::string defnInt( stref newValue, bool &fChanged, int &val, int min=INT_MIN, int max=INT_MAX, bool fUseConstrained=true ) {
      const auto newVal( StrToInt_variable_base( newValue, 10 ) );
      if( newVal == -1 ) {
         return FmtStr<200>( "could not convert '%" PR_BSR "' to int", BSR(newValue) ).k_str();
         }
      auto constrVal( newVal );
      Constrain( min, &constrVal, max );
      if( !fUseConstrained && constrVal != newVal ) {
         return FmtStr<50>( "%" PR_BSR " (%d) not within [%d..%d]", BSR(newValue), newVal, min, max ).k_str();
         }
      fChanged = constrVal != val;
      if( fChanged ) {
         val = constrVal;
         }
      return "";
      }

   };

class SWIs : public SWI_intf {
   stref (* const d_get)();
   stref (* const d_set)( stref );
 public:
   SWIs( stref (* get_)(), stref (* set_)( stref ) )
      : d_get(get_)
      , d_set(set_)
      {}
   std::string defn( stref newValue ) override { return sr2st( d_set( newValue ) ); }
   std::string disp() override { return sr2st( d_get() ); }
   };

class SWIsb : public SWI_intf {
   void  (* const d_dsp)( PChar dest, size_t sizeofDest );
   void  (* const d_set)( stref );
 public:
   SWIsb( void (*dsp_)( PChar dest, size_t sizeofDest ), void (* set_)( stref ) )
      : d_dsp(dsp_)
      , d_set(set_)
      {}
   std::string defn( stref newValue ) override { d_set( newValue ); return ""; }
   std::string disp() override {
      linebuf lbuf; lbuf[0] = '\0';
      d_dsp( BSOB(lbuf) );
      return lbuf;
      }
   };

class SWIi_bv : public SWI_intf {
   bool &d_var;
 public:
   SWIi_bv( bool &var_ ) : SWI_intf(), d_var(var_) {}
   SWIi_bv(SWIi_bv&& mE) = default;
   std::string defn( stref newValue ) override {
      bool fChanged;
      const auto rv( defnBool( fChanged, d_var, newValue ) );
      return rv;
      }
   std::string disp() override {
      return dispBool( d_var );
      }
   };

class SWIi_iv : public SWI_intf {
   int &d_var;
 public:
   SWIi_iv( int &var_ ) : d_var(var_) {}
   SWIi_iv(SWIi_iv&& mE) = default;
   std::string defn( stref newValue ) override {
      bool fChanged;
      const auto rv( defnInt( newValue, fChanged, d_var ) );
      return rv;
      }
   std::string disp() override {
      return dispInt( d_var );
      }
   };

class SWIi_ci : public SWI_intf {
 protected:
   int   (* const d_get)();
   void  (* const d_set)(int);
   int   (* const d_min)();
   int   (* const d_max)();
   const bool d_fUseConstrained;
 public:
   SWIi_ci( int (*get_)(), void (*set_)(int), int (*min_)(), int (*max_)(), bool fUseConstrained_=true )
      : d_get(get_)
      , d_set(set_)
      , d_min(min_)
      , d_max(max_)
      , d_fUseConstrained(fUseConstrained_)
      {}
   SWIi_ci(SWIi_ci&& mE) = default;
   std::string defn( stref newValue ) override {
      bool fChanged;
      int val = d_get();
      const auto rv( defnInt( newValue, fChanged, val, d_min(), d_max(), d_fUseConstrained ) );
      if( fChanged ) {
         d_set( val );
         }
      return rv;
      }
   std::string disp() override {
      return dispInt( d_get() );
      }
   };

class SWI_color : public SWI_intf {
   uint8_t &d_var;
 public:
   SWI_color( uint8_t &var_ ) : d_var(var_) {}
   std::string defn( stref newValue ) override {
      const auto newVal( StrToInt_variable_base( newValue, 16 ) );
      if( newVal == -1  ) {
         return FmtStr<200>( "could not convert %" PR_BSR "", BSR(newValue) ).k_str();
         }
      if( newVal > 0xFF ) {
         return FmtStr<200>( "bad value 0x%X", newVal ).k_str();
         }
      if( d_var != newVal ) {
         d_var = newVal;
         DispNeedsRedrawTotal();  // if color is changed interactively or in a startup macro the change did not affect all lines w/o this change
         }
      return "";
      }
   std::string disp() override {
      return FmtStr<20>( "%02X", d_var ).k_str();
      }
   };

class SWI_chdisp : public SWI_intf {
   char &d_var;
 public:
   SWI_chdisp( char &var_ ) : d_var(var_) {}
   std::string defn( stref newValue ) override {
      d_var = char(StrToInt_variable_base( newValue, 10 ));
      if( char(-1) == d_var ) {
         d_var = newValue.length() == 1 ? newValue[0] : ' ';
         }
      DispNeedsRedrawAllLinesAllWindows();
      return "";
      }
   std::string disp() override {
      return FmtStr<20>( "0x%02X (%c)", d_var, d_var ).k_str();
      }
   };

class SWI_intf_base {
   const stref  d_name;
   AHELP( const stref d_help; )
   std::unique_ptr<SWI_intf> d_intf;
 public:
   enum { DB=0 };
   SWI_intf_base( PCChar name_, SWI_intf * intf_ _AHELP( PCChar help_ ) ) : d_name( name_ ) _AHELP( d_help( help_ ) ), d_intf(intf_) {}
   SWI_intf_base(const SWI_intf_base&  mE) = default;
   SWI_intf_base(      SWI_intf_base&& mE) = default;
   SWI_intf_base& operator=(const SWI_intf_base&  mE) = default;
   SWI_intf_base& operator=(      SWI_intf_base&& mE) = default;
   ~SWI_intf_base() {}
          stref name() const { return d_name; }
   AHELP( stref help() const { return d_help; } )
   bool NameMatch( stref str ) const { return cmpi( str, d_name ) == 0; }
   std::string defn( stref newValue ) { return d_intf->defn( newValue ); }
   std::string disp()                 { return d_intf->disp()          ; }
   };

struct enum_nm { int val; PCChar name; };
class SWI_enum : public SWI_intf {
   int   (* const d_get)();
   void  (* const d_set)(int);
   const enum_nm *d_enums;
   const size_t   d_num_enums;
   std::string    d_str_allowed_names;
 public:
   SWI_enum( int (*get_)(), void (*set_)(int), const enum_nm *enums_, size_t num_enums_ )
      : d_get(get_)
      , d_set(set_)
      , d_enums(enums_)
      , d_num_enums(num_enums_)
      {
      d_str_allowed_names = "{ ";
      for( auto ix(0) ; ix < d_num_enums ; ++ix ) {
         d_str_allowed_names += d_enums[ix].name;
         d_str_allowed_names += ", ";
         }
      if( d_str_allowed_names.length() > 2 ) {
         d_str_allowed_names[d_str_allowed_names.length()-2] = ' ';
         d_str_allowed_names[d_str_allowed_names.length()-1] = '}';
         }
      }
   std::string defn( stref newValue ) override {
      for( auto ix(0) ; ix < d_num_enums ; ++ix ) {
         if( 0==cmpi( newValue, d_enums[ix].name ) ) {
            if( d_get() != d_enums[ix].val ) {
               d_set( d_enums[ix].val );
               }
            return "";
            }
         }
      return FmtStr<200>( "value %" PR_BSR " not in %" PR_BSR "", BSR(newValue), BSR(d_str_allowed_names) ).k_str();
      }
   std::string disp() override {
      const auto val( d_get() );
      for( auto ix(0) ; ix < d_num_enums ; ++ix ) {
         if( 0==cmpi( val, d_enums[ix].val ) ) {
            return d_enums[ix].name;
            }
         }
      return "?";
      }
   };

typedef std::vector< SWI_intf_base > SWI_vector;
SWI_vector s_switbl;
static void addswi( PCChar name_, SWI_intf *intf_ _AHELP( PCChar help_ ) ) {
   s_switbl.emplace_back( name_, intf_ _AHELP( help_ ) );
   }

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

void swidDelims( PChar dest, size_t sizeofDest ) {
   safeSprintf( dest, sizeofDest, "%s -> %s", g_delims, g_delimMirrors );
   }

void SetCurDelims( stref param ) {
   bcpy( g_delims, param );
   xlatStr(    BSOB(g_delims      ), g_delims, delimNorm   );
   xlatStr(    BSOB(g_delimMirrors), g_delims, delimMirror );
   }

//--------------------------------------------------------------

GLOBAL_VAR CharMap g_WordChars;
GLOBAL_VAR CharMap g_HLJChars;

STATIC_CONST char s_dfltWordChars[] = "_0123456789abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ";

STATIC_FXN void swidCharMap( PChar dest, size_t sizeofDest, const CharMap &chMap ) {
   scpy( dest, sizeofDest, chMap.disp );
   }
STATIC_FXN void swidWordchars  ( PChar dest, size_t sizeofDest ) { swidCharMap( dest, sizeofDest, g_WordChars ); }
STATIC_FXN void swidHLJoinchars( PChar dest, size_t sizeofDest ) { swidCharMap( dest, sizeofDest, g_HLJChars  ); }

STATIC_FXN void swixSetChars( CharMap &chMap, stref pS ) { 0 && DBG("%s+ %" PR_BSR, __func__, BSR(pS) );
   if( 0==cmpi( "nonwhite", pS ) ) {
      for( auto &ch : chMap.is ) { ch = true; }
      chMap.is[                   0 ] = false;
      chMap.is[static_cast<UI>(' ') ] = false;
      chMap.is[static_cast<UI>(HTAB)] = false;
      bcpy( chMap.disp, "nonwhite" );
      }
   else {
      for( auto &ch : chMap.is ) {
         ch = false;
         }
      for( auto isix : pS ) {
         chMap.is[ static_cast<UI>(isix) ] = true;
         }
      {
      auto pWc( chMap.disp );
      for( auto ix(1) ; ix < 256 ; ++ix ) {
         if( chMap.is[ix] ) {
            *pWc++ = ix;
            }
         }
      *pWc = '\0';
      }
      for( auto isix : s_dfltWordChars ) {
         chMap.is[ static_cast<UI>(isix) ] = true;
         }
      }                                       0 && DBG( "%s-", __func__ );
   }
STATIC_FXN void swixWordchars  ( stref pS ) { return swixSetChars( g_WordChars, pS ); }
STATIC_FXN void swixHLJoinchars( stref pS ) { return swixSetChars( g_HLJChars , pS ); }

int isWordChar( int ch ) {
   if( !g_WordChars.is['a'] ) {
      swixWordchars( "" );
      }
   return g_WordChars.is[static_cast<unsigned>(ch)];
   }

int isHJChar( int ch ) {
   if( !g_HLJChars.is['a'] ) {
      swixWordchars( "" );
      }
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
GLOBAL_VAR bool    g_fBpEnabled;

//----------- SWIX's  extern decls put here so there's no doubt that these shouldn't be called except via s_SwiTable

#if    defined(_WIN32)
#define kszHelpPlatEoL "always save modified files to disk w/CRLF (\"DOS\") line endings"
#else
#define kszHelpPlatEoL "always save modified files to disk w/LF (\"Unix\") line endings"
#endif

const enum_nm bkup_enums[] = {
   { bkup_BAK  , "bak"   },
   { bkup_UNDEL, "undel" },
   { bkup_NONE , "none"  },
   };

const enum_nm entab_enums[] = {
   { ENTAB_0_NO_CONV                   , "none"     },
   { ENTAB_1_LEADING_SPCS_TO_TABS      , "leading"  },
   { ENTAB_2_SPCS_NOTIN_QUOTES_TO_TABS , "exoquote" },
   { ENTAB_3_ALL_SPC_TO_TABS           , "all"      },
   };

const enum_nm cursorsize_enums[] = {
   { 0, "small" },
   { 1, "large" },
   };

GLOBAL_CONST char kszBackup[] = "backup";

void SwitblInit() {
   addswi( "askexit"        , new SWIi_bv( g_fAskExit          ) _AHELP( "enable last-chance prompt before terminating the editor session" ) );
   addswi( "beep"           , new SWIi_bv( g_fAllowBeep        ) _AHELP( "beeping allowed (yes) or not (no)" ) );
   addswi( "blankdispmask"  , new SWIi_iv( g_iBlankAnnoDispSrcMask  ) _AHELP( "bitmask: tabdisp, traildisp honored only when 1 (dirty) | 2 (arg-selecting) | 4 (all files)" ) );
   addswi( "boxmode"        , new SWIi_bv( g_fBoxMode          ) _AHELP( "selects BOXARGs (yes) or STREAMARGs (no)" ) );
   addswi( "bpen"           , new SWIi_bv( g_fBpEnabled        ) _AHELP( "enables conditional breakpoints" ) );
   addswi( "case"           , new SWIi_bv( g_fCase             ) _AHELP( "searches are case sensitive (yes) or insensitive (no)" ) );
   addswi( "colorerr"       , new SWI_color( g_colorError     )  _AHELP( "the color of error messages" )              );
   addswi( "colorinf"       , new SWI_color( g_colorInfo      )  _AHELP( "the color of informative text" )            );
   addswi( "colorsta"       , new SWI_color( g_colorStatus    )  _AHELP( "the color of most status-bar information" ) );
   addswi( "colorwbc"       , new SWI_color( g_colorWndBorder )  _AHELP( "the color of window borders" )              );
#if !defined(_WIN32)
   addswi( "conin_tmout"    , new SWIi_iv( g_iConin_nonblk_rd_tmout ) _AHELP( "value passed to ncurses::timeout( value ) when nonblocking ncurses::getch() is configured" ) );
#endif
   addswi( "cursorsize"     , new SWI_enum( [](){ return g_iCursorSize ; }, [](int v_){ g_iCursorSize = v_; ConOut::SetCursorSize( ToBOOL(g_iCursorSize) ); }, AEOA(cursorsize_enums) ) _AHELP( "small, large" ) );
   addswi( "delims"         , new SWIsb( swidDelims     , SetCurDelims    )  _AHELP( "string containing delimiters" ) );
   addswi( "dialogtop"      , new SWIi_bv( g_fDialogTop        ) _AHELP( "dialog & status lines placed at top (yes) or bottom (no) of screen" ) );
#if defined(_WIN32)
   addswi( "dvlogcmds"      , new SWIi_bv( g_fDvlogcmds        ) _AHELP( "log non-cursor-movement cmds to DbgView using Windows' OutputDebugString()" ) );
#endif
   addswi( "editreadonly"   , new SWIi_bv( g_fEditReadonly     ) _AHELP( "allow (yes) or prevent (no) editing of files which are not writable on disk" ) );
   addswi( "entab"          , new SWI_enum( []()->int { return g_CurFBuf()->Entab(); }, [](int v_){ g_CurFBuf()->SetEntabOk( v_ ); }, AEOA(entab_enums) )     _AHELP( "when lines are modified, convert 0/none,1/leading,2/exoquote,3/all spaces to tabs" ) );
   addswi( "errprompt"      , new SWIi_bv( g_fErrPrompt        ) _AHELP( "error message display pauses with \"Press any key...\" prompt" ) );
   addswi( "fastsearch"     , new SWIi_bv( g_fFastsearch       ) _AHELP( "use fast search algorithm (when key contains no spaces)" ) );
   addswi( "forceplateol"   , new SWIi_bv( g_fForcePlatformEol ) _AHELP(  kszHelpPlatEoL ) );
   addswi( "ftype"          , new SWIs( GetCurFtype, [](stref v_){ SetCurFtype( v_ ); return stref(""); } )  _AHELP( "set ftype" ) );
   addswi( "hike"           , new SWIi_iv( g_iHike                  ) _AHELP( "the distance from the cursor to the top/bottom of the window if you move the cursor out of the window by more than the number of lines specified by vscroll, as percent of window size" ) );
   addswi( "hljoinchars"    , new SWIsb( swidHLJoinchars, swixHLJoinchars )  _AHELP( "Hierarchial Left Join charset: chars that, when seen to the left of the cursor, join other identifiers further left to the word under cursor for WUC highlighting purposes; [_a-zA-Z0-9] are always members" ) );
   addswi( "hscroll"        , new SWIi_ci( [](){ return g_iHscroll  ; }, [](int v_){ g_iHscroll   = v_; }, [](){ return 1; }, [](){ return EditScreenCols ()-1; }, false ) _AHELP( "the number of columns that the editor scrolls the text left or right when you move the cursor out of the window" ) );
   addswi(  kszBackup       , new SWI_enum( [](){ return g_iBackupMode ; }, [](int v_){ g_iBackupMode = v_; }, AEOA(bkup_enums)  )     _AHELP( "choices are 'undel', 'bak' or 'none'; see online help for details" ) );
   addswi( "langhilites"    , new SWIi_bv( g_fLangHilites      ) _AHELP( "enable (yes) partial language-aware hilighting" ) );
   addswi( "luagcstep"      , new SWIi_iv( g_iLuaGcStep             ) _AHELP( "in the idle thread, if $luagcstep > 0 then lua_gc( L, LUA_GCSTEP, $luagcstep )" ) );
   addswi( "m4backtickquote", new SWIi_bv( g_fM4backtickquote  ) _AHELP( "spanning backtick quoting right ends with ' (yes) or ` (no)" ) );
   addswi( "maxundo"        , new SWIi_iv( g_iMaxUndo               ) _AHELP( "maximum number of major undo-steps allowed before oldest undo-step is discarded" ) );
   addswi( "memusgink"      , new SWIi_bv( g_fShowMemUseInK    ) _AHELP( "Show memory usage message in Kbytes (yes) or Mbytes (no)" ) );
   addswi( "mfgrepnoise"    , new SWIi_bv( g_fMfgrepNoise      ) _AHELP( "during mfgrep and mfreplace execution: display (yes) or hide (no) each filename & fio-phase display" ) );
   addswi( "msgflush"       , new SWIi_bv( g_fMsgflush         ) _AHELP( "<compile> is flushed (yes) or retained (no) when a new job is started" ) );
   addswi( "realtabs"       , new SWIi_bv( g_fRealtabs         ) _AHELP( "see online help" ) );
   addswi( "replacecase"    , new SWIi_bv( g_fReplaceCase      ) _AHELP( "replace operations are case sensitive (yes) or insensitive (no)" ) );
   addswi( "rmargin"        , new SWIi_iv( g_iRmargin               ) _AHELP( "see online help" ) );
   addswi( "showfbufdetails", new SWIi_bv( g_fShowFbufDetails  ) _AHELP( "show FBUF status details in <winN> sysbufs" ) );
   addswi( "softcr"         , new SWIi_bv( g_fSoftCr           ) _AHELP( "see online help" ) );
   addswi( "tabalign"       , new SWIi_bv( g_fTabAlign         ) _AHELP( "within tab fields, cursor can be positioned (yes) only on tab char (no) in any column" ) );
   addswi( "tabdisp"        , new SWI_chdisp( g_chTabDisp        ) _AHELP( "the numeric ASCII code of the character used to display tab characters; if 0, the space character is used" ) );
   addswi( "tabwidth"       , new SWIi_ci( [](){ return g_iTabWidth ; }, [](int v_){ g_iTabWidth  = v_; DispNeedsRedrawAllLinesAllWindows(); }, [](){ return 1; }, [](){ return MAX_TAB_WIDTH; }, false ) _AHELP( "the width of a 'tab-column'; set PER FILE" ) );
   addswi( "traildisp"      , new SWI_chdisp( g_chTrailSpaceDisp ) _AHELP( "the numeric ASCII code of the character used to display trailing spaces on a line; if 0, the space character is used" ) );
   addswi( "traillinedisp"  , new SWI_chdisp( g_chTrailLineDisp  ) _AHELP( "the numeric ASCII code of the character used to display trailing lines at the end of file; if 0, the space character is used" ) );
   addswi( "traillinewrite" , new SWIi_bv( g_fTrailLineWrite   ) _AHELP( "write to disk file (yes) or discard (no) trailing empty lines" ) );
   addswi( "trailspace"     , new SWIi_bv( g_fTrailSpace       ) _AHELP( "preserve (yes) or remove (no) trailing spaces on lines that are modified" ) );
 #if MOUSE_SUPPORT
   addswi( "usemouse"       , new SWIi_bv( g_fUseMouse         ) _AHELP( "accept (yes) or ignore (no) mouse input" ) );
 #endif
   addswi( "viewonly"       , new SWIi_bv( g_fViewOnly         ) _AHELP( "files subsequently opened will be editable (no) or no-edit (yes)" ) );
   addswi( "vscroll"        , new SWIi_ci( [](){ return g_iVscroll  ; }, [](int v_){ g_iVscroll   = v_; }, [](){ return 1; }, [](){ return EditScreenLines()-1; }, false ) _AHELP( "the number of lines scrolled when the mlines and plines functions move the cursor out of the window" ) );
#if VARIABLE_WINBORDER
   addswi( "wbcidx"         , new SWIi_ci( [](){ return g_swiWBCidx ; }, [](int v_){ g_swiWBCidx  = v_; DispNeedsRedrawTotal(); }, [](){ return 0; }, [](){ return Max_wbc_idx(); } ) _AHELP( "select window border charset (0..7)" ) );
#endif
   addswi( "wcshowdotdir"   , new SWIi_bv( g_fWcShowDotDir     ) _AHELP( "in recursive wildcard buffers show (yes) or hide (NO) dir (subtrees) whose names start with '.'" ) );
   addswi( "wordchars"      , new SWIsb( swidWordchars  , swixWordchars   )  _AHELP( "the set of valid word-component characters; [_a-zA-Z0-9] are always members" ) );
   addswi( "wordwrap"       , new SWIi_bv( g_fWordwrap         ) _AHELP( "the editor performs automatic word-wrapping as you type past rmargin" ) );
   addswi( "wucminlen"      , new SWIi_ci( [](){ return g_iWucMinLen; }, [](int v_){ g_iWucMinLen = v_; }, [](){ return 1; }, [](){ return 32                 ; }, false ) _AHELP( "minimum length of a word for it to qualify for 'word under the cursor' status" ) );
   }

void FBufRead_Assign_Switches( PFBUF pFBuf ) {
   FBufRead_Assign_SubHd( pFBuf, "Switches", s_switbl.size() );
   Xbuf xb;
   for( auto &swic : s_switbl ) {
      pFBuf->xFmtLastLine( &xb, "%-20" PR_BSR ": %-*" PR_BSR " # %" PR_BSR "", BSR(swic.name()), g_MaxKeyNameLen, BSR(swic.disp()), BSR(swic.help()) );
      }
   }

STATIC_FXN SWI_intf_base *FindSwitch( stref pszSwiName ) {
   for( auto &swic : s_switbl ) {
      if( swic.NameMatch( pszSwiName ) ) {
         return &swic;
         }
      }
   return nullptr;
   }

bool SetSwitch( stref switchName, stref newValue ) {
   trim( switchName ); trim( newValue );  0 && DBG( "SetSwitch '%" PR_BSR "' ", BSR(switchName) );
   auto pSwi( FindSwitch( switchName ) );
   if( pSwi ) {
      if( g_pFBufAssignLog ) { g_pFBufAssignLog->FmtLastLine( "ASGN  %" PR_BSR "='%" PR_BSR "'", BSR(switchName), BSR(newValue) ); }
      if( newValue.empty() ) { return Msg( "ERR setting switch %" PR_BSR ": empty switch value", BSR(switchName) ); }
      const auto rv( pSwi->defn( newValue ) );
      if( !rv.empty() ) {
         return Msg( "ERR setting switch %" PR_BSR ": %s", BSR(switchName), rv.c_str() );
         }
      return true;
      }
   return false;
   }
