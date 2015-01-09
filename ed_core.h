//
// Copyright 1991 - 2015 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

static_assert( CHAR_MAX > SCHAR_MAX, "char not unsigned" );

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
const LINE MAX_LINES = PTRDIFF_MAX;

#else

  // "the way we were (pre x64)"
  // according to https://news.ycombinator.com/item?id=7712328
  // use of 32-bit data in x64 is _preferred_ for multiple reasons
  // a. smaller data footprint leads to better dcache effects.
  // b. smaller opcodes to ref 32-bit data "due to the lack of REX
  //    prefixes in instructions" leads to better icache effects.
  // [So this will probably remain the status quo for as long as x64 is the dominant platform]

typedef int   COL ;     // column or position within line
typedef int   LINE;     // line number within file
   enum { MAX_LINES = INT_MAX };

#endif

typedef LINE *PLINE;

#include "dlink.h"

class FBUF;
typedef       FBUF * PFBUF;
typedef FBUF const * PCFBUF;
typedef      PFBUF * PPFBUF;


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
const COL COL_MAX  = INT_MAX-1; // -1 is a HACK to avoid integer overflow in cases like alcc->PutColor( xMin, xMax-xMin+1, COLOR::DIM ); where xMin==0 and xMax==COL_MAX
const COL LINELEN  = (512)+1  ;
const COL BUFBYTES = LINELEN+1;

typedef char linebuf[BUFBYTES];       // line buffer
typedef char dbllinebuf[2*BUFBYTES];  // double line buffer
typedef char pathbuf[_MAX_PATH+1];    // Pathname buffer

typedef  FixedCharArray<BUFBYTES>     Linebuf;

typedef  FmtStr<BUFBYTES>    SprintfBuf;
typedef  FmtStr<2*BUFBYTES>  Sprintf2xBuf;


//=============================================================================

#define  USE_CURSORLINE_HILITE       1
#define  USE_CURSORLINE_VERT_HILITE  1

//
// Editor color table indicies
//

enum { // these are COLOR CODES!
   fgBLK =  0x0,
   fgBLU =  0x1,
   fgGRN =  0x2,
   fgCYN =  0x3,
   fgRED =  0x4,
   fgPNK =  0x5,
   fgBRN =  0x6,
   fgDGR =  0x7,
   fgMGR =  0x8,
   fgLBL =  0x9,
   fgLGR =  0xA,
   fgLCY =  0xB,
   fgLRD =  0xC,
   fgLPK =  0xD,
   fgYEL =  0xE,
   fgWHT =  0xF,
   //------------
   bgBLK = (fgBLK<<4),
   bgBLU = (fgBLU<<4),
   bgGRN = (fgGRN<<4),
   bgCYN = (fgCYN<<4),
   bgRED = (fgRED<<4),
   bgPNK = (fgPNK<<4),
   bgBRN = (fgBRN<<4),
   bgDGR = (fgDGR<<4),
   bgMGR = (fgMGR<<4),
   bgLBL = (fgLBL<<4),
   bgLGR = (fgLGR<<4),
   bgLCY = (fgLCY<<4),
   bgLRD = (fgLRD<<4),
   bgLPK = (fgLPK<<4),
   bgYEL = (fgYEL<<4),
   bgWHT = (fgWHT<<4),

   //------------ masks
   FGmask=0x0F, BGmask=0xF0,
   FGhi  =0x08, BGhi  =0x80,
   FGbase=0x07, BGbase=0x70,
   };

namespace COLOR { // see color2Lua
   enum { // these are ARRAY _INDICES_!
      FG ,  // foreground (normal)
      HG ,  // highlighted region
      SEL,  // selection
      WUC,  // word-under-cursor hilight
      CXY,  // cursor line (and column) hilight
      CPH,  // cppc line hilight
      DIM,  // comment hiliting, etc.
      LIS,  // litStr
      VIEW_COLOR_COUNT,

      INF=VIEW_COLOR_COUNT,  // information
      STA,  // status line
      WND,  // window border
      ERRM,  // error message
      COLOR_COUNT // MUST BE LAST!
      }; // NO MORE THAN 16 ALLOWED!
   }

// CompileTimeAssert( COLOR::COLOR_COUNT <= 16 ); // all COLOR:: must fit into a nibble

extern U8 g_colorInfo     ; // INF
extern U8 g_colorStatus   ; // STA
extern U8 g_colorWndBorder; // WND
extern U8 g_colorError    ; // ERR

#define  HILITE_CPP_CONDITIONALS  1


#include "conio.h"

struct Point {   // file location
   LINE    lin;
   COL     col;

   Point() {} //lint !e1401
   Point( LINE yLine, COL xCol ) : lin(yLine), col(xCol) {}

   Point( const Point  &rhs, LINE yDelta=0, COL xDelta=0 ) : lin(rhs.lin + yDelta), col(rhs.col + xDelta) {} // COPY CTOR

   void Set( LINE yLine, COL xCol ) { lin = yLine, col = xCol; }
   bool isValid() const { return lin >= 0 && col >= 0; }

   void ScrollTo( COL xWidth=1 ) const;
   bool operator==( const Point &rhs ) const { return lin == rhs.lin && col == rhs.col; }
   bool operator!=( const Point &rhs ) const { return !(*this == rhs); }
   bool operator< ( const Point &rhs ) const { return lin < rhs.lin || (lin == rhs.lin && col < rhs.col); }
   bool operator> ( const Point &rhs ) const { return lin > rhs.lin || (lin == rhs.lin && col > rhs.col); }
   bool operator>=( const Point &rhs ) const { return !(*this < rhs); }
   bool operator<=( const Point &rhs ) const { return !(*this > rhs); }

