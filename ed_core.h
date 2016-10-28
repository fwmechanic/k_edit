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

#pragma once

// static_assert( CHAR_MAX > SCHAR_MAX, "char not unsigned" );

//
// MACRO_BACKSLASH_ESCAPES - Affects escape char defn and behavior in macro
//                           string literals:
//
// if 0
//      Only escape char is '"' for '"' (this was an old Borland C compiler
//      extension which I liked).  You DON'T have to escape every '\' with
//      another '\'.  This makes working with MS directory paths or regular
//      expressions embedded within macro string literals HUGELY EASIER.
//      Drawback is that you can't escape any characters except '"'.
//
// if 1
//      '\' is the escape char in macro string literals.  Any literal '\'
//      desired must be escaped by a preceding '\'.  This allows various sorts
//      of flexibility, but is a HUGE practical hassle when it comes to MS
//      directory paths and regular expressions which both make heavy use of '\'.
//
#define  MACRO_BACKSLASH_ESCAPES  0

// Editor Meta-Types

#if 0

  // int stays 32-bit in w64, so forcing these to 64-bits is a radical change

  #ifndef PTRDIFF_MAX
  #error undefined PTRDIFF_MAX
  #endif

typedef ptrdiff_t   COL ;     // column or position within line
typedef ptrdiff_t   LINE;     // line number within file
constexpr LINE MAX_LINES = PTRDIFF_MAX;

#else

  // "the way we were (pre x64)"
  // according to https://news.ycombinator.com/item?id=7712328
  // use of 32-bit data in x64 is _preferred_ for multiple reasons
  // a. smaller data footprint leads to better dcache effects.
  // b. smaller opcodes to ref 32-bit data "due to the lack of REX
  //    prefixes in instructions" leads to better icache effects.
  // [So this will probably remain the status quo for as long as x64 is the dominant platform]

typedef int COL ;     // column or position within line
typedef int LINE;     // line number within file
   enum { MAX_LINES = INT_MAX };

#endif

#include "dlink.h"

class FBUF;
typedef       FBUF * PFBUF;
typedef FBUF const * PCFBUF;
typedef      PFBUF * PPFBUF;
typedef     PCFBUF const * CPPFBUF;

typedef  DLinkHead<FBUF> FBufHead;

// Use OpenFileNotDir_... in lieu of fChangeFile if you don't want to affect
// PFBUF-stack-order and cwd
//
extern PFBUF OpenFileNotDir_(               PCChar pszName, bool fCreateOk );
STIL   PFBUF OpenFileNotDir_NoCreate(       PCChar pszName ) { return OpenFileNotDir_( pszName, false ); }
STIL   PFBUF OpenFileNotDir_CreateSilently( PCChar pszName ) { return OpenFileNotDir_( pszName, true  ); }

/*

LINELEN is the maximum line length that can be passed or will be returned by any
editor API.

   [2014]: a concerted effort has been made in recent years to remove this
   (line-length) limitation everywhere thru use of XBuf, therefore the preceding
   statement has become nearly ubiquitously false.  However use of the linebuf
   and Linebuf types has not been eradicated :-(

BUFBYTES is the size of a buffer that can hold such a line w/a
terminating NUL.

  COL_MAX  max COL (a COL is a 0-based INDEX of a non-EOL char)
  LINELEN  number of chars that can be stored in a line, NOT including the terminating NUL
  BUFBYTES number of bytes in a buffer, including space for the terminating NUL

*/
constexpr COL COL_MAX  = INT_MAX-1; // -1 is a HACK to avoid integer overflow in cases like alcc->PutColor( xMin, xMax-xMin+1, ColorTblIdx::COM ); where xMin==0 and xMax==COL_MAX

constexpr COL LINELEN  = (512)+1  ;    // DEPRECATED
constexpr COL BUFBYTES = LINELEN+1;    // DEPRECATED
typedef char linebuf[BUFBYTES];    // DEPRECATED line buffer
typedef char pathbuf[_MAX_PATH+1]; // DEPRECATED Pathname buffer
typedef  FixedCharArray<BUFBYTES> Linebuf; // DEPRECATED

typedef  FmtStr<BUFBYTES>    SprintfBuf;
typedef  FmtStr<2*BUFBYTES>  Sprintf2xBuf;

//=============================================================================

#define  USE_CURSORLINE_HILITE       1
#define  USE_CURSORLINE_VERT_HILITE  1

// Editor color table indicies
namespace ColorTblIdx { // see color2Lua
   enum { // these are ARRAY _INDICES_!
      TXT,  // foreground (normal)
      HIL,  // highlighted region
      SEL,  // selection
      WUC,  // word-under-cursor hilight
      CXY,  // cursor line (and column) hilight
      CPP,  // cppc line hilight
      COM,  // comment hiliting, etc.
      STR,  // litStr
      VIEW_COLOR_COUNT,

      INF=VIEW_COLOR_COUNT,  // information
      STS,  // status line
      WND,  // window border
      ERRM,  // error message
      COLOR_COUNT // MUST BE LAST!
      }; // NO MORE THAN 16 ALLOWED!
   }

// CompileTimeAssert( ColorTblIdx::COLOR_COUNT <= 16 ); // all ColorTblIdx:: must fit into a nibble

extern uint8_t g_colorInfo     ; // INF
extern uint8_t g_colorStatus   ; // STA
extern uint8_t g_colorWndBorder; // WND
extern uint8_t g_colorError    ; // ERR

#define  HILITE_CPP_CONDITIONALS  1

#include "conio.h"

struct Point {   // file location
   LINE    lin = 0;
   COL     col = 0;
   Point() {}
   Point( LINE yLine, COL xCol ) : lin(yLine), col(xCol) {}
   Point( const YX_t &src ) : lin( src.lin ), col( src.col ) {} // conv from conio.h type
   Point( const Point  &rhs, LINE yDelta=0, COL xDelta=0 ) : lin(rhs.lin + yDelta), col(rhs.col + xDelta) {} // COPY CTOR
   void Set( LINE yLine, COL xCol ) { lin = yLine, col = xCol; }
   bool operator==( const Point &rhs ) const { return lin == rhs.lin && col == rhs.col; }
   bool operator!=( const Point &rhs ) const { return !(*this == rhs); }
   bool operator< ( const Point &rhs ) const { return lin < rhs.lin || (lin == rhs.lin && col < rhs.col); }
   bool operator> ( const Point &rhs ) const { return lin > rhs.lin || (lin == rhs.lin && col > rhs.col); }
   bool operator>=( const Point &rhs ) const { return !(*this < rhs); }
   bool operator<=( const Point &rhs ) const { return !(*this > rhs); }
   }; // HAS CTORS, so union canNOT HAS-A one of these

class FBufLocn { // dflt ctor leaves locn empty; must be Set() later
   PFBUF d_pFBuf = nullptr;
   Point d_pt;
   COL   d_width = 1;
public:
   FBufLocn() : d_pFBuf(nullptr) {}
   FBufLocn( PFBUF pFBuf, const Point &pt, COL width=1 ) : d_pFBuf(pFBuf), d_pt(pt), d_width(width) {}
   void Set( PFBUF pFBuf, Point pt, COL width=1 ) { d_pFBuf=pFBuf, d_pt=pt, d_width=width; }
   bool         ScrollToOk() const;
   bool         IsSet()     const { return d_pFBuf != nullptr; }
   bool         InCurFBuf() const;
   bool         Moved()     const;
   PFBUF        FBuf()      const { return d_pFBuf; }
   const Point &Pt()        const { return d_pt   ; }
   bool operator==( const FBufLocn &rhs ) const { return d_pFBuf == rhs.d_pFBuf && d_pt == rhs.d_pt && d_width == rhs.d_width; }
   };

class FBufLocnNow : public FBufLocn { // dflt ctor uses curfile, cursor; const instances work fine
public:
   FBufLocnNow();
   };

struct Rect {
   Point flMin;  // - Lower line, or leftmost col
   Point flMax;  // - Higher, or rightmost
   Rect() {}
   Rect( Point ulc, Point lrc ) : flMin(ulc), flMax(lrc) {}
   Rect( LINE yUlc, COL xUlc, LINE yLrc, COL xLrc ) : flMin(yUlc, xUlc), flMax(yLrc, xLrc) {}
   Rect( PFBUF pFBuf );
   Rect( bool fSearchFwd );
   COL  Width()  const { return flMax.col - flMin.col + 1; }
   COL  Height() const { return flMax.lin - flMin.lin + 1; }
   int  cmp_line( LINE yLine ) const { // IGNORES EFFECT OF COLUMNS!
      if( yLine < flMin.lin ) { return -1; }
      if( yLine > flMax.lin ) { return +1; }
      return 0;
      }
   };

class Xbuf {
   // ever-growing string class :-) which implements an ...
   // ever-growing line buffer intended to be used for all lines touched over
   // the duration of a command or operation, in lieu of malloc'ing a buffer for
   // each line touched.
   PChar           d_buf;
   size_t          d_buf_bytes;
   STATIC_VAR char ds_empty;
public:
   Xbuf()                          : d_buf (&ds_empty), d_buf_bytes( 0 ) {}
   Xbuf( size_t size )             : d_buf ( nullptr ), d_buf_bytes( 0 ) { wresize( size ); }
   Xbuf( PCChar str )              : d_buf ( nullptr ), d_buf_bytes( 0 ) { assign( str ); }
   Xbuf( PCChar str, size_t len_ ) : d_buf ( nullptr ), d_buf_bytes( 0 ) { assign( str, len_ ); }
   ~Xbuf() { if( &ds_empty!=d_buf ) { Free_( d_buf ); } }
public:
   PChar    wbuf()      const { return d_buf;       }
   PCChar   c_str()     const { return d_buf;       }
   size_t   buf_bytes() const { return d_buf_bytes; }
   void     clear()           { poke( 0, '\0' );    }
   PChar wresize( size_t size ) {
      if( d_buf_bytes < size ) {
          d_buf_bytes = ROUNDUP_TO_NEXT_POWER2( size, 512 );
          if( &ds_empty==d_buf ) { d_buf = nullptr; }
          ReallocArray( d_buf, d_buf_bytes, __func__ );
         }
      return d_buf;
      }
   size_t length() const {
      const auto pnul( PChar( memchr( d_buf, 0, d_buf_bytes ) ) );
      return pnul ? pnul - d_buf : 0;
      }
   stref sr() const { return stref( d_buf, length() ); }
   PCChar push_back( char ch ) {
      const auto slen( length() );
      const auto rv( wresize( slen+2 ) );
      rv[slen] = ch;
      rv[slen+1] = '\0';
      return rv;
      }
   PCChar cat( PCChar str ) {
      return cat( str, Strlen(str) );
      }
private:
   PCChar cat( PCChar str, size_t len_ ) {
      const auto len0( d_buf ? Strlen(d_buf) : 0 );
      const auto rv( wresize( 1+len0+len_ ) );
      memcpy( rv+len0, str, len_ );
      rv[len0+len_] = '\0';
      return rv;
      }
   PCChar assign( PCChar str ) {
      const size_t len_( Strlen(str) );
      const auto rv( wresize( 1+len_ ) );
      memcpy( rv, str, len_ );
      rv[len_] = '\0';
      return rv;
      }
   PCChar assign( PCChar str, const size_t len_ ) {
      const auto rv( wresize( 1+len_ ) );
      memcpy( rv, str, len_ );
      rv[len_] = '\0';
      return rv;
      }
   PCChar poke( int xCol, char ch, char fillch=' ' ) {
      const auto rv( wresize( 1+xCol ) );
      const auto len0( length() );
      if( xCol > len0 ) {
         for( auto ir=len0; ir < xCol ; ++ir ) { rv[ir] = fillch; }
         }
      rv[xCol] = ch;
      if( ch && xCol >= len0 ) {
         rv[1+xCol] = '\0';
         }
      return rv;
      }
public:
   PCChar vFmtStr( PCChar format, va_list val ) {
      va_list  val_copy;        // http://stackoverflow.com/questions/9937505/va-list-misbehavior-on-linux
      va_copy( val_copy, val ); // http://julipedia.meroh.net/2011/09/using-vacopy-to-safely-pass-ap.html
      const auto olen( 1+WL(_vsnprintf,vsnprintf)( nullptr, 0   , format, val_copy ) );
      va_end( val_copy );
      const auto rv( wresize( olen ) );
                         WL(_vsnprintf,vsnprintf)( rv     , olen, format, val );
      return rv;
      }
   PCChar FmtStr( PCChar format, ... ) ATTR_FORMAT(2, 3) {
      va_list val; va_start(val, format);
      vFmtStr( format, val );
      va_end(val);
      return c_str();
      }
   };  typedef Xbuf *PXbuf;

class LineInfo { // LineInfo is a standalone class since it is used by both FBUF and various subclasses of EditRec (undo/redo)
   friend class FBUF;
   NO_COPYCTOR(LineInfo);
   NO_ASGN_OPR(LineInfo);
   PCChar d_pLineData;           // CAN be -1 when REPLACEREC is for line that didn't exist
   COL    d_iLineLen;
public:
   void clear() {
      d_pLineData = nullptr;
      d_iLineLen  = 0;
      }
   LineInfo(LineInfo&& rhs)               // move constructor     so we can std::swap(), std::move() these
      : d_pLineData( rhs.d_pLineData )
      , d_iLineLen ( rhs.d_iLineLen  )
      {
      rhs.clear();
      }
   LineInfo& operator=(LineInfo&& rhs) {  // move assignment op   so we can std::swap() these
      if( this != &rhs ) {
         d_pLineData = rhs.d_pLineData ;
         d_iLineLen  = rhs.d_iLineLen  ;
         rhs.clear();
         }
      return *this;
      }
   void   PutContent( stref src );
   void   FreeContent( const FBUF &fbuf );
   PCChar GetLineRdOnly()                        const { return d_pLineData; }
   COL    GetLineLen()                           const { return d_iLineLen;  }
   bool   fCanFree_pLineData( const FBUF &fbuf ) const;
   };

//-----------------------------------------------------------------------------

class LineColors       ;
class LineColorsClipped;

//-----------------------------------------------------------------------------

#include "conio.h"

struct SearchScanMode; // forward

// ARG type bits
//
// These have two closedly-related but different uses:
//
// 1) In CMD::d_argType (a static object), these specify the allowed argument
//    types which CMD.func will accept.
//
// 2) In ARG::d_argType (a dynamic object), specifies the processed arg type
//    being supplied to CMD.func in this invocation.
//
enum               // Actually can be set in ARG::Abc?
   {               // |
   bpNOARG      ,  // * no argument specified
     bpNOARGWUC ,  //   no arg => word under cursor (converts to TEXTARG, which must also be set)
   bpNULLARG    ,  // * arg + no cursor movement
   bpTEXTARG    ,  // * text specified (NULLEOL, NULLEOW and BOXSTR convert to this)
   bpLINEARG    ,  // * contiguous range of entire lines
   bpBOXARG     ,  // * box delimited by arg, cursor
   bpSTREAMARG  ,  // * from low-to-high, viewed 1-D
     bpNULLEOL  ,  //   null arg => text from arg->eol (converts to TEXTARG, which must also be set)
     bpNULLEOW  ,  //   null arg => text from arg->end of word (converts to TEXTARG, which must also be set)
     bpBOXSTR   ,  //   single-line box selection converts to TEXTARG, which must also be set
   bpNUMARG     ,  //   text => delta to y position (converts to LINEARG, which must also be set)
   bpMARKARG    ,  //   text => mark at end of arg
   bpMODIFIES   ,  //   modifies current file (if > granularity is needed, DON'T specify MODIFIES; call FBUF::CantModify() in code paths which MODIFY.
   bpKEEPMETA   ,  //   do not consume meta flag
   bpWINDOWFUNC ,  //   do not cancel highlight resulting from a previous command, such as psearch.
   bpCURSORFUNC ,  //   do not recognize or cancel Arg prefix.  Conflicts with all flags except MODIFIES and KEEPMETA.
   bpMACROFUNC  ,  //   nests/pushes literal text from which the command stream is generated.  Conflicts with all flags except KEEPMETA.
   };

typedef uint32_t ArgType_t;
#define DEFINE_ARGTYPE( argnm )  constexpr ArgType_t argnm = BIT( bp##argnm )
                              // Actually can be set in ARG::Abc?
                              // |
DEFINE_ARGTYPE( NOARG      ); // * no argument specified
DEFINE_ARGTYPE(   NOARGWUC ); //   no arg => word under cursor (converts to TEXTARG, which must also be set)
DEFINE_ARGTYPE( NULLARG    ); // * arg + no cursor movement
DEFINE_ARGTYPE( TEXTARG    ); // * text specified (NULLEOL, NULLEOW and BOXSTR convert to this)
DEFINE_ARGTYPE( LINEARG    ); // * contiguous range of entire lines
DEFINE_ARGTYPE( BOXARG     ); // * box delimited by arg, cursor
DEFINE_ARGTYPE( STREAMARG  ); // * from low-to-high, viewed 1-D
DEFINE_ARGTYPE(   NULLEOL  ); //   null arg => text from arg->eol (converts to TEXTARG, which must also be set)
DEFINE_ARGTYPE(   NULLEOW  ); //   null arg => text from arg->end of word (converts to TEXTARG, which must also be set)
DEFINE_ARGTYPE(   BOXSTR   ); //   single-line box selection converts to TEXTARG, which must also be set
DEFINE_ARGTYPE( NUMARG     ); //   text => delta to y position (converts to LINEARG, which must also be set)
DEFINE_ARGTYPE( MARKARG    ); //   text => mark at end of arg
DEFINE_ARGTYPE( MODIFIES   ); //   modifies current file (if finer granularity is needed, DON'T specify MODIFIES; call FBUF::CantModify() in code paths which MODIFY.
DEFINE_ARGTYPE( KEEPMETA   ); //   do not consume meta flag
DEFINE_ARGTYPE( WINDOWFUNC ); //   do not cancel highlight resulting from a previous command, such as psearch.
DEFINE_ARGTYPE( CURSORFUNC ); //   do not recognize or cancel Arg prefix.  Conflicts with all flags except MODIFIES and KEEPMETA.
DEFINE_ARGTYPE( MACROFUNC  ); //   nests/pushes literal text from which the command stream is generated.  Conflicts with all flags except KEEPMETA.
#undef DEFINE_ARGTYPE

// only ONE among ACTUAL_ARGS may be set in ARG::d_argType when a ARG::Xyz is Invoke()'ed
constexpr ArgType_t ACTUAL_ARGS = NOARG | TEXTARG | NULLARG | LINEARG | BOXARG | STREAMARG;

// CMD's w/ANY of these SET need additional ARG processing when Invoke()'ed
constexpr ArgType_t TAKES_ARG   = ACTUAL_ARGS | NOARGWUC | NULLEOW | NULLEOL | NUMARG;

// note that 'mark' fxn does NOT have NUMARG set!
#define IS_NUMARG    (d_argType == LINEARG)
#define NUMARG_VALUE (d_linearg.yMax - d_linearg.yMin + 1)

struct CMD;
typedef       CMD *         PCMD;
typedef CMD const *        PCCMD;
typedef CMD const * const CPCCMD;