   Point( const YX_t &src ) : lin( src.lin ), col( src.col) {}

   }; // HAS CTORS, so union canNOT HAS-A one of these

typedef Point *PPoint;

class FBufLocn { // dflt ctor leaves locn empty; must be Set() later
   PFBUF d_pFBuf;
   Point d_pt;
   COL   d_width;

public:

   FBufLocn() : d_pFBuf(nullptr) {}
   FBufLocn( PFBUF pFBuf, const Point &pt ) : d_pFBuf(pFBuf), d_pt(pt) {}
   void Set( PFBUF pFBuf, Point pt, COL width=1 ) { d_pFBuf=pFBuf, d_pt=pt, d_width=width; }

   bool ScrollToOk() const;

   bool         IsSet()     const { return d_pFBuf != nullptr; }
   bool         InCurFBuf() const;
   bool         Moved()     const;
   PFBUF        FBuf()      const { return d_pFBuf; }
   const Point &Pt()        const { return d_pt   ; }
   };

class FBufLocnNow : public FBufLocn // dflt ctor uses curfile, cursor; const instances work fine
   {
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
   int  LineNotWithin( LINE yLine ) const; // IGNORES EFFECT OF COLUMNS!
   };


class Xbuf {
   // ever-growing string class :-) which implements an ...
   // ever-growing line buffer intended to be used for all lines touched over
   // the duration of a command or operation, in lieu of malloc'ing a buffer for
   // each line touched.

   PChar                  d_buf;
   size_t                 d_buf_bytes;
   STATIC_VAR char        ds_empty;

   public:

   Xbuf()
      : d_buf      ( &ds_empty )
      , d_buf_bytes( 0 )
      {}

   Xbuf( size_t size )
      : d_buf      ( nullptr )
      , d_buf_bytes( 0 )
      { wresize( size ); }

   Xbuf( PCChar str )
      : d_buf      ( nullptr )
      , d_buf_bytes( 0 )
      { assign( str ); }

   Xbuf( PCChar str, size_t len_ )
      : d_buf      ( nullptr )
      , d_buf_bytes( 0 )
      { assign( str, len_ ); }

   ~Xbuf() {
          if( &ds_empty==d_buf ) {} else
          { Free_( d_buf ); }
          }

public:
   PChar    wbuf()      const { return d_buf;       }
   PCChar   c_str()     const { return d_buf;       }
   size_t   buf_bytes() const { return d_buf_bytes; }
   void     clear()           { poke( 0, '\0' );    }
   bool     is_clear()  const { return !(         d_buf[0]); }

   stref    bsr() { return stref( c_str(), length() ); }

   PChar wresize( size_t size ) {
      if( d_buf_bytes < size ) {
          d_buf_bytes = ROUNDUP_TO_NEXT_POWER2( size, 512 );
          if( &ds_empty==d_buf ) { d_buf = nullptr; }
          ReallocArray( d_buf, d_buf_bytes, __func__ );
         }
      return d_buf;
      }

   PCChar kresize( size_t size ) { return wresize( size ); }