struct ARG {
   ArgType_t d_argType;
   int      d_cArg;    // count of <arg>s pressed: 0 for NOARG
   struct {            // no argument specified
      Point cursor;    // - cursor
      } d_noarg;
   struct {            // null argument specified
      Point cursor;    // - cursor
      } d_nullarg;
   struct {            // text argument specified
      Point  ulc;      // - cursor (or left end of seln if any)
      PCChar pText;    // - ptr to text of arg
      } d_textarg;
   struct LINEARG_t {  // line argument specified
      LINE   yMin;     // - starting line of selection
      LINE   yMax;     // - ending   line of selection
      } d_linearg;
   typedef Rect STREAMARG_t;  STREAMARG_t d_streamarg; // stream argument specified  (char at 'd_streamarg.flMax' is NOT INCLUDED in the selection)
   typedef Rect BOXARG_t;     BOXARG_t    d_boxarg;    // box argument specified
   bool     d_fMeta;
   PCCMD    d_pCmd;
   PFBUF    d_pFBuf;
   //*******  END OF DATA
   void     BeginPt( Point *pPt ) const;
   void     EndPt( Point *pPt ) const;
   bool     Within( const Point &pt, COL len=-1 ) const;
   bool     Beyond( const Point &pt ) const;
   COL      GetLine( std::string &st, LINE yLine ) const;
   void     ColsOfArgLine( LINE yLine, COL *pxLeftIncl, COL *pxRightIncl ) const;
   void     GetColumnRange( COL *pxMin, COL *pxMax ) const;
   int      GetLineRange( LINE *yTop, LINE *yBottom ) const;
private:
   bool     FillArgStructFailed();
   bool     BOXSTR_to_TEXTARG( LINE yOnly, COL xMin, COL xMax );
public:
   bool     InitOk( PCCMD pCmd );
   void     SaveForRepeat() const;
   PCChar   CmdName() const;
   bool     Invoke();
   bool     fnMsg( PCChar fmt, ... ) const ATTR_FORMAT(2, 3) ;
   bool     BadArg() const; // a specific error: boilerplate (but informative) err msg
   bool     ErrPause( PCChar fmt, ... ) const ATTR_FORMAT(2, 3) ;
   PCChar   ArgTypeName() const;
   void     ConvertStreamargToLineargOrBoxarg();
   void     ConvertLineOrBoxArgToStreamArg();
   ArgType_t ActualArgType() const { return d_argType & ACTUAL_ARGS; }
private:
   bool   pmlines( int direction ); // factored from mlines and plines
public:
   // EDITOR_FXNs start here!!!
   typedef bool (ARG::*pfxCMD)(); // declare type of pointer to class method
   bool RunMacro();
   bool ExecLuaFxn();
// bool _spushArg();
   //--------------------------
   #define CMDTBL_H_ARG_METHODS
   #include "cmdtbl.h"
   #undef  CMDTBL_H_ARG_METHODS
   //--------------------------
   };  typedef ARG const * const CPCARG;

extern std::string StreamArgToString( PFBUF pfb, Rect stream );

// PCFV_ fxns operate on the current View/FBUF, using ARG::-typed params
// intended mostly for use within ARG:: methods
extern void PCFV_delete_STREAMARG( ARG::STREAMARG_t const &d_streamarg, bool copyToClipboard );
extern void PCFV_delete_LINEARG  ( ARG::LINEARG_t   const &d_linearg  , bool copyToClipboard );
extern void PCFV_delete_BOXARG   ( ARG::BOXARG_t    const &d_boxarg   , bool copyToClipboard, bool fCollapse=true );
extern void PCFV_BoxInsertBlanks ( ARG::BOXARG_t    const &d_boxarg   );
extern void PCFV_delete_ToEOL    ( Point            const &curpos     , bool copyToClipboard );

class ArgLineWalker {
   NO_COPYCTOR(ArgLineWalker);
   NO_ASGN_OPR(ArgLineWalker);
   // Simplest way to traverse an ARG region: thin wrapper encapsulating ARG + a
   // Point variable.  Useful for simple iterations: when the Point IS NOT
   // manipulated by other agents like FileSearcher
   //
   // If the Point is manipulated by other agents like FileSearcher, it's best
   // to use ARG:: methods directly.
   //
   CPCARG       d_Arg;
   Point        d_pt;
   std::string  d_st;
public:
   ArgLineWalker( CPCARG Arg ) : d_Arg(Arg) { d_Arg->BeginPt( &d_pt ); }
   bool   Beyond()              const { return d_Arg->Beyond( d_pt ); }
   bool   Within( COL len=-1 )  const { return d_Arg->Within( d_pt, len ); }
   LINE   Line()                const { return d_pt.lin; }
   LINE   Col ()                const { return d_pt.col; }
   COL    GetLine()                   { return d_Arg->GetLine( d_st, d_pt.lin ); }
   bool   NextLine()                  { d_pt.col = 0; ++d_pt.lin; return Beyond(); }
   void   buf_erase( size_t pos )     {        d_st.erase( pos ); }
   PCChar c_str()               const { return d_st.c_str(); }
   stref  lineref()             const { return d_st; }
   };

//
//  Function definition table definitions
//
typedef ARG::pfxCMD funcCmd;

STIL funcCmd fn_runmacro()  { return &ARG::RunMacro   ; }
STIL funcCmd fn_runLua()    { return &ARG::ExecLuaFxn ; }

struct GTS {
   enum eRV { KEEP_GOING, DONE };
   int           &xCursor_     ;
   std::string   &stb_         ;
   int           &textargStackPos_;
   //-------------------------------- ref/value boundary
   const PCCMD    pCmd_        ; // if valid (currently only when we're called by ArgMainLoop) will be ARG::graphic, the first char of a typed arg.
   const COL      xColInFile_  ;
   const int      flags_       ;
   const bool     fInitialStringSelected_;

   // methods start here!!!
   typedef eRV (GTS::*pfxGTS)(); // declare type of pointer to class method
   #define CMDTBL_H_GTS_METHODS
   #include "cmdtbl.h"
   #undef  CMDTBL_H_GTS_METHODS
   };

typedef GTS::pfxGTS pfxGTS;

#   define  AHELP( x )   x
#   define _AHELP( x ) , x

struct CMD {              // function definition entry
   PCChar    d_name;      // - pointer to name of fcn     !!! DON'T CHANGE ORDER OF FIRST 2 ELEMENTS
   funcCmd   d_func;      // - pointer to function        !!! UNLESS you change initializer of macro_graphic
   pfxGTS    d_GTS_fxn;   // - pointer to function
   ArgType_t d_argType;   // - user args allowed
   PCChar    d_HelpStr;   // - help string shown in <CMD-SWI-Keys>
   union {
      EdKC_Ascii eka;
      PCChar     pszMacroDef;
      char  chAscii() const { return eka.Ascii; }
      int   isEmpty() const { return pszMacroDef == nullptr; }
      } d_argData;   // - NON-MACRO: key that invoked; MACRO: ptr to macro string (defn)
   mutable  uint32_t  d_callCount;  // - how many times user has invoked (this session) since last cmdusage_updt() call
   mutable  uint32_t  d_gCallCount; // - how many times user has invoked, global
   bool     isCursorFunc(        )  const { return ToBOOL(d_argType &  CURSORFUNC); }
   bool     isCursorOrWindowFunc()  const { return ToBOOL(d_argType & (CURSORFUNC|WINDOWFUNC)); }
            // There are a number of functions which are macro-like (have
            // d_argType |= MACROFUNC) in order to take advantage of that ability
            // to push (typically) a literal string onto the interpreter stack
            // for subsequent functions to consume.  These "ExecutesLikeMacro",
            // but aren't macros per se; macros store their macro string
            // persistently in d_argData.pszMacroDef.
            //
   bool     ExecutesLikeMacro()     const { return ToBOOL(d_argType & MACROFUNC); } // see cmdtbl for all builtin MACROFUNC fxns
   bool     IsRealMacro()           const { return d_func == fn_runmacro(); }
   bool     IsLuaFxn()              const { return d_func == fn_runLua()  ; }
   PCChar   Name()                  const { return d_name; }
   bool     NameMatch( PCChar str ) const { return Stricmp( str, d_name ) == 0; }
   PCChar   MacroText()             const { return d_argData.pszMacroDef; }
   stref    MacroStref()            const { return d_argData.pszMacroDef; }
   bool     BuildExecute()          const;
   bool     IsFnCancel()            const;
   bool     IsFnUnassigned()        const;
   bool     IsFnGraphic()           const;
   // mutators
   void     RedefMacro( stref newDefn );
   void     IncrCallCount() const { ++d_callCount; }
   };

inline PCChar ARG::CmdName() const { return d_pCmd->Name(); }

// forward decls:

class                            EditRec;
struct                           HiLiteRec;
class                            View;
   typedef View       *          PView;
   typedef View const *          PCView;
typedef  DLinkHead<View>         ViewHead;
class                            ViewHiLites;
class                            FileTypeSettings;
struct                           Win;
   typedef Win        *          PWin;
   typedef Win const  *          PCWin;
class                            HiliteAddin;
typedef  DLinkHead<HiliteAddin>  HiliteAddinHead;
extern void DestroyViewList( ViewHead *pViewHd );

struct FTypeSetting;

class View { // View View View View View View View View View View View View View View View View View View View View View View View View
public:
   DLinkEntry<View> dlinkViewsOfWindow;
   DLinkEntry<View> dlinkViewsOfFBUF;
                View( const View &, PWin pWin );
                View( PFBUF pFBuf , PWin pWin, PCChar szViewOrdinates=nullptr );
                ~View();
   void         Write( FILE *fout ) const;
private:
                View( const View & ) = delete; // NO copy CTOR !
                View()               = delete; // NO dflt CTOR !
   const PWin   d_pWin;      // back pointer, needed cuz our owning Win knows things we don't, but need
   const PFBUF  d_vwToPFBuf; // back pointer; DO NOT REFERENCE DIRECTLY!!! USE CFBuf() && FBuf() !!!
   ViewHiLites *d_pHiLites = nullptr; // we own this!
   void         CommonInit();
   time_t       d_tmFocusedOn = 0; // http://en.wikipedia.org/wiki/Year_2038_problem
public:
   void         PutFocusOn();
   time_t       TmFocusedOn() const { return d_tmFocusedOn; }
   PCFBUF       CFBuf()  const { return d_vwToPFBuf; }
   PFBUF        FBuf()   const { return d_vwToPFBuf; }
   PCWin        Win()    const { return d_pWin ; }
   PWin         wr_Win() const { return d_pWin ; }
   bool         ActiveInWin();
   struct ULC_Cursor {
      Point     Origin;
      Point     Cursor;
      bool      fValid = false;
      void Set( const ULC_Cursor &src ) {
         Cursor = src.Cursor;
         Origin = src.Origin;
         fValid = true;
         }
      void EnsureWinContainsCursor( Point winSize );
      };
private:
   ULC_Cursor   d_current;       // current window & cursor state
   ULC_Cursor   d_prev;          // window state before any cursor movements
   LINE         d_prevLineCount = -1; // used for tail-cursor-scrolling
public:
   ULC_Cursor   d_saved;         // window state saved by ARG::savecur
   const Point  &Cursor() const { return d_current.Cursor; }
   const Point  &Origin() const { return d_current.Origin; }
   bool         RestCur();
   void         SaveCur()     { d_saved.Set( d_current ); }
   void         SavePrevCur() { d_prev .Set( d_current ); }
   void         ScrollToPrev();
   Point        GetCursor() const { return d_current.Cursor; }
   void         CapturePrevLineCount();
   LINE         PrevLineCount() const { return d_prevLineCount; }
private:
   void         MoveCursor_( LINE yLine, COL xColumn, COL xWidth, bool fUpdtWUC );
public:
   void         MoveCursor(           LINE yLine, COL xColumn, COL xWidth=1 )   { MoveCursor_( yLine, xColumn, xWidth, true  ); }
   void         MoveCursor( const Point &pt, COL xWidth=1 )                     { MoveCursor_( pt.lin, pt.col, xWidth, true  ); }
   void         MoveCursor_NoUpdtWUC( LINE yLine, COL xColumn, COL xWidth=1 )   { MoveCursor_( yLine, xColumn, xWidth, false ); }
   void         MoveAndCenterCursor( const Point &pt, COL xWidth );
private:
   void         ScrollOriginAndCursor_(         LINE ulc_yLine, COL ulc_xCol, LINE cursor_yLine, COL cursor_xCol, bool fUpdtWUC );
public:
   void         ScrollOriginAndCursor(          LINE ulc_yLine, COL ulc_xCol, LINE cursor_yLine, COL cursor_xCol ) { ScrollOriginAndCursor_( ulc_yLine, ulc_xCol, cursor_yLine, cursor_xCol, true  ); }
   void         ScrollOriginAndCursorNoUpdtWUC( LINE ulc_yLine, COL ulc_xCol, LINE cursor_yLine, COL cursor_xCol ) { ScrollOriginAndCursor_( ulc_yLine, ulc_xCol, cursor_yLine, cursor_xCol, false ); }
   void         ScrollOriginAndCursor( const Point &ulc, const Point &cursor ) { ScrollOriginAndCursor( ulc.lin, ulc.col, cursor.lin, cursor.col ); }
   void         ScrollOriginAndCursor( const ULC_Cursor &ulcc )                { ScrollOriginAndCursor( ulcc.Origin, ulcc.Cursor ); }
   void         ScrollOriginYX( LINE yLine, COL xCol );
   void         ScrollOrigin_X_Abs( COL  xCol  )     { ScrollOriginYX(     Origin().lin , xCol         ); }
   void         ScrollOrigin_Y_Abs( LINE yLine )     { ScrollOriginYX(     yLine        , Origin().col ); }
   void         ScrollOrigin_X_Rel( int colDelta  )  { ScrollOrigin_X_Abs( Origin().col + colDelta     ); }
   void         ScrollOrigin_Y_Rel( int lineDelta )  { ScrollOrigin_Y_Abs( Origin().lin + lineDelta    ); }
   void         ScrollByPages( int pages );
   void         EnsureWinContainsCursor();
   //********** screen highlights
   void         SetStrHiLite  ( const Point &pt, COL Cols, int color );
   void         SetMatchHiLite( const Point &pt, COL Cols, bool fErrColor );
   void         InsHiLiteBox( int newColorIdx, Rect newRgn );
   void         InsHiLite1Line( int newColorIdx, LINE yLine, COL xLeft, COL xRight );
   void         FreeHiLiteRects();
   void         RedrawHiLiteRects();
   void         InsertHiLitesOfLineSeg
                   ( LINE               yLine
                   , COL                xIndent
                   , COL                xMax
                   , LineColorsClipped &alcc
                   , const HiLiteRec  *&pFirstPossibleHiLite
                   , bool               isActiveWindow
                   , bool               isCursorLine
                   ) const;
   void         GetLineForDisplay
                   ( std::string       &dest
                   , const COL          xLeft
                   , const COL          xWidth
                   , LineColorsClipped &alcc
                   , const HiLiteRec * &pFirstPossibleHiLite
                   , LINE               yLineOfFile
                   , bool               isActiveWindow
                   ) const;
private:
   bool         d_LineCompile_isValid = false;
   LINE         d_LineCompile = -1;
public:
   bool         LineCompile_Valid() const { return d_LineCompile_isValid; }
   LINE         Get_LineCompile() const { return d_LineCompile; }
   void         Set_LineCompile( LINE yLine );
   bool         LineCompileOk() const;
   bool         Inc_LineCompileOk( int delta=1 ) {
                   d_LineCompile += delta;
                   return LineCompileOk();
                   }
private:
   bool         d_fCursorMoved     = true;
   bool         d_fUpdtWUC_pending = true; // fUpdtWUC_pending is state ASSOCIATED with (or perhaps independent of) fCursorMoved
                                           // which is passed to the VCursorMoved method
public:
   void         ForceCursorMovedCondition() { d_fCursorMoved = true; }
private:
   HiliteAddinHead d_addins;
   bool         InsertAddinLast( HiliteAddin *pAddin );
   unsigned     d_HiliteAddin_Event_FBUF_content_rev = 0;
public:
   void         HiliteAddins_Init();
   void         HiliteAddins_Delete();
   void         HiliteAddin_Event_If_CursorMoved();
   void         HiliteAddin_Event_WinResized();
   void         HiliteAddin_Event_FBUF_content_changed( LINE yMinChangedLine, LINE yMaxChangedLine );
   void         Event_Win_Resized( const Point &newSize );
   void         ViewEvent_LineInsDel( LINE yLine, LINE lineDelta );
   void         PokeOriginLine_HACK( LINE yLine ) { d_current.Origin.lin = yLine; }
   char         CharUnderCursor(); // cursor being a View concept...
   bool         PBalFindMatching( bool fSetHilite, Point *pPt );
   bool         GetBOXSTR_Selection( std::string &st );
   bool         d_LastSelect_isValid;
   Point        d_LastSelectBegin, d_LastSelectEnd;
   bool         prev_balln( LINE yStart, bool fStopOnElse );
   bool         next_balln( LINE yStart, bool fStopOnElse );
public:
   int          ColorIdx2Attr( int colorIdx ) const;
   }; // View View View View View View View View View View View View View View View View View View View View View View View View

struct Win { // Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win
   ViewHead  ViewHd;   // public: exposed
   Point     d_Size;   // public: exposed
   Point     d_UpLeft; // public: exposed
   int       d_wnum = 0;  // will be set correctly if/when this is sorted into correct place
   Point     d_size_pct;
public:
             Win();
             Win( PCChar pC );
             Win( Win &rhs, bool fSplitVertical, int ColumnOrLineToSplitAt );
             ~Win();
   void      Maximize();
   bool      operator< ( const Win &rhs ) const { return d_UpLeft < rhs.d_UpLeft; }
   void      Write( FILE *fout ) const;
   PCView    CurView()   const { return ViewHd.front(); }
   PView     CurViewWr() const { return ViewHd.front(); }
   void      DispNeedsRedrawAllLines() const;
   void      Event_Win_Reposition( const Point &newUlc );
   void      Event_Win_Resized( const Point &newSize );
   void      Event_Win_Resized( const Point &newSize, const Point &newSizePct );
   bool      VisibleOnDisplayLine( LINE yLineOfDisplay ) const { return( WithinRangeInclusive( d_UpLeft.lin, yLineOfDisplay, d_UpLeft.lin + d_Size.lin - 1 ) ); }
   bool      VisibleOnDisplayCol ( COL  xColOfDisplay  ) const { return( WithinRangeInclusive( d_UpLeft.col, xColOfDisplay , d_UpLeft.col + d_Size.col - 1 ) ); }
   bool      GetCursorForDisplay( Point *pt ) const;
   void      GetLineForDisplay( int winNum, std::string &dest, LineColors &alc, const HiLiteRec * &pFirstPossibleHiLite, const LINE yDisplayLine ) const;
   const Point &SizePct() const { return d_size_pct; }
   void      SizePct_set( const Point &src )  { d_size_pct = src; }
   }; // Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win

inline bool View::ActiveInWin() { return d_pWin->CurView() == this; }

//---------------------------------------------------------------------------------------------------------------------

struct   NamedPoint;
typedef  DLinkHead<NamedPoint>  NamedPointHead;

extern bool DeleteAllViewsOntoFbuf( PFBUF pFBuf ); // a very friendly (with FBUF) function

extern void MakeEmptyAllViewsOntoFbuf( PFBUF pFBuf );

extern bool g_fRealtabs;         // some inline code below references
extern char g_chTabDisp;         // some inline code below references
extern char g_chTrailSpaceDisp;  // some inline code below references

typedef bool (*ForFBufCallbackDone)( const FBUF &fbuf, void *pContext );
enum eEntabModes { ENTAB_0_NO_CONV, ENTAB_1_LEADING_SPCS_TO_TABS, ENTAB_2_SPCS_NOTIN_QUOTES_TO_TABS, ENTAB_3_ALL_SPC_TO_TABS, MAX_ENTAB_INVALID };
enum eBlankDispSrcs { BlankDispSrc_DIRTY=BIT(0), BlankDispSrc_SEL=BIT(1), BlankDispSrc_ALL_ALWAYS=BIT(2), BlankDispSrc_USER_ALWAYS=BIT(3), MAX_BlankDispSrc_INVALID=BIT(4) };

enum cppc
   { cppcNone=0
   , cppcIf     // , cppcIf_known_true  , cppcIf_known_false
   , cppcElif   // , cppcElif_known_true, cppcElif_known_false
   , cppcElse
   , cppcEnd
   };

#define   FBUF_TREE   0
#if       FBUF_TREE
   extern void FBufIdxInit();
   extern RbTree * g_FBufIdx;
   STIL   PFBUF IdxNodeToFBUF( RbNode *pNd ) { return reinterpret_cast<PFBUF>( rb_val(pNd) ); }  // type-safe conversion function
#else
   extern FBufHead g_FBufHead;
#endif

enum bkupMode { bkup_USE_SWITCH, bkup_UNDEL, bkup_BAK, bkup_NONE };