   size_t length() {
      const auto pnul( PChar( memchr( d_buf, 0, d_buf_bytes ) ) );
      return pnul ? pnul - d_buf : 0;
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

   PCChar cpy2( PCChar str1, PCChar str2 ) {
      const auto len1_( Strlen(str1) );
      const auto len2_( Strlen(str2) );
      const auto rv( wresize( 1+len1_+len2_ ) );
      memcpy( rv      , str1, len1_ );
      memcpy( rv+len1_, str2, len2_ );
      rv[len1_+len2_] = '\0';
      return rv;
      }

   PCChar cat( PCChar str, size_t len_ ) {
      const auto len0( d_buf ? Strlen(d_buf) : 0 );
      const auto rv( wresize( 1+len0+len_ ) );
      memcpy( rv+len0, str, len_ );
      rv[len0+len_] = '\0';
      return rv;
      }

   PCChar cat( PCChar str ) {
      return cat( str, Strlen(str) );
      }

   PCChar push_back( char ch ) {
      const auto slen( length() );
      const auto rv( wresize( slen+2 ) );
      rv[slen] = ch;
      rv[slen+1] = '\0';
      return rv;
      }

   PCChar poke( int xCol, char ch, char fillch=' ' ) {
      const auto rv( wresize( 1+xCol ) );
      const auto len0( length() );
      if( xCol > len0 ) {
         memset( rv+len0, fillch, xCol-len0 );
         }
      rv[xCol] = ch;
      if( ch && xCol >= len0 ) {
         rv[xCol+1] = '\0';
         }
      return rv;
      }

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

   PChar d_pLineData;           // CAN be -1 when REPLACEREC is for line that didn't exist
   COL   d_iLineLen;

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

   void CheckDump( PCChar msg, PFBUF pFBuf, LINE yLine ) const;
   bool Check( PCChar msg, PFBUF pFBuf, LINE yLine ) const {
      if( !d_pLineData || d_iLineLen < 5000 )
         return false;

      CheckDump( msg, pFBuf, yLine );
      return true;
      }

   void   PutContent( PCChar pNewLine, int newLineBytes );
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

union CmdData {
   EdKC_Ascii eka;
   PChar      pszMacroDef;

   char  chAscii() const { return eka.Ascii; }

   int   isEmpty() const { return pszMacroDef == nullptr; }
   };


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
enum                      // Actually can be set in ARG::Abc?
   {                      // |
   NOARG      = BIT( 0),  // * no argument specified
     NOARGWUC = BIT( 1),  //   no arg => word under cursor (converts to TEXTARG, which must also be set)
   NULLARG    = BIT( 2),  // * arg + no cursor movement
   TEXTARG    = BIT( 3),  // * text specified (NULLEOL, NULLEOW and BOXSTR convert to this)
   LINEARG    = BIT( 4),  // * contiguous range of entire lines
   BOXARG     = BIT( 5),  // * box delimited by arg, cursor
   STREAMARG  = BIT( 6),  // * from low-to-high, viewed 1-D
     NULLEOL  = BIT( 7),  //   null arg => text from arg->eol (converts to TEXTARG, which must also be set)
     NULLEOW  = BIT( 8),  //   null arg => text from arg->end of word (converts to TEXTARG, which must also be set)
     BOXSTR   = BIT( 9),  //   single-line box selection converts to TEXTARG, which must also be set
   NUMARG     = BIT(10),  //   text => delta to y position (converts to LINEARG, which must also be set)
   MARKARG    = BIT(11),  //   text => mark at end of arg
   MODIFIES   = BIT(12),  //   modifies current file (if > granularity is needed, DON'T specify MODIFIES; call FBUF::CantModify() in code paths which MODIFY.
   KEEPMETA   = BIT(13),  //   do not consume meta flag
   WINDOWFUNC = BIT(14),  //   do not cancel highlight resulting from a previous command, such as psearch.
   CURSORFUNC = BIT(15),  //   do not recognize or cancel Arg prefix.  Conflicts with all flags except MODIFIES and KEEPMETA.
   MACROFUNC  = BIT(16),  //   nests/pushes literal text from which the command stream is generated.  Conflicts with all flags except KEEPMETA.

   // only ONE among ACTUAL_ARGS may be set in ARG::d_argType when a ARG::Xyz is Invoke()'ed
   ACTUAL_ARGS = NOARG | TEXTARG | NULLARG | LINEARG | BOXARG | STREAMARG,

   // CMD's w/ANY of these SET need additional ARG processing when Invoke()'ed
   TAKES_ARG   = ACTUAL_ARGS | NOARGWUC | NULLEOW | NULLEOL | NUMARG,
   };

// note that 'mark' fxn does NOT have NUMARG set!  I guess it uses atoul()?
#define IS_NUMARG    (d_argType == LINEARG)
#define NUMARG_VALUE (d_linearg.yMax - d_linearg.yMin + 1)


//
//  Argument defininition struct+union
//

struct CMD;
typedef       CMD *         PCMD;
typedef CMD const *        PCCMD;
typedef CMD const * const CPCCMD;

struct ARG {
   U32  d_argType;
   int  d_cArg;        // count of <arg>s pressed: 0 for NOARG

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

   bool  d_fMeta;
   PCCMD d_pCmd;
   PFBUF d_pFBuf;

   //*********  END OF DATA

   void  DelArgRegion() const;
   void  BeginPt( Point *pPt ) const;
   void  EndPt( Point *pPt ) const;
   bool  Within( const Point &pt, COL len=-1 ) const;
   bool  Beyond( const Point &pt ) const;
   COL   GetLine( std::string &st, LINE yLine ) const;
   void  ColsOfArgLine( LINE yLine, COL *pxLeftIncl, COL *pxRightIncl ) const;
   void  GetColumnRange( COL *pxMin, COL *pxMax ) const;
   int   GetLineRange( LINE *yTop, LINE *yBottom ) const;

private:

   bool  FillArgStructFailed();
   bool  BOXSTR_to_TEXTARG( LINE yOnly, COL xMin, COL xMax );

   bool  GenericReplace( bool fInteractive, bool fMultiFileReplace );
// bool  GenericSearch( const SearchScanMode &sm, FileSearchMatchHandler &mh, Point curPt );

public:

   bool   InitOk( PCCMD pCmd );
   void   SaveForRepeat() const;
   PCChar CmdName() const;
   bool   Invoke();
   bool   fnMsg( PCChar fmt, ... ) const ATTR_FORMAT(2, 3) ;
   bool   BadArg() const; // a specific error: boilerplate (but informative) err msg
   bool   ErrPause( PCChar fmt, ... ) const ATTR_FORMAT(2, 3) ;
   PCChar ArgTypeName() const;

   void   ConvertStreamargToLineargOrBoxarg();
   void   ConvertLineOrBoxArgToStreamArg();
   U32    ActualArgType() const { return d_argType & ACTUAL_ARGS; }

private:

   bool   pmlines( int direction ); // factored from mlines and plines

public:

   typedef bool (*BoxStringFunction)( PCChar pszLineSeg, LINE yAbs, LINE yRel, COL xCol );

   // EDITOR_FXNs start here!!!

   typedef bool (ARG::*pfxCMD)(); // declare type of pointer to class method

   bool nextmsg_engine( const bool fFwd );

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

   ArgLineWalker( CPCARG Arg )
      : d_Arg(Arg)
      {
      d_Arg->BeginPt( &d_pt );
      }

   bool   Beyond()              const { return d_Arg->Beyond( d_pt ); }
   bool   Within( COL len=-1 )  const { return d_Arg->Within( d_pt, len ); }
   LINE   Line()                const { return d_pt.lin; }
   LINE   Col ()                const { return d_pt.col; }
   COL    GetLine()                   { return d_Arg->GetLine( d_st, d_pt.lin ); }
   bool   NextLine()                  { d_pt.col = 0; ++d_pt.lin; return Beyond(); }
   void   buf_erase( size_t pos )     {        d_st.erase( pos ); }
   PCChar c_str()               const { return d_st.c_str(); }
   };


//
//  Function definition table definitions
//
typedef ARG::pfxCMD funcCmd;

STIL funcCmd fn_runmacro()  { return &ARG::RunMacro   ; }
STIL funcCmd fn_runLua()    { return &ARG::ExecLuaFxn ; }


#define AHELPSTRINGS  1
#if     AHELPSTRINGS
#   define  AHELP( x )   x
#   define _AHELP( x ) , x
#else
#   define  AHELP( x )
#   define _AHELP( x )
#endif//AHELPSTRINGS


struct CMD {             // function definition entry
   PCChar   d_name;      // - pointer to name of fcn     !!! DON'T CHANGE ORDER OF FIRST 2 ELEMENTS
   funcCmd  d_func;      // - pointer to function        !!! UNLESS you change initializer of macro_graphic
   U32      d_argType;   // - user args allowed
#if AHELPSTRINGS
   PCChar   d_HelpStr;   // - help string shown in <CMD-SWI-Keys>
#endif
   CmdData  d_argData;   // - NON-MACRO: key that invoked; MACRO: ptr to macro string (defn)
   mutable  U32  d_callCount;  // - how many times user has invoked (this session) since last cmdusage_updt() call
   mutable  U32  d_gCallCount; // - how many times user has invoked, global

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

   PCChar   Name()                  const { return PCChar( d_name ); }
   bool     NameMatch( PCChar str ) const { return Stricmp( str, d_name ) == 0; }

   PCChar   MacroText()             const { return PCChar( d_argData.pszMacroDef ); }

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


struct FileExtensionSetting;

class View { // View View View View View View View View View View View View View View View View View View View View View View View View
public:
   DLinkEntry<View> dlinkViewsOfWindow;
   DLinkEntry<View> dlinkViewsOfFBUF;

                View( const View & );
                View( const View &, PWin pWin );
                View( PFBUF pFBuf , PWin pWin, PCChar szViewOrdinates=nullptr );
                ~View();

   void         Write( FILE *fout ) const;

private:
                View() = delete; // NO dflt CTOR !

   const PWin   d_pWin;     // back pointer, needed cuz our owning Win knows things we don't, but need
   const PFBUF  d_pFBuf;    // back pointer
   ViewHiLites *d_pHiLites = nullptr; // we own this!

   void         CommonInit();

   time_t       d_tmFocusedOn = 0; // http://en.wikipedia.org/wiki/Year_2038_problem

public:
   void         PutFocusOn();

   time_t       TmFocusedOn() const { return d_tmFocusedOn; }

   PCFBUF       CFBuf()  const { return d_pFBuf; }
   PFBUF        FBuf()   const { return d_pFBuf; }
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
public:
   ULC_Cursor   d_saved;         // window state saved by ARG::savecur

   const Point  &Cursor() const { return d_current.Cursor; }
   const Point  &Origin() const { return d_current.Origin; }

   bool         RestCur();
   void         SaveCur()     { d_saved.Set( d_current ); }
   void         SavePrevCur() { d_prev .Set( d_current ); }
   void         ScrollToPrev();

   void         GetCursor( PPoint pPt ) const { *pPt = d_current.Cursor; }

private:
   void         MoveCursor_( LINE yLine, COL xColumn, COL xWidth, bool fUpdtWUC );
public:
   void         MoveCursor(           LINE yLine, COL xColumn, COL xWidth=1 )   { MoveCursor_( yLine, xColumn, xWidth, true  ); }
   void         MoveCursor_NoUpdtWUC( LINE yLine, COL xColumn, COL xWidth=1 )   { MoveCursor_( yLine, xColumn, xWidth, false ); }
   void         MoveCursor( const Point &pt )                                   { MoveCursor( pt.lin, pt.col ); }
   void         MoveAndCenterCursor( const Point &pt, COL width );
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
                   ( LINE                yLine
                   , COL                 xIndent
                   , COL                 xMax
                   , LineColorsClipped  &alcc
                   , const HiLiteRec  * &pFirstPossibleHiLite
                   , bool                isActiveWindow
                   , bool                isCursorLine
                   ) const;

   void         GetLineForDisplay
                   ( PChar              pTextBuf
                   , LineColorsClipped &alcc
                   , const HiLiteRec * &pFirstPossibleHiLite
                   , LINE               yLineOfFile
                   , bool               isActiveWindow
                   , COL                xWidth
                   ) const;

private:
   LINE         d_LineCompile = -1;
public:
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

   Point        d_LastSelectBegin, d_LastSelectEnd;

   bool         prev_balln( LINE yStart, bool fStopOnElse );
   bool         next_balln( LINE yStart, bool fStopOnElse );

private:
   FileExtensionSetting *d_pFES = nullptr;
public:
   FileExtensionSetting *GetFileExtensionSettings();

   int          ColorIdx2Attr( int colorIdx ) const;
   }; // View View View View View View View View View View View View View View View View View View View View View View View View


struct Win { // Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win
   ViewHead  ViewHd;   // public: exposed
   Point     d_Size;   // public: exposed
   Point     d_UpLeft; // public: exposed
   int       d_wnum;

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

public: // std pimpl implemenations declare it as private, but we have "special needs"
   class impl;
   std::unique_ptr<impl> pimpl;

   }; // Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win Win


inline bool View::ActiveInWin() { return d_pWin->CurView() == this; }


//---------------------------------------------------------------------------------------------------------------------
//
// FILE flags values
//
enum eFileType
   { ftype_UNKNOWN
   , ftype1_C
   };


struct   NamedPoint;
typedef  DLinkHead<NamedPoint>  NamedPointHead;


extern bool DeleteAllViewsOntoFbuf( PFBUF pFBuf ); // a very friendly (with FBUF) function

extern void MakeEmptyAllViewsOntoFbuf( PFBUF pFBuf );


extern bool g_fRealtabs;  // some inline code below references
extern char g_chTabDisp;  // some inline code below references
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

typedef U32         filesize_t;
typedef signed long FilesysTime;

struct FileStat {
   FilesysTime    d_ModifyTime = 0;
   filesize_t     d_Filesize   = 0;
   bool Refresh( int fd );
   bool none() const { return d_ModifyTime == 0 && d_Filesize == 0; }
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

STIL stref se2bsr( PCChar bos, PCChar eos ) { return stref( bos, eos - bos ); }
STIL stref se2bsr( const std::string &str ) { return stref( str ); }

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
                     if( !DeleteAllViewsOntoFbuf( this ) )
                        private_RemovedFBuf();
                     return nullptr;          // returns NULL always
                     }
   void           RemoveFBufOnly();  // ONLY USE THIS AT SHUTDOWN TIME, or within private_RemovedFBuf()!

   //************ FBUF name
private:
   Path::str_t    d_filename; // on heap
   bool           d_fFnmDiskWritable;
   void           ChangeName( stref newName );  // THE ONLY PLACE WHERE AN FBUF's NAME MAY BE SET!!!

public:
   bool           FnmIsDiskWritable() const { return d_fFnmDiskWritable; }
   PCChar         Name() const { return d_filename.c_str(); }
   const Path::str_t &Namestr() const { return d_filename; }

   char           UserNameDelimChar() const;
   PChar          UserName( PChar dest, size_t destSize ) const;
   int            UserNameLen() const {
                     const auto len( d_filename.length() );
                     return UserNameDelimChar() ? len + 2 : len;
                     }

   bool           NameMatch( stref name ) const { return Path::eq( d_filename, name ); }

   STATIC_FXN bool FnmIsPseudo( PCChar name )   { return  name[0] == '<'; }
   bool            FnmIsPseudo()          const { return  FnmIsPseudo( Name() ); }

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
   PPFBUF         GetGlobalPtr()   const { return d_ppGlobalPtr; }
public:

   void           UnsetGlobalPtr() {
                  if( d_ppGlobalPtr ) { *d_ppGlobalPtr = nullptr; }  // un-cross-link any existing reference
                  d_ppGlobalPtr = nullptr;                           // new reference: none
                  }

   void           SetGlobalPtr( PPFBUF ppGlobalPtr ) {
                  if( d_ppGlobalPtr ) { *d_ppGlobalPtr = nullptr; }  // un-cross-link any existing reference
                  d_ppGlobalPtr = ppGlobalPtr;                       // new reference: add
                  if( d_ppGlobalPtr ) { *d_ppGlobalPtr = this;    }  // cross-link
                  }

   bool           HasGlobalPtr()                 const { return ToBOOL(d_ppGlobalPtr); }
   bool           IsSysPseudo()                  const { return HasGlobalPtr(); }

   //***********  (list of) Views of this (FBUF)
private:
   ViewHead       d_dhdViewsOfFBUF;
public:
   PView          PutFocusOn();
   void           LinkView( PView pv );
   void           UnlinkView( PView pv );
   int            ViewCount();

   //************ affect all Views of FBUF
   bool           UnlinkAllViews();
private:
   void           MoveCursorAllViews( LINE yLine, COL xCol );
public:
   void           MoveCursorToBofAllViews()  { MoveCursorAllViews( 0, 0 ); }
   void           MoveCursorToEofAllViews()  { MoveCursorAllViews( LastLine()+1, 0 ); }

   //***********  OrigFileImage
private:
   PChar          d_pOrigFileImage = nullptr;
   size_t         d_cbOrigFileImage = 0;

   void           FreeOrigFileImage();

   LINE           d_naLineInfoElements = 0;
   LineInfo      *d_paLineInfo = nullptr;       // array of LineInfo, has d_naLineInfoElements elements alloc'd, d_LineCount used
   LINE           d_LineCount = 0;

public:
   bool           HasLines()                 const { return ToBOOL(d_paLineInfo); }
   LINE           LineCount()                const { return d_LineCount; }
   bool           KnownLine( LINE lineNum )  const { return lineNum >= 0 && lineNum < d_LineCount; }
   LINE           LastLine()                 const { return d_LineCount - 1; }
   COL            LineLength( LINE lineNum ) const { return d_paLineInfo[lineNum].d_iLineLen; }

   bool           PtrWithinOrigFileImage( PChar pc ) const { return pc >= d_pOrigFileImage && pc < (d_pOrigFileImage + d_cbOrigFileImage); }

   filesize_t     cbOrigFileImage() const { return d_cbOrigFileImage; }

   //************ ImgBuf manipulators (currently used only in CGrepper::WriteOutput)
private:
   int            d_ImgBufBytesWritten = 0;
   LineInfo      &ImgBufNextLineInfo();
public:
   void           ImgBufAlloc(      size_t bufBytes, int PreallocLines=400 );
   void           ImgBufAppendLine( PCChar pNewLineData, int LineLen );
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
   bool           DoUserUndoOrRedo( bool fRedo ); // API actually called ARG::undo() & ARG::redo()

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
   bool           d_fDiskRdOnly = false ; // file on disk is read only

   bool           d_fInhibitRsrcLoad = false ;
                                         // DO NOT call LoadFileExtRsrcIniSection in FBOP::AssignFromRsrc().
                                         // Why?  Some pseudofiles exist to show the user the current state
                                         // of internals that can be perturbed by loading a rsrc-file
                                         // section.  This flag prevents that, so such pseudofiles give a
                                         // 100% accurate picture of the editor state prior to switching to
                                         // that pseudofile.  Initial use includes <CMD-SWI-Keys> and <macdefs>

   bool           d_fForgetOnExit = false ; // file state will not be written to statefile on editor close or saveall
   bool           d_fSavedToStateFileYet = false ; // used by statefile writer code only!
   bool           d_fNoEdit = false    ; // file may not be edited
   bool           d_fPreserveTrailSpc  ;
   bool           d_fTabSettingsFrozen ; // file's tab settings should not be auto-changed

   S8             d_TabWidth;
   eEntabModes    d_Entab = ENTAB_0_NO_CONV;

   eFileType      d_FileType = ftype_UNKNOWN;   // enum FileType
   int            d_BlankAnnoDispSrcAsserted = BlankDispSrc_ALL_ALWAYS;
   bool           d_fRevealBlanks = true;

public:
   eFileType      FileType()             const { return  d_FileType; }
   void           SetFileType( eFileType eft )        {  d_FileType = eft; }

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

   char           TabDispChar()   const { return RevealBlanks() ? g_chTabDisp        : ' '; }
   char           TrailDispChar() const { return RevealBlanks() ? g_chTrailSpaceDisp : 0  ; }

   //************ tab-width-dependent line-content-related calcs
   int            TabWidth() const { return d_TabWidth; }
   void           SetTabWidthOk( COL NewTabWidth ); // NOT CONST!!!

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
   bool           write_to_disk( PCChar DestFileNm );

public:

   bool           WriteToDisk( PCChar pszSavename=nullptr );
   void           SetBackupMode( int backupMode ) { d_backupMode = backupMode; }

   bool           SaveToDiskByName( PCChar pszNewName, bool fNeedUserConfirmation );

   //************ Marks
public:
   NamedPointHead d_MarkHead; // needs to be public since Mark subsys manages it directly
   bool           FindMark( PCChar pszMarkname, FBufLocn *pFL );
   void           DestroyMarks();

   //************ GetLine
private:
   COL            getLine_(   std::string &dest, LINE yLine, int chExpandTabs=0 ) const;
public:
   void           getLineRaw( std::string &dest, LINE yLine ) const;

   COL            getLineTabx(            std::string &dest, LINE yLine ) const { return getLine_( dest, yLine, ' ' ); }
   COL            getLineTabxPerRealtabs( std::string &dest, LINE yLine ) const { return getLine_( dest, yLine, g_fRealtabs ?0:' ' ); }
   COL            getLineTabxPerTabDisp ( std::string &dest, LINE yLine ) const { return getLine_( dest, yLine, RevealBlanks()?0:' ' ); }

   void           GetLineSeg( std::string &dest, LINE yLine, COL xLeftIncl, COL xRightIncl ) const;  // <-- prefer
   int            GetLineForInsert     (  std::string &dest, LINE yLine, COL xIns , COL insertCols ) const;
   int            GetLineIsolateFilename( Path::str_t &st, LINE yLine, COL xCol ) const; // -1=yLine does not exist, 0=no token found, 1=token found

   stref          PeekRawLine( LINE lineNum ) const; // returns RAW line content BY REFERENCE
   stref          PeekRawLineSeg( LINE lineNum, COL xMinIncl, COL xMaxIncl=COL_MAX ) const; // returns RAW line content BY REFERENCE

   //************ PutLine
public:
   void           PutLine( LINE yLine, stref srSrc, std::string &tmp ); // WITH UNDO
   void           PutLine( LINE yLine, CPCChar pa[], int elems );

   int            PutLastMultiline( PCChar pszNewLineData );
   void           PutLastLine( PCChar pszNewLineData )   { PutLastMultiline( pszNewLineData ); }
   void           PutLastLine( CPCChar pa[], int elems ) { PutLine( LastLine()+1, pa, elems ); }

   void           InsBlankLinesBefore( LINE firstLine, LINE lineCount=1 )     { InsertLines__( firstLine, lineCount, true  ); }
   void           InsLine( LINE yLine, const stref &srSrc, std::string &tmp )  // WITH UNDO
                     {
                     InsBlankLinesBefore( yLine );
                     PutLine( yLine, srSrc, tmp );
                     }

   void           PutLineSeg( LINE lineNum, const stref &ins, std::string &tmp0, std::string &tmp1, COL xLeftIncl=0, COL xRightIncl=COL_MAX, bool fInsert=false );
   void           cat( PCChar pszNewLineData );

   void           xvsprintf( PXbuf pxb, LINE lineNum, PCChar format, va_list val );
   void           Vsprintf( LINE lineNum, PCChar format, va_list val );
   void          xFmtLine( PXbuf pxb, LINE lineNum, PCChar format, ...  ) ATTR_FORMAT(4, 5) ;
   void           FmtLine( LINE lineNum, PCChar format, ... ) ATTR_FORMAT(3, 4) ;
   void          xFmtLastLine( PXbuf pxb, PCChar format, ...  ) ATTR_FORMAT(3, 4) ;
   void           FmtLastLine( PCChar format, ... ) ATTR_FORMAT(2, 3) ;
   void         xvFmtLastLine( PXbuf pxb, PCChar format, va_list val );
   void          vFmtLastLine( PCChar format, va_list val );

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
   int            d_IndentIncrement = 3;
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

inline bool View::LineCompileOk() const { return d_LineCompile >= 0 && d_LineCompile < d_pFBuf->LineCount(); }


//************ tabWidth-dependent col-of-ptr/ptr-of-col xlators
// DEPRECATED pEos points AFTER last valid (non-NUL) char in pS; if pS were a standard C string, *pEos == 0, BUT pS MAY NOT BE a standard C string!
// DEPRECATED
// DEPRECATED if fKeepPtrWithinStringRegion then retval <= pEos  (NOTE THAT retval == pEos (and therefore can point at a non-deref'able locn)
extern PChar   PtrOfCol_                 ( COL tabWidth, PChar  pS, PChar  pEos, COL colTgt, bool fKeepPtrWithinStringRegion );
// DEPRECATED PtrOfColWithinStringRegion: retval <= pEos  (NOTE THAT retval == pEos (and therefore can be non-deref'able)
// DEPRECATED should TRY to stop using PtrOfColWithinStringRegion in lieu of PtrOfColWithinStringRegionNoEos
STIL   PChar   PtrOfColWithinStringRegion( COL tabWidth, PChar  pS, PChar  pEos, COL xCol ) { return PtrOfCol_( tabWidth,       pS ,       pEos , xCol, true  ); }
STIL   PCChar  PtrOfColWithinStringRegion( COL tabWidth, PCChar pS, PCChar pEos, COL xCol ) { return PtrOfCol_( tabWidth, PChar(pS), PChar(pEos), xCol, true  ); }
// DEPRECATED PtrOfColWithinStringRegionNoEos: retval < pEos  (therefore retval is ALWAYS deref'able)
extern PChar   PtrOfColWithinStringRegionNoEos( COL tabWidth, PChar  pS, PChar  pEos, COL xCol );
STIL   PCChar  PtrOfColWithinStringRegionNoEos( COL tabWidth, PCChar pS, PCChar pEos, COL xCol ) { return PtrOfColWithinStringRegionNoEos( tabWidth, PChar(pS), PChar(pEos), xCol ); }
extern COL     ColOfPtr                       ( COL tabWidth, PCChar pS, PCChar pWithinString, PCChar pEos );


// PREFERRED   use stref and std::string with indices, not pointers
//             FreeIdxOfCol returns index that MAY be out of range; used to see whether col maps to content, or is beyond it
extern sridx   FreeIdxOfCol    ( COL tabWidth, const stref &content, const COL colTgt );
extern char    CharAtCol       ( COL tabWidth, const stref &content, const COL colTgt ); // returns 0 if col is not present in content

//             CaptiveIdxOfCol content[CaptiveIdxOfCol(colTgt)] is always valid; colTgt values which
//                             map beyond the last char of content elicit the index of the last char
STIL   sridx   CaptiveIdxOfCol ( COL tabWidth, const stref &content, const COL colTgt ) {
                  return Min( FreeIdxOfCol( tabWidth, content, colTgt ), content.length()-1 );
                  }

//             CaptiveIdxOfCols  two for the price of one?
STIL   sridx2  CaptiveIdxOfCols( COL tabWidth, const stref &content, COL x0, COL x1 ) {
                  sridx2 rv;
                  rv.ix0 = CaptiveIdxOfCol( tabWidth, content, x0 );
                  rv.ix1 = CaptiveIdxOfCol( tabWidth, content, x1 );
                  return rv;
                  }
extern COL     ColOfFreeIdx ( COL tabWidth, const stref &content, sridx offset );


struct rlc1 {
   stref ln;
   sridx ix0;
   rlc1( PFBUF pfb, LINE yy, COL x0 )
      : ln( pfb->PeekRawLine( yy ) )
      , ix0( CaptiveIdxOfCol( pfb->TabWidth(), ln, x0 ) )
      {}
   bool beyond()                                  const { return ix0 >= ln.length(); }
   bool beyond( sridx ix ) const { return ix  >= ln.length(); }
   char ch0() const { return ln[ix0]; }
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

//************ tabWidth-dependent string fxns
extern COL     ColPrevTabstop( COL tabWidth, COL xCol );
extern COL     ColNextTabstop( COL tabWidth, COL xCol );
extern COL     StrCols(        COL tabWidth, const stref &src );

extern void PrettifyAppend( std::string &dest, stref src, COL xStart, size_t maxChars, COL tabWidth, char chTabExpand, char chTrailSpcs );
extern void        FormatExpandedSeg ( std::string &dest, stref src, COL xStart, size_t maxChars, COL tabWidth, char chTabExpand=' ', char chTrailSpcs=0 ); // <-- PREFER
extern std::string FormatExpandedSeg ( /* USE RARELY!! */ stref src, COL xStart, size_t maxChars, COL tabWidth, char chTabExpand=' ', char chTrailSpcs=0 ); // <-- USE RARELY
extern COL     PrettifyMemcpy( PChar pDestBuf, size_t sizeof_dest, stref src, COL tabWidth, char chTabExpand, COL xStart=0, char chTrailSpcs=0 );

namespace FBOP { // FBUF Ops: ex-FBUF methods per Effective C++ 3e "Item 23: Prefer non-member non-friend functions to member functions."

   extern PFBUF   FindOrAddFBuf( stref filename, PFBUF *ppGlobalPtr=nullptr );

   extern void    PrimeRedrawLineRangeAllWin( PCFBUF fb, LINE yMin, LINE yMax );

   //************ ExpandWildcard FBUF
   extern int     ExpandWildcard(         PFBUF fb, PCChar pszWildcardString, const bool fSorted );
   STIL   int     ExpandWildcardSorted(   PFBUF fb, PCChar pszWildcardString ) { return ExpandWildcard( fb, pszWildcardString, true  );  }
   STIL   int     ExpandWildcardUnsorted( PFBUF fb, PCChar pszWildcardString ) { return ExpandWildcard( fb, pszWildcardString, false );  }

   //************ tab-width-dependent line-content-related calcs

   extern COL     LineCols(               PCFBUF fb, LINE yLine );

   //************ copy LINE/BOX/STREAM file->file
   extern void    CopyLines(  PFBUF FBdest, LINE yDestStart    , PCFBUF FBsrc, LINE ySrcStart, LINE ySrcEnd );
   extern void    CopyBox(    PFBUF FBdest, COL xDst, LINE yDst, PCFBUF FBsrc, COL xSrcLeft, LINE ySrcTop, COL xSrcRight, LINE ySrcBottom );
   extern void    CopyStream( PFBUF FBdest, COL xDst, LINE yDst, PCFBUF FBsrc, COL xSrcStart, LINE ySrcStart, COL xSrcEnd, LINE ySrcEnd );

   extern void    AssignFromRsrc( PCFBUF fb );
   extern Path::str_t GetRsrcExt( PCFBUF fb );


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
   extern void    PutChar_( PFBUF fb, LINE yLine, COL xCol, char theChar, bool fInsert, std::string &tmp1, std::string &tmp2 );
   STIL void      InsertChar ( PFBUF fb, LINE yLine, COL xCol, char theChar, std::string &tmp1, std::string &tmp2 ) { PutChar_( fb, yLine, xCol, theChar, true , tmp1, tmp2 ); }
   STIL void      ReplaceChar( PFBUF fb, LINE yLine, COL xCol, char theChar, std::string &tmp1, std::string &tmp2 ) { PutChar_( fb, yLine, xCol, theChar, false, tmp1, tmp2 ); }
   STIL void      DelChar( PFBUF fb, COL xLeft, LINE yTop ) { fb->DelBox( xLeft, yTop, xLeft, yTop ); }
   STIL void      DelChar( PFBUF fb, Point pt )             { DelChar( fb, pt.col, pt.lin ); }

   //************ text-content scanners
   extern PChar   IsGrepBuf( PCFBUF fb, PChar fnmbuf, const size_t sizeof_fnmbuf, int *pGrepHdrLines );
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

STIL int          g_iWindowCount() { return  g__.aWindow.size()       ; }
STIL int          g_CurWindowIdx() { return  g__.ixCurrentWin         ; }
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

STIL PFBUF        g_CurFBuf()      { extern PFBUF s_curFBuf; return s_curFBuf; }
STIL void         g_UpdtCurFBuf()  { extern PFBUF s_curFBuf;        s_curFBuf = g_CurView()->FBuf(); }

#define PCF        const auto pcf( g_CurFBuf() )
#define PCFV  PCV; PCF

inline bool FBufLocn::InCurFBuf() const { return g_CurFBuf() == d_pFBuf; }

//---------------------------------------------------------------------------------------------------------------------
//
// utility interface to allow output to be directed to PFBUF, dbgview (Windows OutputDebugString()), or both

struct OutputWriter {
   virtual void VWriteLn( PCChar string ) const = 0;
   };

struct FbufWriter : public OutputWriter {
   PFBUF d_pFBuf;
   FbufWriter( PFBUF pFBuf ) : d_pFBuf(pFBuf) {}
   virtual void VWriteLn( PCChar string ) const { d_pFBuf->PutLastLine( string ); }
   };

struct DBGWriter : public OutputWriter {
   virtual void VWriteLn( PCChar string ) const;
   };

struct DBGFbufWriter : public OutputWriter {
   PFBUF d_pFBuf;
   DBGFbufWriter( PFBUF pFBuf ) : d_pFBuf(pFBuf) {}
   virtual void VWriteLn( PCChar string ) const;
   };
//
//---------------------------------------------------------------------------------------------------------------------

extern char toLower( int ch );