class InternalShellJobExecutor;

struct FileStat {
   FilesysTime    d_ModifyTime = 0;
   filesize_t     d_Filesize   = 0;
   int            d_mode       = 0;
   bool Refresh( int fd );
   bool none() const { return d_ModifyTime == 0 && d_Filesize == 0 && d_mode == 0; }
   };
STIL bool operator==(const FileStat& a, const FileStat& b) {
   return a.d_ModifyTime == b.d_ModifyTime
       && a.d_Filesize   == b.d_Filesize;
   }
STIL bool operator!=(const FileStat& a, const FileStat& b) {
   return !(a==b);
   }

STIL int cmp( const FileStat &a, const FileStat &b ) {
   if( a.d_ModifyTime > b.d_ModifyTime ) return +1;
   if( a.d_ModifyTime < b.d_ModifyTime ) return -1;
   if( a.d_Filesize   > b.d_Filesize   ) return +1;
   if( a.d_Filesize   < b.d_Filesize   ) return -1;
   return 0;
   }

enum Eol_t { EolLF, EolCRLF };
extern const Eol_t platform_eol;
extern PCChar EolName( Eol_t );

class FBUF { // FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF

public:
   //************ CONSTRUCTOR
                  FBUF( stref filename, PPFBUF ppGlobalPtr ); // the ONLY CTOR
   STATIC_FXN PFBUF AddFBuf( stref pBufferName, PFBUF *ppGlobalPtr=nullptr ); // and put at END of filelist

   //************ DESTRUCTORS
                  ~FBUF();
private:
   bool           private_RemovedFBuf();
public:
   PFBUF          ForciblyRemoveFBuf() {
                     if( !DeleteAllViewsOntoFbuf( this ) ) {
                        private_RemovedFBuf();
                        }
                     return nullptr;          // returns NULL always
                     }
   void           RemoveFBufOnly();  // ONLY USE THIS AT SHUTDOWN TIME, or within private_RemovedFBuf()!
   //************ FBUF name
private:
   Path::str_t    d_RsrcExt; // on heap
   Path::str_t    d_filename; // on heap
   bool           d_fFnmDiskWritable;
   void           ChangeName( stref newName );  // THE ONLY PLACE WHERE AN FBUF's NAME MAY BE SET!!!
public:
   bool           FnmIsDiskWritable() const { return d_fFnmDiskWritable; }
   PCChar         Name() const { return d_filename.c_str(); }
   const Path::str_t &Namestr() const { return d_filename; }
   stref              Namesr() const { return stref( d_filename.data(), d_filename.length() ); }
   Path::str_t    UserName() const;
   int            UserNameLen() const {
                     const auto len( d_filename.length() );
                     return Path::DelimChar( Name() ) ? len + 2 : len;
                     }
   bool           NameMatch( stref name ) const { return Path::eq( d_filename, name ); }
   STATIC_FXN bool FnmIsPseudo( PCChar name )   { return  name[0] == '<'; }
   bool            FnmIsPseudo()          const { return  FnmIsPseudo( Name() ); }
   stref           GetRsrcExt()           const { return  d_RsrcExt; }
   void            SetRsrcExt();
   //***********  membership in list of all FBUFs
 #if              FBUF_TREE
private:
   RbNode *         d_pRbNode;
 #else
   DLinkEntry<FBUF> dlinkAllFBufs;  // must be public
 #endif
public:
   PFBUF          Next() {
                     #if FBUF_TREE
                         RbNode *pNxtNd( rb_next( d_pRbNode ) );
                         return (pNxtNd == rb_nil(g_FBufIdx)) ? nullptr : PFBUF(rb_val( pNxtNd ));
                     #else
                         return dlinkAllFBufs.Next();
                     #endif
                         }
   /* d_ppGlobalPtr: support global pointer for this

      d_ppGlobalPtr allows direct (pointer) reference to a FBUF (that must
      obviously, while so referenced, remain in existence).  It is assumed that
      d_ppGlobalPtr points to a PFBUF having sufficient lifetime (i.e.  is static
      or (rarely) heap-based), such that d_ppGlobalPtr is always dereferencible.

      The alternative to d_ppGlobalPtr is to perform FBUF-name-based lookups of
      said FBUF at each point of reference, which is code- and
      runtime-inefficient.  Also this approach provides no guarantee that said
      FBUF will exist continuously between such references, and does not handle
      the case where the user decides to rename the FBUF (for example in order to
      save its contents to disk).

      The methods below provide a framework that fulfills the requirements
      outlined above.
   */
private:
   PPFBUF         d_ppGlobalPtr; // init'd by the "only ctor"; nullptr or addr-of a global ptr which points at this object
   PPFBUF         GetGlobalPtr() const { return d_ppGlobalPtr; }
public:
   void           UnsetGlobalPtr() {
                     if( d_ppGlobalPtr ) {
                       *d_ppGlobalPtr = nullptr;          // un-cross-link any existing reference
                        d_ppGlobalPtr = nullptr;          // new reference: none
                        }
                     }
   void           SetGlobalPtr( PPFBUF ppGlobalPtr ) { // Assert( nullptr==d_ppGlobalPtr );
                     d_ppGlobalPtr = ppGlobalPtr;                    // new reference: add
                     if( d_ppGlobalPtr ) { *d_ppGlobalPtr = this; }  // cross-link
                     }
   bool           HasGlobalPtr() const { return ToBOOL(d_ppGlobalPtr); }
   bool           IsSysPseudo()  const { return HasGlobalPtr(); }
   //***********  (list of) Views of this (FBUF)
private:
   ViewHead       d_dhdViewsOfFBUF;
public:
   PView          PutFocusOn();
   void           LinkView( PView pv );
   void           UnlinkView( PView pv );
   int            ViewCount() const;
   //************ affect all Views of FBUF
   bool           UnlinkAllViews();
private:
   void           MoveCursorAllViews( LINE yLine, COL xCol );
public:
   void           MoveCursorToBofAllViews()  { MoveCursorAllViews( 0, 0 ); }
   void           MoveCursorToEofAllViews()  { MoveCursorAllViews( LastLine()+1, 0 ); }
   //***********  OrigFileImage
private:
   // 20160425 note from a failed attempt (learning experience) to switch d_pOrigFileImage to std::string
   //    there seems to be NO WAY to read the entire content of a file into a std::string
   //    without FIRST writing each string memloc with an initializer value.  Using the phrasing
   //       str.ctor(), str.reserve( filebytes ), read( &str[0] ), str[filebytes] = '\0'
   //    invokes undefined behavior (though it "seems to work" today).  To avoid undefined
   //    behavior, replace reserve with resize (but this forces the undesired writing each string
   //    memloc with an initializer value).
   //    https://www.reddit.com/r/learnprogramming/comments/3qotqr/how_can_i_read_an_entire_text_file_into_a_string/
   PCChar         d_pOrigFileImage = nullptr;
   size_t         d_cbOrigFileImage = 0;
   enum text_encode_t { TXTENC_ASCII=0, TXTENC_UTF8=1 };
   text_encode_t  d_OrigFileImageContentEncoding = TXTENC_ASCII;
   LINE           d_naLineInfoElements = 0;
   LineInfo      *d_paLineInfo = nullptr;       // array of LineInfo, has d_naLineInfoElements elements alloc'd, d_LineCount used
   LINE           d_LineCount = 0;
public:
   bool           HasLines()                 const { return ToBOOL(d_paLineInfo); }
   LINE           LineCount()                const { return d_LineCount; }
   bool           KnownLine( LINE lineNum )  const { return lineNum >= 0 && lineNum < d_LineCount; }
   LINE           LastLine()                 const { return d_LineCount - 1; }
   COL            LineLength( LINE lineNum ) const { return d_paLineInfo[lineNum].d_iLineLen; }
   bool           PtrWithinOrigFileImage( PCChar pc ) const { return pc >= d_pOrigFileImage && pc < (d_pOrigFileImage + d_cbOrigFileImage); }
   filesize_t     cbOrigFileImage() const { return d_cbOrigFileImage; }
   //************ ImgBuf manipulators (currently used only in CGrepper::WriteOutput)
private:
   int            d_ImgBufBytesWritten = 0;
   LineInfo      &ImgBufNextLineInfo();
public:
   void           ImgBufAlloc(      size_t bufBytes, int PreallocLines=400 );
   void           ImgBufAppendLine( stref src );
   void           ImgBufAppendLine( PFBUF pFBufSrc, int srcLineNum, PCChar prefix=nullptr );
   //************ Undo/Redo storage
   friend class   EditRec;
   friend class   EdOpBoundary;
   friend class   EdOpSaveLineContent;
   friend class   EdOpLineRangeInsert;
   friend class   EdOpLineRangeDelete;
private:
   int            d_UndoCount;
   EditRec       *d_pNewestEdit;  // most recent edit action
   EditRec       *d_pCurrentEdit; // "current" editlist node
   EditRec       *d_pOldestEdit;  // least recent edit action
   void           InitUndoInfo(); // CTOR for 4 above private vars via EdOpBoundary::EdOpBoundary( PFBUF pFBuf, int only ) and EditRec::EditRec( PFBUF pFBuf, int only_placeholder )
   void           AddNewEditOpToListHead( EditRec *pEr );
   //************ Undo/Redo intf used by ARG::eds()
public:
   int            UndoCount()   const { return d_UndoCount   ; }
   EditRec       *CurrentEdit() const { return d_pCurrentEdit; }
   EditRec       *OldestEdit()  const { return d_pOldestEdit ; }
   EditRec       *NewestEdit()  const { return d_pNewestEdit ; }
   int            GetUndoCounts( int *pBRTowardUndo, int *pARTowardUndo, int *pBRTowardRedo, int *pARTowardRedo ) const;
   //************ user-level undo/redo impl
private:          // called by DoUserUndoOrRedo
   bool           EditOpUndo();
   bool           EditOpRedo();
   bool           AnythingToUndoRedo( bool fChkRedo ) const;
   bool           RmvOneEdOp_fNextIsBoundary( bool fFromListHead );
   void           SetUndoStateToBoundrec();
public:
   bool           DoUserUndoOrRedo( bool fRedo ); // called by ARG::undo() & ARG::redo()
   //************ Undo/Redo FBUF edit API
private:
   void           UndoReplaceLineContent(  LINE lineNum  , stref newContent );
   void           UndoSaveLineRange(       LINE firstLine, LINE lastLine );
   void           UndoInsertLineRangeHole( LINE firstLine, int lineCount );
   void           InsertLines__( LINE firstLine, LINE lineCount, bool fSaveUndoInfo );
   void           DeleteLines__( LINE firstLine, LINE lastLine , bool fSaveUndoInfo );
   void           DeleteLines__ForUndoRedo( LINE firstLine, LINE lastLine  )  { DeleteLines__( firstLine, lastLine , false ); }
   void           InsertLines__ForUndoRedo( LINE firstLine, LINE lineCount )  { InsertLines__( firstLine, lineCount, false ); }
   //************ Undo/Redo user-level-command execution boundary insertion
public:
   void           UndoInsertCmdAnnotation( PCCMD Cmd ); // used by buildexecute
   void           PutUndoBoundary();
   //************ command-level FBUF-content cleanup
private:
   void           DiscardUndoInfo();
public:
   void           FreeLinesAndUndoInfo();
   void           MakeEmpty();
   void           ClearUndo();
   //***********  yChangedMin
private:
   unsigned       d_contentRevision = 1; // used to passively detect FBUF content change; each change incrs this value; init value 1 allows others who maintain copied to init these to 0
   LINE           d_yChangedMin = -1; // Views displaying lines >= d_yChangedMin must update from d_yChangedMin to end of display
   void           Set_yChangedMin( LINE yChangedMin ) { Min( &d_yChangedMin, yChangedMin ); ++d_contentRevision; }
public:
   void           Push_yChangedMin();
   unsigned       CurrContentRevision() const { return d_contentRevision; }
   //************ FLAGS FLAGS FLAGS
   //************ FLAGS FLAGS FLAGS
   //************ FLAGS FLAGS FLAGS
private:
   bool           d_fDirty = false; // file state (content) is different from (last saved) disk state
                                    // ****** d_fDirty is saved in EdOpBoundary, restored on undo/redo ******
 #ifdef           fn_su
   bool           d_fSilentUpdateMode = false ;
 #endif
   bool           d_fAutoRead = false  ; // re-read every time file is fSetfile'd (gen'ly only useful for pseudofiles)
   Eol_t          d_EolMode = platform_eol;
#if defined(_WIN32)
   bool           d_fDiskRdOnly = false ; // file on disk is read only
#endif
   bool           d_fInhibitRsrcLoad = false ;
                                         // DO NOT call RsrcFileLdAllNamedSections in FBOP::CurFBuf_AssignMacros_RsrcLd().
                                         // Why?  Some pseudofiles exist to show the user the current state
                                         // of internals that can be perturbed by loading a rsrc-file
                                         // section.  This flag prevents that, so such pseudofiles give a
                                         // 100% accurate picture of the editor state prior to switching to
                                         // that pseudofile.  Initial use includes <CMD-SWI-Keys> and <macdefs>
   bool           d_fForgetOnExit = false ; // file state will not be written to statefile on editor close or saveall
   bool           d_fSavedToStateFileYet = false ; // used by statefile writer code only!
   bool           d_fNoEdit = false    ; // file may not be edited
   bool           d_fPreserveTrailSpc  ;
   bool           d_fTabSettingsFrozen = false ; // file's tab settings should not be auto-changed
   int8_t         d_TabWidth = MIN_TAB_WIDTH;
   BoolOneShot    d_fNeverReceivedFocus;
   eEntabModes    d_Entab = ENTAB_0_NO_CONV;
   int            d_BlankAnnoDispSrcAsserted = BlankDispSrc_ALL_ALWAYS;
   bool           d_fRevealBlanks = true;
   std::string    d_ftype; // since d_ftype is now mostly a content-based auto-detected property (therefore of the file itself), it is a FBUF (not View) property
   const FTypeSetting *d_ftypeStruct = nullptr;
public:
   const std::string &FType()            const { return  d_ftype; }
   bool           FTypeEmpty()           const { return  d_ftype.empty(); }
   bool           FTypeEq( stref ft )    const { return  eqi( d_ftype, ft ); } // case-insensitive!
   void           SetFType( stref ft );
   const FTypeSetting *GetFTypeSettings() const { return d_ftypeStruct; }
 #ifdef           fn_su
   bool           SilentUpdateMode()     const { return  d_fSilentUpdateMode; }
   void           SetSilentUpdateMode()               {  d_fSilentUpdateMode = true; }
 #endif
   bool           IsDirty()              const { return  d_fDirty; }
   void           UnDirty()                           {  // public since used by many ARG:: methods
                                                         SetDirty( false );
                                                      }
   bool           BlankAnnoDispSrcAsserted( int cause ) const { return ToBOOL( d_BlankAnnoDispSrcAsserted & cause ); }
   void           BlankAnnoDispSrcEdge( int cause, bool fReveal );
   bool           RevealBlanks() const { return d_fRevealBlanks; }
private:
   void           SetDirty( bool fDirty=true );
public:
   void           SetAutoRead()                       {  d_fAutoRead = true; }
   bool           IsAutoRead()           const { return  d_fAutoRead; }
#if defined(_WIN32)
   // in Linux we adopt a policy of "we'll determine whether the file is writable or not only when we attempt to write it"
public:
   bool           IsDiskRO()             const { return  d_fDiskRdOnly; }
private:
   void           SetDiskRW()                         {  d_fDiskRdOnly = false; }
   void           SetDiskRO()                         {  d_fDiskRdOnly = true; }
#endif
public:
   bool           IsRsrcLdBlocked()      const { return  d_fInhibitRsrcLoad; }
   void           SetBlockRsrcLd()                    {  d_fInhibitRsrcLoad = true; }
public:
   Eol_t          EolMode()              const { return  d_EolMode; }
   PCChar         EolName()              const { return ::EolName( d_EolMode ); }
   bool           SetEolModeChanged( Eol_t newval ) { const bool rv( newval != d_EolMode );
                                                      d_EolMode = newval;
                                                      return rv;
                                                      }
   bool           ToForgetOnExit()       const { return  d_fForgetOnExit; }
   void           SetForgetOnExit()                   {  d_fForgetOnExit = true; }
   void           SetRememberAfterExit()              {  d_fForgetOnExit = false; }
   bool           IsSavedToStateFile()   const { return  d_fSavedToStateFileYet; }
   void           SetSavedToStateFile()               {  d_fSavedToStateFileYet = true; }
   void           NotSavedToStateFile()               {  d_fSavedToStateFileYet = false; }
   bool           IsNoEdit()             const { return  d_fNoEdit; }
   void           ClrNoEdit()                         {  d_fNoEdit = false; }
   void           SetNoEdit()                         {  d_fNoEdit = true;  }
   void           ToglNoEdit()                        {  d_fNoEdit = !d_fNoEdit; }
   bool           TrailSpcsKept()        const { return  d_fPreserveTrailSpc; }
   void           DiscardTrailSpcs()                  {  d_fPreserveTrailSpc = false; }
   void           KeepTrailSpcs()                     {  d_fPreserveTrailSpc = true;  }
#if defined(_WIN32)
   bool           MakeDiskFileWritable();
#endif
   bool           CantModify() const;
   eEntabModes    Entab() const { return d_Entab; }
   bool           SetEntabOk( int newEntab ); // NOT CONST!!!
   char           TabDispChar()   const { return g_chTabDisp; }
   char           TrailDispChar() const { return g_chTrailSpaceDisp; }
   //************ tab-width-dependent line-content-related calcs
   int            TabWidth() const { return d_TabWidth; }
   #define SetTabWidth( newTabWidth )  SetTabWidth_( newTabWidth, __func__ )
   void           SetTabWidth_( COL newTabWidth, PCChar funcnm_ ); // NOT CONST!!!
   void           FreezeTabSettings()       { d_fTabSettingsFrozen = true; }
   bool           TabSettingsFrozen() const { return d_fTabSettingsFrozen; }
   //************ file-type checkers
public:
   bool           IsFileInfoFile(    int widx ) const;
   bool           IsInvisibleFile(   int widx ) const;
   bool           IsInterestingFile( int widx ) const;
   //************ Diskfile READ
private:
   FileStat       d_LastFileStat;
   void           SetLastFileStatFromDisk();
   enum DiskFileVsFbufStatus { DISKFILE_NO_EXIST, DISKFILE_NEWERTHAN_FBUF, DISKFILE_SAME_AS_FBUF, DISKFILE_OLDERTHAN_FBUF };
   DiskFileVsFbufStatus checkDiskFileStatus() const;
   PView          PutFocusOnView();
   bool           FBufReadOk_( bool fAllowDiskFileCreate, bool fCreateSilently ); // called ONLY by FBufReadOk!
   bool           FBufReadOk( bool fAllowDiskFileCreate, bool fCreateSilently );
   bool           ReadDiskFileFailed( int hFile );
   bool           UpdateFromDisk( bool fPromptBeforeRefreshing );
public:
   FileStat       GetLastFileStat() const { return d_LastFileStat; }
   bool           ReadDiskFileAllowCreateFailed( bool fCreateSilently=false ) { return !FBufReadOk( true , fCreateSilently ); }
   bool           ReadDiskFileNoCreateFailed   ()                             { return !FBufReadOk( false, false           ); }
   bool           ReadOtherDiskFileNoCreateFailed( PCChar pszName );
   bool           RefreshFailed();
   bool           RefreshFailedShowError();
   int            SyncNoWrite();
   int            SyncWriteIfDirty_Wrote();
   //************ (Internal) LineCount & LineInfo management
private:
   void           SetLineCount( LINE newlc ) { d_LineCount  = newlc; }
   void           IncLineCount( LINE delta ) { d_LineCount += delta; }
   void           InitLineInfoRange( LINE yMin, LINE numToInit ) {
                     auto pLi( d_paLineInfo + yMin );
                     for( const auto pLiPastEnd( pLi + numToInit ) ; pLi < pLiPastEnd ; ++pLi )
                        pLi->clear();
                     }
   void           SetLineInfoCount( LINE linesNeeded );
   //------------------------------------------------------------------
   void           FBufEvent_LineInsDel( LINE yLine, LINE lineDelta );  // negative lineDelta value signifies deletion of lines
   //------------------------------------------------------------------
   //************ Diskfile WRITE
private:
   int            d_backupMode = bkup_USE_SWITCH;
   time_t         d_tmLastWrToDisk = 0; // http://en.wikipedia.org/wiki/Year_2038_problem
   bool           write_to_disk( PCChar DestFileNm );
public:
   bool           WriteToDisk( PCChar pszSavename=nullptr );
   time_t         TmLastWrToDisk() const { return d_tmLastWrToDisk; }
   void           Set_TmLastWrToDisk( time_t viewVal ) { if( viewVal > d_tmLastWrToDisk ) { d_tmLastWrToDisk = viewVal; } }
   void           Reset_TmLastWrToDisk() { d_tmLastWrToDisk = 0; }
   void           SetBackupMode( int backupMode ) { d_backupMode = backupMode; }
   bool           SaveToDiskByName( PCChar pszNewName, bool fNeedUserConfirmation );
   //************ Marks
public:
   NamedPointHead d_MarkHead; // needs to be public since Mark subsys manages it directly
   bool           FindMark( PCChar pszMarkname, FBufLocn *pFL );
   void           DestroyMarks();
   //************ GetLine
private:
   COL            getLine_( std::string &dest, LINE yLine, int chExpandTabs=0 ) const;
public:
               // !!!use PeekRawLine to access line content whenever possible!!!
               // you can just use PeekRawLine() and stref methods + helper functions
               // in my_strutils.h and the tabWidth-dependent col-of-ptr/ptr-of-col xlators
               // to reference/parse line content w/o copying (MUCH more efficient to
               // avoid a heap alloc).  SEE ALSO: PeekRawLineSeg()
   stref          PeekRawLine( LINE yLine ) const { // returns RAW line content BY REFERENCE
                     auto len( 0 );
                     if( KnownLine( yLine ) && (len = LineLength( yLine )) > 0 ) {
                        return stref( d_paLineInfo[ yLine ].GetLineRdOnly(), len );
                        }
                     else {
                        return stref();
                        }
                     }
               // most of the time, you can just use PeekRawLine() and stref methods + helper functions
               // in my_strutils.h and the tabWidth-dependent col-of-ptr/ptr-of-col xlators
               // to reference/parse line content w/o copying (MUCH more efficient to
               // avoid a heap alloc).  However there are times when an actual copy is needed, so...
               // (NOTE that this the dup'd string is RAW, you still have to handle tabx yourself
               // if necessary)
   void           DupRawLine( std::string &dest, LINE yLine ) const {
                     const auto rv( PeekRawLine( yLine ) );
                     dest.assign( rv.data(), rv.length() );
                     }
   COL            getLineTabxPerRealtabs( std::string &dest, LINE yLine ) const { return getLine_( dest, yLine, g_fRealtabs ?0:' ' ); }
               // DupLineLua should only be used by Lua (I haven't yet figured out how to get the
               // tab-handling niceties available here (C++ ed_core.h) into Lua, or whether it would be
               // worth the hassle/headache.  In the meantime for Lua coding only, tabs are ALWAYS
               // translated to spaces).
   COL            DupLineLua( std::string &dest, LINE yLine ) const { return getLine_( dest, yLine, ' ' ); }
   stref          PeekRawLineSeg(                LINE yLine, COL xMinIncl, COL xMaxIncl=COL_MAX ) const; // returns RAW line content BY REFERENCE
   void           DupLineSeg( std::string &dest, LINE yLine, COL xMinIncl, COL xMaxIncl ) const;
   int            DupLineForInsert     (  std::string &dest, LINE yLine, COL xIns , COL insertCols ) const;
   int            GetLineIsolateFilename( Path::str_t &st, LINE yLine, COL xCol ) const; // -1=yLine does not exist, 0=no token found, 1=token found
   //************ PutLine
public:
               // meat-and-potatoes PutLine functions; note surfacing of tmpbuf(s) for efficiency
   void           PutLine( LINE yLine, stref srSrc, std::string &tmpbuf ); // WITH UNDO
   void           PutLastLine(         stref srSrc, std::string &tmpbuf ) { PutLine( 1+LastLine(), srSrc, tmpbuf ); }
   void           PutLineSeg( LINE yLine, const stref &ins, std::string &tmpbuf0, std::string &tmpbuf1, COL xLeftIncl=0, COL xRightIncl=COL_MAX, bool fInsert=false );
   void           PutLine( LINE yLine, const std::vector<stref> &vsrSrc, std::string &tmpbuf0, std::string &tmpbuf1 );
   void           PutLastLine(         const std::vector<stref> &vsrSrc, std::string &tmpbuf0, std::string &tmpbuf1 ) { PutLine( 1+LastLine(), vsrSrc, tmpbuf0, tmpbuf1 ); }
               // _oddball_ PutLine... functions; may soon be deprecated; use sparingly!
   void           PutLine( LINE yLine, CPCChar pa[], int elems );
   int            PutLastMultiline( PCChar pszNewLineData );
   void           PutLastLine( PCChar pszNewLineData )   { PutLastMultiline( pszNewLineData ); }
   void           PutLastLine( CPCChar pa[], int elems ) { PutLine( 1+LastLine(), pa, elems ); }
   void           InsBlankLinesBefore( LINE firstLine, LINE lineCount=1 )     { InsertLines__( firstLine, lineCount, true  ); }
   void           InsLine( LINE yLine, const stref &srSrc, std::string &tmp )  // WITH UNDO
                     {
                     InsBlankLinesBefore( yLine );
                     PutLine( yLine, srSrc, tmp );
                     }
   void           cat( PCChar pszNewLineData );
private:
   void           xvsprintf( PXbuf pxb, LINE lineNum, PCChar format, va_list val );
   void           Vsprintf( LINE lineNum, PCChar format, va_list val );
               // these 2 are UNREF'D?:
   void           FmtLine( LINE lineNum, PCChar format, ... ) ATTR_FORMAT(3, 4) ;
   void          xFmtLine( PXbuf pxb, LINE lineNum, PCChar format, ...  ) ATTR_FORMAT(4, 5) ;
   void          vFmtLastLine( PCChar format, va_list val );
   void         xvFmtLastLine( PXbuf pxb, PCChar format, va_list val );
public:
   void          xFmtLastLine( PXbuf pxb, PCChar format, ...  ) ATTR_FORMAT(3, 4) ;
   void           FmtLastLine( PCChar format, ... ) ATTR_FORMAT(2, 3) ;
   //************ delete LINE/BOX/STREAM
public:
   void           DelLine            ( LINE firstLine                   ) { DeleteLines__( firstLine, firstLine, true  ); }
   void           DelLines           ( LINE firstLine, LINE lastLine    ) { DeleteLines__( firstLine, lastLine , true  ); }
   void           DelBox( COL xLeft, LINE yTop, COL xRight, LINE yBottom, bool fCollapse=true );
   void           DelStream( COL xStart, LINE yStart, COL xEnd, LINE yEnd );
private:
   void           DirtyFBufAndDisplay();
   //************ indent infrastructure
private:
   int            d_IndentIncrement = 0;
public:
   void           CalcIndent( bool fWholeFileScan=false );
   int            IndentIncrement() const { return d_IndentIncrement; }
   //************ misc junk
public:
   int            DbgCheck();
   InternalShellJobExecutor *d_pInternalShellJobExecutor = nullptr;
   }; // end of class FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF FBUF

STIL PFBUF AddFBuf( stref pBufferName, PFBUF *ppGlobalPtr=nullptr ) {
   return FBUF::AddFBuf( pBufferName, ppGlobalPtr );
   }

inline bool LineInfo::fCanFree_pLineData( const FBUF &fbuf ) const { return !fbuf.PtrWithinOrigFileImage( d_pLineData ); }

inline bool View::LineCompileOk() const { return d_LineCompile_isValid && d_LineCompile >= 0 && d_LineCompile < CFBuf()->LineCount(); }

//************ Format for display
extern void        FormatExpandedSeg // more efficient version: recycles (but clear()s) dest, should hit the heap less frequently
          ( std::string &dest, size_t maxCharsToWrite  // dest-related
          , stref src, COL src_xMin, COL tabWidth, char chTabExpand=' ', char chTrailSpcs=0
          );
extern std::string FormatExpandedSeg // less efficient version: uses virgin dest each call, thus hits the heap each time (USE RARELY!!!)
          ( size_t maxCharsToWrite                     // dest-related
          , stref src, COL src_xMin, COL tabWidth, char chTabExpand=' ', char chTrailSpcs=0
          );

extern void    PrettifyMemcpy( std::string &dest, COL xLeft, size_t maxCharsToWrite, stref src, COL src_xMin, COL tabWidth, char chTabExpand, char chTrailSpcs );

//************ tabWidth-dependent string fxns
//---          FreeIdxOfCol returns index that MAY be > max indexable char in content; used to see whether col maps to content, or is beyond it, and for cursor movement
extern sridx   FreeIdxOfCol ( COL tabWidth, stref content, COL   colTgt );
extern COL     ColOfFreeIdx ( COL tabWidth, stref content, sridx offset );
//---          tabWidth-dependent col-of-ptr/ptr-of-col xlators
STIL   COL     ColOfNextChar( COL tabWidth, stref rl, COL xCol ) {
                  return ColOfFreeIdx( tabWidth, rl, FreeIdxOfCol( tabWidth, rl, xCol ) + 1 );
                  }
STIL  COL      ColOfPrevChar( COL tabWidth, stref rl, COL xCol ) {
                  const auto ix( FreeIdxOfCol( tabWidth, rl, xCol ) );
                  return ix == 0 ? -1 : ColOfFreeIdx( tabWidth, rl, ix - 1 );
                  }
extern COL     ColPrevTabstop( COL tabWidth, COL xCol );
extern COL     ColNextTabstop( COL tabWidth, COL xCol );
STIL   COL     StrCols( COL tabWidth, const stref &src ) { return ColOfFreeIdx( tabWidth, src, src.length() ); }
extern char    CharAtCol     ( COL tabWidth, stref content, const COL colTgt ); // returns 0 if col is not present in content
//             CaptiveIdxOfCol content[CaptiveIdxOfCol(colTgt)] is always valid; colTgt values which
//                             map beyond the last char of content elicit the index of the last char
STIL   sridx   CaptiveIdxOfCol ( COL tabWidth, stref content, const COL colTgt ) {
                  return Min( FreeIdxOfCol( tabWidth, content, colTgt ), content.length()-1 );
                  }
STIL   COL     TabAlignedCol( COL tabWidth, stref rl, COL xCol ) {
                  return ColOfFreeIdx( tabWidth, rl, FreeIdxOfCol( tabWidth, rl, xCol ) );
                  }
struct rlc1 {
   stref ln;
   sridx ix0;
   rlc1( PFBUF pfb, LINE yy, COL x0 )
      : ln( pfb->PeekRawLine( yy ) )
      , ix0( CaptiveIdxOfCol( pfb->TabWidth(), ln, x0 ) )
      {}
   bool beyond()           const { return ix0 >= ln.length(); }
   bool beyond( sridx ix ) const { return ix  >= ln.length(); }
   char ch0()              const { return ln[ix0]; }
   };
struct rlc2 {
   stref ln;
   sridx ix0;
   sridx ix1;
   rlc2( PFBUF pfb, LINE yy, COL x0, COL x1 )    // middle() ASSUMES x0 <= x1 !!!
      : ln( pfb->PeekRawLine( yy ) )
      , ix0( CaptiveIdxOfCol( pfb->TabWidth(), ln, x0 ) )
      , ix1( CaptiveIdxOfCol( pfb->TabWidth(), ln, x1 ) )
      {}
   bool beyond()                                  const { return ix0 >= ln.length(); }
   bool beyond( sridx ix ) const { return ix  >= ln.length(); }
   stref middle() const { return ln.substr( ix0, ix1-ix0 ); }
   };
class IdxCol {
   const COL    d_tw;
   const stref  d_sr;
public:
   IdxCol( const COL tw, stref sr )
      : d_tw    ( tw )
      , d_sr    ( sr )
      {}
   COL    i2c ( sridx iC   ) const { return ColOfFreeIdx( d_tw, d_sr, iC ); }
   sridx  c2i ( COL   xCol ) const { return CaptiveIdxOfCol( d_tw, d_sr, xCol ); }
   COL    cols(            ) const { return StrCols( d_tw, d_sr ); }
   };

namespace FBOP { // FBUF Ops: ex-FBUF methods per Effective C++ 3e "Item 23: Prefer non-member non-friend functions to member functions."

   extern PFBUF   FindOrAddFBuf( stref filename, PFBUF *ppGlobalPtr=nullptr );

   extern void    PrimeRedrawLineRangeAllWin( PCFBUF fb, LINE yMin, LINE yMax );

   //************ ExpandWildcard FBUF
   extern int     ExpandWildcard(         PFBUF fb, PCChar pszWildcardString, const bool fSorted );
   STIL   int     ExpandWildcardSorted(   PFBUF fb, PCChar pszWildcardString ) { return ExpandWildcard( fb, pszWildcardString, true  );  }
   STIL   int     ExpandWildcardUnsorted( PFBUF fb, PCChar pszWildcardString ) { return ExpandWildcard( fb, pszWildcardString, false );  }

   //************ tab-width-dependent line-content-related calcs

   STIL COL       LineCols( PCFBUF fb, LINE yLine ) { return StrCols( fb->TabWidth(), fb->PeekRawLine( yLine ) ); }

   //************ copy LINE/BOX/STREAM file->file
   extern void    CopyLines(  PFBUF FBdest, LINE yDestStart    , PCFBUF FBsrc, LINE ySrcStart, LINE ySrcEnd );
   extern void    CopyBox(    PFBUF FBdest, COL xDst, LINE yDst, PCFBUF FBsrc, COL xSrcLeft, LINE ySrcTop, COL xSrcRight, LINE ySrcBottom );
   extern void    CopyStream( PFBUF FBdest, COL xDst, LINE yDst, PCFBUF FBsrc, COL xSrcStart, LINE ySrcStart, COL xSrcEnd, LINE ySrcEnd );

   extern void    CurFBuf_AssignMacros_RsrcLd(); // implicitly takes g_CurFBuf() as param

   //************ Insert Lines in sorted order
   //
   // will NOT insert a duplicate line
   //
   // NB: Anything that calls InsertLines__ (pretty much any function named
   //     /^InsLine/) needs to be aware of its cursor side-effect: if a line is
   //     inserted above (lower or same line # as the) cursor, the cursor is
   //     moved down (increased line #).  This implies that, in any pseudofile
   //     generator functions, you should not move the cursor to any particular
   //     place until AFTER you've inserted all lines.
   //
   extern void    InsLineSorted_(          PFBUF fb, std::string &tmp, bool descending, LINE ySkipLeading, const stref &src );
   STIL void      InsLineSortedAscending(  PFBUF fb, std::string &tmp, LINE ySkipLeading, const stref &src ) { InsLineSorted_( fb, tmp, false, ySkipLeading, src ); }
   STIL void      InsLineSortedDescending( PFBUF fb, std::string &tmp, LINE ySkipLeading, const stref &src ) { InsLineSorted_( fb, tmp, true , ySkipLeading, src ); }
 #ifdef           fn_csort
   extern void    SortLineRange( PFBUF fb, LINE yMin, LINE yMax, bool fAscending, bool fCase, COL xMin, COL xMax, bool fRmvDups=false );
 #endif
   extern void    LuaSortLineRange( PFBUF fb, LINE y0, LINE y1, COL xLeft, COL xRight, bool fCaseIgnored=true, bool fOrdrAscending=true, bool fDupsKeep=false );

   // remove line
   bool           PopFirstLine( std::string &st, PFBUF pFbuf );

   //************ char-level ops
   extern COL     PutChar_( PFBUF fb, LINE yLine, COL xCol, char theChar, bool fInsert, std::string &tmp1, std::string &tmp2 );
   STIL COL       InsertChar ( PFBUF fb, LINE yLine, COL xCol, char theChar, std::string &tmp1, std::string &tmp2 ) { return PutChar_( fb, yLine, xCol, theChar, true , tmp1, tmp2 ); }
   STIL COL       ReplaceChar( PFBUF fb, LINE yLine, COL xCol, char theChar, std::string &tmp1, std::string &tmp2 ) { return PutChar_( fb, yLine, xCol, theChar, false, tmp1, tmp2 ); }
   extern COL     DelChar( PFBUF fb, LINE yLine, COL xCol );

   //************ text-content scanners
   extern PCChar  IsGrepBuf( Path::str_t &dest, int *pGrepHdrLines, PCFBUF fb );
   extern cppc    IsCppConditional( PCFBUF fb, LINE yLine );

   extern bool    IsLineBlank( PCFBUF fb, LINE yLine );
   extern bool    IsBlank( PCFBUF fb );
   extern COL     MaxCommonLeadingBlanksInLinerange( PCFBUF fb, LINE yTop, LINE yBottom );

   //************ indent
   extern COL     GetSoftcrIndent( PFBUF fb );
   extern COL     GetSoftcrIndentLua( PFBUF fb, LINE yLine );
   } // namespace FBOP   namespace FBOP   namespace FBOP   namespace FBOP   namespace FBOP   namespace FBOP   namespace FBOP


//---------------------------------------------------------------------------------------------------------------------
//
// replace global variables with readonly function backed by "static" var which
// is written only in one module
//

struct TGlobalStructs  // in a struct for easier debugger access
   {
   std::vector<PWin> aWindow;
   int   ixCurrentWin;
   };

extern TGlobalStructs g__;

STIL size_t       g_iWindowCount() { return  g__.aWindow.size()       ; }
STIL size_t       g_CurWindowIdx() { return  g__.ixCurrentWin         ; }
STIL PCWin        g_Win( int ix )  { return  g__.aWindow[ ix ]        ; }
STIL PWin         g_WinWr( int ix ){ return  g__.aWindow[ ix ]        ; }
STIL PCWin        g_CurWin()       { return  g_Win( g__.ixCurrentWin ); }
STIL PWin         g_CurWinWr()     { return  g_WinWr( g__.ixCurrentWin ); }
STIL ViewHead    &g_CurViewHd()    { return *(&g_CurWinWr()->ViewHd)  ; } // directly dependent on s_CurWindowIdx
STIL PView        g_CurView()      { return  g_CurViewHd().front()  ; } // NOT CACHED since can change independent of s_CurWindowIdx changing

// *** I'm not totally sure I like these, but it beats repeating g_CurWin() and g_CurView() multiple times in the code

#define PCW        const auto pcw( g_CurWin()          )
#define PCWr       const auto pcw( g_CurWinWr()        )
#define PCV        const auto pcv( g_CurView()         )
#define PCWV  PCW; const auto pcv( pcw->ViewHd.front() )
#define PCWrV PCWr;const auto pcv( pcw->ViewHd.front() )

STIL const Point &g_Cursor()       { return  g_CurView()->Cursor(); } // NOT CACHED since can change independent of s_CurWindowIdx changing
STIL LINE         g_CursorLine()   { return  g_Cursor().lin       ; } // NOT CACHED since can change independent of s_CurWindowIdx changing
STIL COL          g_CursorCol()    { return  g_Cursor().col       ; } // NOT CACHED since can change independent of s_CurWindowIdx changing

// this extern decl _was_ inside each of g_CurFBuf() and g_UpdtCurFBuf()'s bodies,
// however this led to an optimization-codegen-only Assert in PutFocusOn() !!!  20150405
extern PFBUF s_curFBuf; // not literally static (s_), but s/b treated as such!
STIL PFBUF        g_CurFBuf()                { return s_curFBuf; }
STIL void         g_UpdtCurFBuf( PFBUF pfb ) {        s_curFBuf = pfb; }

#define PCF        const auto pcf( g_CurFBuf() )
#define PCFV  PCV; PCF

inline bool FBufLocn::InCurFBuf() const { return g_CurFBuf() == d_pFBuf; }
