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

#include "ed_main.h"

#if defined(_WIN32)
#define  IS_LINUX  0
#define  FULL_DB   0
#else
#define  IS_LINUX  1
#define  FULL_DB   0
#endif

#if DBGHILITE
void DbgHilite_( char ch, PCChar func ) {
   DBG( "%c%s SavedHL=[%p] FileHL=[%p] (%s)"
      , ch
      , func
      , s_savedHiLiteList.dl_first
      , g_CurView()->pFBuf->d_pHiLiteList.dl_first
      , g_CurView()->pFBuf->Name()
      );
   }
#define  DbgHilite( c )  DbgHilite_( c, __func__ )
#else
#define  DbgHilite( c )
#endif

/*
   the OLD refresh model

      a series of APIs

          DispNeeds ... RefreshStatLn()
          DispNeeds ... RefreshVerticalCursorHilite()
          DispNeeds ... RefreshAllLinesCurWin()
          DispNeeds ... RefreshAllLinesAllWindows()
          DispNeeds ... RefreshCurWin()        called only in FBUF::PutFocusOn
          DispNeeds ... RefreshTotal()

      called by code that modifies the corresponding "stuff".

      This is sub-optimal: since lines to refresh are not enumerated; it's
      either all or nothing.

      Yes, the low-level video driver only pushes display deltas thru the I/O
      path, but I'm also trying to minimize display-generation overhead, AND
      have the system be "self-updating" (i.e.  hooks in the low-level
      (file/cursor) modify functions will accumulate a set of pending events
      which will be (minimally) executed when display-update time comes.

      Also, HiliteAddin's need specific, accurate information about changes in
      order to "do their thing" correctly and (again) optimally.


   the NEW refresh model (ideal)

      file modifying API are called, and "the system" keeps track of the range
      of pending changes and notifies (or is used to notify) the display update
      system about what it needs to do
*/

class LineColors {
   enum { END_MARKER=0, ELEMENTS_=BUFBYTES };  // one simplifying assumption: color==0 is not used (used for EOS)
   uint8_t b[ ELEMENTS_+1 ];
public:
   bool    inRange( int ix ) const { return ix < ELEMENTS(b); }
   uint8_t colorAt( int ix ) const { return b[ ix ]; }
   int  cols()            const { return Strlen( PCChar(&b[0]) ); }  // BUGBUG assumes END_MARKER == 0 !
   LineColors( uint8_t initcolor=END_MARKER ) {
      for( auto &ch : b ) { ch = initcolor; }
      b[ ELEMENTS_ ] = END_MARKER;
      }
   int runLength( int ix ) const {
      if( !inRange( ix ) ) { return 0; }
      const auto ix0( ix );
      const auto color( colorAt( ix ) );
      for( ++ix ; inRange( ix ) && colorAt( ix ) == color ; ++ix ) {}
      return ix - ix0;
      }
   void PutColor( int ix, int len, int color ) {
      const auto maxIx( ix+len );
      for( ; ix < maxIx && inRange( ix ) ; ++ix ) {
         b[ ix ] = uint8_t(color);
         }
      }
   void Cat( const LineColors &rhs );
   };

class LineColorsClipped {
   const View &d_view ;
         LineColors &d_alc;
   const int d_idxWinLeft ;  // LineColors ix of leftmost visible char
   const int d_colWinLeft ;
   const int d_width      ;
public:
   LineColorsClipped( const View &view, LineColors &alc, int idxWinLeft, int colWinLeft, int width )
      : d_view        ( view )
      , d_alc         ( alc         )
      , d_idxWinLeft  ( idxWinLeft  )
      , d_colWinLeft  ( colWinLeft  )
      , d_width       ( width       )
      {
      0 && DBG( "%s iWL=%d cWL=%d width=%d", __func__, idxWinLeft, colWinLeft, width );
      }
   void PutColorRaw( int col, int len, int color ) {
      0 && DBG( "%s a: %3d L %3d", __func__, col, len );
      if( col > d_colWinLeft+d_width || col + len < d_colWinLeft ) { return; }
      0 && DBG( "%s b: %3d L %3d", __func__, col, len );
      const auto colMin( Max( col      , d_colWinLeft           ) );
      const auto colMax( Min( col+len-1, d_colWinLeft+d_width-1 ) );
      const auto ixMin( colMin - d_colWinLeft + d_idxWinLeft );
      const auto ixMax( colMax - d_colWinLeft + d_idxWinLeft );
      0 && DBG( "%s c: %3d L %3d", __func__, ixMin, ixMax );
      d_alc.PutColor( ixMin, ixMax - ixMin+1, color );
      }
   void PutColor( int col, int len, int colorIdx ) { PutColorRaw( col, len, d_view.ColorIdx2Attr( colorIdx ) ); }
   };

#if VARIABLE_WINBORDER

STATIC_CONST struct
   { // H -> both horiz edges, V -> both vertical edges, T -> top edge, B -> bottom edge, L -> left edge, R -> right edge
   uint8_t HV_    , LV_    , RV_    , _V_    , H__    , HT_    , HB_      ;
   } wbc[] =
   {
    { uint8_t('≈'), uint8_t('¥'), uint8_t('√'), uint8_t('≥'), uint8_t('ƒ'), uint8_t('¡'), uint8_t('¬') },  // 0
    { uint8_t('◊'), uint8_t('∂'), uint8_t('«'), uint8_t('∫'), uint8_t('ƒ'), uint8_t('–'), uint8_t('“') },  // 1
    { uint8_t('ÿ'), uint8_t('µ'), uint8_t('∆'), uint8_t('≥'), uint8_t('Õ'), uint8_t('œ'), uint8_t('—') },  // 2
    { uint8_t('Œ'), uint8_t('π'), uint8_t('Ã'), uint8_t('∫'), uint8_t('Õ'), uint8_t(' '), uint8_t('À') },  // 3
    { uint8_t('∞'), uint8_t('∞'), uint8_t('∞'), uint8_t('∞'), uint8_t('∞'), uint8_t('∞'), uint8_t('∞') },  // 4
    { uint8_t('±'), uint8_t('±'), uint8_t('±'), uint8_t('±'), uint8_t('±'), uint8_t('±'), uint8_t('±') },  // 5
    { uint8_t('≤'), uint8_t('≤'), uint8_t('≤'), uint8_t('≤'), uint8_t('≤'), uint8_t('≤'), uint8_t('≤') },  // 6
    { uint8_t('€'), uint8_t('€'), uint8_t('€'), uint8_t('€'), uint8_t('€'), uint8_t('€'), uint8_t('€') },  // 7
   };

GLOBAL_VAR int g_swiWBCidx = 3;

int Max_wbc_idx() { return ELEMENTS(wbc)-1; }

#define HV_ (wbc[g_swiWBCidx].HV_)
#define LV_ (wbc[g_swiWBCidx].LV_)
#define RV_ (wbc[g_swiWBCidx].RV_)
#define _V_ (wbc[g_swiWBCidx]._V_)
#define H__ (wbc[g_swiWBCidx].H__)
#define HT_ (wbc[g_swiWBCidx].HT_)
#define HB_ (wbc[g_swiWBCidx].HB_)

#else

enum WinBorderChars
   { // H -> both horiz edges, V -> both vertical edges, T -> top edge, B -> bottom edge, L -> left edge, R -> right edge
#define    LINEDRAW_WIN  8
#if    (0==LINEDRAW_WIN)
                         HV_=uint8_t('≈'), LV_=uint8_t('¥'), RV_=uint8_t('√'), _V_=uint8_t('≥'), H__=uint8_t('ƒ'), HT_=uint8_t('¡'), HB_=uint8_t('¬'),  // "⁄ƒø¿Ÿ≥√¥¬¡≈",
#elif  (1==LINEDRAW_WIN)
                         HV_=uint8_t('◊'), LV_=uint8_t('∂'), RV_=uint8_t('«'), _V_=uint8_t('∫'), H__=uint8_t('ƒ'), HT_=uint8_t('–'), HB_=uint8_t('“'),  // "÷ƒ∑”Ω∫«∂“–◊",
#elif  (2==LINEDRAW_WIN)
                         HV_=uint8_t('ÿ'), LV_=uint8_t('µ'), RV_=uint8_t('∆'), _V_=uint8_t('≥'), H__=uint8_t('Õ'), HT_=uint8_t('œ'), HB_=uint8_t('—'),  // "’Õ∏‘æ≥∆µ—œÿ"
#elif  (3==LINEDRAW_WIN)
                         HV_=uint8_t('Œ'), LV_=uint8_t('π'), RV_=uint8_t('Ã'), _V_=uint8_t('∫'), H__=uint8_t('Õ'), HT_=uint8_t(' '), HB_=uint8_t('À'),  // "…Õª»º∫ÃπÀ Œ",
#elif  (4==LINEDRAW_WIN)
                         HV_=uint8_t('∞'), LV_=HV_,     RV_=HV_,     _V_=HV_,     H__=HV_,     HT_=HV_,     HB_=HV_,      // "∞∞∞∞∞∞∞∞∞∞∞"
#elif  (5==LINEDRAW_WIN)
                         HV_=uint8_t('±'), LV_=HV_,     RV_=HV_,     _V_=HV_,     H__=HV_,     HT_=HV_,     HB_=HV_,      // "±±±±±±±±±±±"
#elif  (6==LINEDRAW_WIN)
                         HV_=uint8_t('≤'), LV_=HV_,     RV_=HV_,     _V_=HV_,     H__=HV_,     HT_=HV_,     HB_=HV_,      // "≤≤≤≤≤≤≤≤≤≤≤"
#elif  (7==LINEDRAW_WIN)
                         HV_=uint8_t('€'), LV_=HV_,     RV_=HV_,     _V_=HV_,     H__=HV_,     HT_=HV_,     HB_=HV_,      // "€€€€€€€€€€€"
#elif  (8==LINEDRAW_WIN)
                         HV_=uint8_t(' '), LV_=HV_,     RV_=HV_,     _V_=HV_,     H__=HV_,     HT_=HV_,     HB_=HV_,      // "           "
#endif
#undef     LINEDRAW_WIN
   };

#endif

enum { DBG_HL_EVENT = 0 };

//-----------------------------------------------------------------------------------------------------------

STATIC_FXN char GenAltHiliteColor( const char color ) {
   // algorithm ATTEMPTS to choose a _readable_ hilite-alternative-color for a given color
   const char fgColor( (color & FGmask)    );
   const char bgColor( (color & BGmask)    );
   if( bgBLK == bgColor ) { return ( fgColor | (bgColor ^ 0x10) ); }
   const auto fgBrite( (color & FGhi) != 0 );
   const auto bgBrite( (color & BGhi) != 0 );
   if(        fgBrite &&  bgBrite ) { return ( color ^ BGhi ); }
   else if(   fgBrite && !bgBrite ) { return ( color ^ BGhi ); }
   else if(  !fgBrite &&  bgBrite ) { return ( color ^ FGhi ); }
   else                             { return ( color ^ BGhi ); }
   }

STATIC_CONST struct {  // contents init'd from $KINIT:k.filesettings
   PCChar   pLuaName;
   size_t   ofs;
   int      dflt;
   } s_color2Lua[] = {
#define SINIT( nm, idx, dflt )  { #nm, idx, dflt }
   SINIT( txt , ColorTblIdx::TXT , 0x1E ),
   SINIT( hil , ColorTblIdx::HIL , 0x3F ),
   SINIT( sel , ColorTblIdx::SEL , 0x2F ),
   SINIT( wuc , ColorTblIdx::WUC , 0xF2 ),
   SINIT( cxy , ColorTblIdx::CXY , 0x4F ),
   SINIT( str , ColorTblIdx::STR , 0x16 ),
   SINIT( cpp , ColorTblIdx::CPP , 0x06 ),
   SINIT( com , ColorTblIdx::COM , 0x08 ),
#undef SINIT
   };

typedef uint8_t ViewColors[ ColorTblIdx::VIEW_COLOR_COUNT ];

static_assert( ELEMENTS( s_color2Lua ) == sizeof( ViewColors ), "ELEMENTS( s_color2Lua ) != ELEMENTS( ViewColors )" );

//--------------------------------------------------------------------------------------------------------------------------
STATIC_VAR RbTree *s_FTS_idx;

STATIC_FXN inline FTypeSetting *IdxNodeToFTS( RbNode *pNd ) { return static_cast<FTypeSetting *>( rb_val(pNd) ); }  // type-safe conversion function

struct FTypeSetting {
   enum { DB=0 };
   enum HL_ID {
          HL_none   ,
          HL_C      ,
          HL_MAKE   ,
          HL_LUA    ,
          HL_PYTHON ,
          HL_BASH   ,
          HL_DIFF   ,
        };

   Path::str_t d_key;  // rbtree key
   ViewColors  d_colors;
   char        d_eolCommentDelim[5]; // the longest eol-comment I know of is "rem " ...
   HL_ID       d_hl_id;

   void  Update();

   FTypeSetting( stref ext ) : d_key( sr2st(ext) ) {                         0 && DBG( "%s CTOR: '%" PR_BSR "' ----------------------------------------------", __func__, BSR(d_key) );
      Update();
      }
   ~FTypeSetting() {}

   };

STATIC_FXN int Show_FTypeSettings() {                                        FTypeSetting::DB && DBG( "%s+ ----------------------------------------------", __func__ );
   rb_traverse( pNd, s_FTS_idx ) {
      const auto pFTS( IdxNodeToFTS( pNd ) );
      FTypeSetting::DB && DBG( "%s  [%s]", __func__, pFTS->d_key.c_str() );
      }                                                                      FTypeSetting::DB && DBG( "%s- ----------------------------------------------", __func__ );
   return 1;
   }

void FTypeSetting::Update() {
   linebuf kybuf; auto pbuf( kybuf ); auto kybufBytes( sizeof kybuf );
   snprintf_full( &pbuf, &kybufBytes, "filesettings.ftype_map.%s.", d_key.c_str() );
   d_eolCommentDelim[0] = '\0';
   scpy( pbuf, kybufBytes, "eolCommentDelim" );
   LuaCtxt_Edit::Tbl2S( BSOB(d_eolCommentDelim), kybuf, "" );           DB && DBG( "%s: %s = %s", __func__, kybuf, d_eolCommentDelim );
   {
   scpy( pbuf, kybufBytes, "hilite" );
   char hiliteNmBuf[21];
   LuaCtxt_Edit::Tbl2S( BSOB(hiliteNmBuf), kybuf, "" );           DB && DBG( "%s: %s = %s", __func__, kybuf, d_eolCommentDelim );
   stref key( hiliteNmBuf );
   STATIC_CONST struct {
      PCChar   nm;
      HL_ID    enumval;
      } hlnms[] = {
      { "c"      , HL_C      },
      { "make"   , HL_MAKE   },
      { "lua"    , HL_LUA    },
      { "python" , HL_PYTHON },
      { "bash"   , HL_BASH   },
      { "diff"   , HL_DIFF   },
      };
   d_hl_id = HL_none;
   for( const auto &cc : hlnms ) {
      if( eq( key, cc.nm ) ) {
         d_hl_id = cc.enumval;
         break;
         }
      }
   }
   snprintf_full( &pbuf, &kybufBytes, "colors." );
   for( const auto &c2L : s_color2Lua ) {
      scpy( pbuf, kybufBytes, c2L.pLuaName );
      d_colors[ c2L.ofs ] = LuaCtxt_Edit::Tbl2Int( kybuf, c2L.dflt );   DB && DBG( "%s: %s = 0x%02X", __func__, kybuf, d_colors[ c2L.ofs ] );
      }
// d_colors[ ColorTblIdx::CXY ] = GenAltHiliteColor( d_colors[ ColorTblIdx::TXT ] );
   }

void InitFTypeSettings() {                                              FTypeSetting::DB && DBG( "%s+ ----------------------------------------------", __func__ );
   STATIC_VAR RbCtrl s_FTS_idx_RbCtrl = { AllocNZ_, Free_, };
   s_FTS_idx = rb_alloc_tree( &s_FTS_idx_RbCtrl );                      FTypeSetting::DB && DBG( "%s- ----------------------------------------------", __func__ );
   }

STATIC_FXN void DeleteFTS( void *pData, void *pExtra ) {                FTypeSetting::DB && DBG( "%s+ ----------------------------------------------", __func__ );
   auto pFTS( static_cast<FTypeSetting *>(pData) );
   Free0( pFTS );                                                       FTypeSetting::DB && DBG( "%s- ----------------------------------------------", __func__ );
   }

void CloseFTypeSettings() {
   rb_dealloc_treev( s_FTS_idx, nullptr, DeleteFTS );
   }

STATIC_FXN FTypeSetting *Get_FTypeSetting( stref ftype ) {              FTypeSetting::DB && DBG( "%s+ ---------------------------------------------- PROBING [%" PR_BSR "]", __func__, BSR(ftype) );
   int equal;
   auto pNd( rb_find_gte_sri( &equal, s_FTS_idx, ftype ) );
   if( equal ) {                                                        FTypeSetting::DB && DBG( "%s FOUND [%" PR_BSR "]", __func__, BSR(ftype) );
      return IdxNodeToFTS( pNd );
      }
   auto pNew( new FTypeSetting( ftype ) );                              FTypeSetting::DB && DBG( "%s CREATING [%" PR_BSR "]", __func__, BSR(ftype) );
   rb_insert_before( s_FTS_idx, pNd, pNew->d_key.c_str(), pNew );
   return pNew;
   }

void Reread_FTypeSettings() {                                           FTypeSetting::DB && DBG( "%s+ ----------------------------------------------", __func__ );
   rb_traverse( pNd, s_FTS_idx ) {
      const auto pFTS( IdxNodeToFTS( pNd ) );                           FTypeSetting::DB && DBG( "%s  [%s]", __func__, pFTS->d_key.c_str() );
      pFTS->Update();
      }                                                                 FTypeSetting::DB && DBG( "%s- ----------------------------------------------", __func__ );
   }

GLOBAL_CONST unsigned char *g_colorVars[] = {
   &g_colorInfo      ,
   &g_colorStatus    ,
   &g_colorWndBorder ,
   &g_colorError     ,
   };

static_assert( ELEMENTS(g_colorVars) == (ColorTblIdx::COLOR_COUNT - ColorTblIdx::VIEW_COLOR_COUNT), "ELEMENTS(g_colorVars) == ColorTblIdx::COLOR_COUNT" );

void FBUF::SetFType( stref ft ) {
   if( !eqi( ft, d_ftype ) ) {
      d_ftype.assign( sr2st(ft) );
      d_ftypeStruct = ::Get_FTypeSetting( ft );
      }
   }

int View::ColorIdx2Attr( int colorIdx ) const {
   if( colorIdx < ColorTblIdx::VIEW_COLOR_COUNT ) {
      const auto pFTS( CFBuf()->GetFTypeSettings() );
      return  pFTS ? pFTS->d_colors[ colorIdx ] :
             *g_colorVars[ ColorTblIdx::ERRM - ColorTblIdx::VIEW_COLOR_COUNT ];
      }
   if( colorIdx < ColorTblIdx::COLOR_COUNT ) {
      return *g_colorVars[ colorIdx          - ColorTblIdx::VIEW_COLOR_COUNT ];
      }
   return    *g_colorVars[ ColorTblIdx::ERRM - ColorTblIdx::VIEW_COLOR_COUNT ];
   }

//-----------------------------------------------------------------------------------------------------------

#define  KSTRCMP( kstr, sz )  memcmp( kstr, sz, KSTRLEN(kstr) )

/*
   design notes, HiliteAddin system

      1.  I'm lazy: I want the most effect for the least effort.
      2.  In _some_ hiliting situations, as an exception, I'm not a perfectionist.
      3.  I'm not a trained Computer Scientist.  The idea of writing a sufficiently
          correct and robust parser to perform full syntax hiliting for any HLL is
          sufficiently daunting to me that I won't attempt it.  The amount of
          (complex) code needed is beyond my bandwidth to write and/or maintain.
      4.  201407: I (partially) bit the bullet on #3, and from scratch wrote a
          simple generic parser framework, HiliteAddin_StreamParse, for comments
          and literal strings presently covering (in separate derivative
          implementations) C/C++, Lua, Python.  Development of full language
          parsing (to the statement/block/keyword level) remains unlikely; I just
          wanted very reliable _low-lighting_ of comments to facilitate reading
          "enterprise" codebases which often contain vast amounts of gratuitous
          boilerplate comments (e.g.  AutoDuck function comment headers).
          A more likely future development is a search mode that ignores
          comments as parsed herein.

   20071014 kgoodwin
*/

class HiliteAddin { // prototype base/interface class: public HiliteAddin
protected:
   View  &d_view ;
public:
   HiliteAddin( PView pView ) : d_view( *pView ) {}
   virtual bool VWorthKeeping() { return true; }
   virtual PCChar Name() const = 0;
   virtual ~HiliteAddin() {}
   /* update/event-receive methods: any useful incarnation of HiliteAddin must
      implement at least one of these, which is used to optimally update object
      state which is derived from the event's info */
   virtual void VCursorMoved( bool fUpdtWUC ) {}
   virtual void VFbufLinesChanged( LINE yMin, LINE yMax ) {}
   virtual void VWinResized() {}
   /* output methods: the _purpose_ of HiliteAddin objects; how they manifest
      themselves at the user level (by changing the color of displayed FBUF content).
      retval of bool HilitLine...(): true means stop hiliting after this */
   virtual bool VHilitLine    ( LINE yLine, COL xIndent, LineColorsClipped &alcc ) { return false; }
   virtual bool VHilitLineSegs( LINE yLine,              LineColorsClipped &alcc ) { return false; }
   virtual size_t VGetStreamParse( LINE yLine, hl_rgn_t *&hlrt ) { return 0; /* number of valid entries in hlrt */ }
   DLinkEntry <HiliteAddin> dlinkAddins;
protected:
         PCFBUF CFBuf()               const { return d_view.CFBuf(); }
         LINE   Get_LineCompile()     const { return d_view.Get_LineCompile(); }
   const Point &Cursor()              const { return d_view.Cursor(); }
   const Point &Origin()              const { return d_view.Origin(); }
         LINE   ViewLines()           const { return d_view.Win()->d_Size.lin ; }
         LINE   ViewCols()            const { return d_view.Win()->d_Size.col ; }
         LINE   MaxVisibleFbufLine()  const { return Origin().lin + ViewLines() - 1; }
         bool   isActiveWindow()      const { return d_view.Win() == g_CurWin(); }
   int  ColorIdx2Attr( int colorIdx ) const { return d_view.ColorIdx2Attr( colorIdx ); }
   NO_COPYCTOR(HiliteAddin);
   NO_ASGN_OPR(HiliteAddin);
   void DispNeedsRedrawAllLines() { d_view.Win()->DispNeedsRedrawAllLines(); }
   };

//--------------------------------------------------------------------------

class HiliteAddin_Pbal : public HiliteAddin {
   void VCursorMoved( bool fUpdtWUC ) override;
   bool VHilitLineSegs( LINE yLine,              LineColorsClipped &alcc ) override;
public:
   HiliteAddin_Pbal( PView pView )
      : HiliteAddin( pView )
      , d_fPbalMatchValid( false )
      {}
   ~HiliteAddin_Pbal() {}
   PCChar Name() const override { return "PBal"; }
private:
   bool  d_fPbalMatchValid;
   Point d_ptPbalMatch;
   };

void HiliteAddin_Pbal::VCursorMoved( bool fUpdtWUC ) {
   const auto prevPbalValid( d_fPbalMatchValid );
   d_fPbalMatchValid = d_view.PBalFindMatching( false, &d_ptPbalMatch );
   if( d_fPbalMatchValid || prevPbalValid ) { // turning ON, turning OFF, _or_ MOVING?
      DispNeedsRedrawAllLinesAllWindows(); // BUGBUG need API to select lines/ranges of curWindow
      }
   }

bool HiliteAddin_Pbal::VHilitLineSegs( LINE yLine, LineColorsClipped &alcc ) {
   if( d_fPbalMatchValid && d_ptPbalMatch.lin == yLine ) { 0 && DBG( "HiliteAddin_Pbal::HilitLine %d %d", d_ptPbalMatch.lin, yLine );
      alcc.PutColor( d_ptPbalMatch.col, 1, ColorTblIdx::WND );
      }
   return false;
   }

/* 20110516 kgoodwin
   allow a envp-style sequence of WUC needles so that calc'd derivatives of the
   actual WUC can be highlighted.  In particular, the MWDT elfdump disasm shows
   0xhexaddress in intruction operands, but each instruction's address is shown
   as hexaddress (and I want the latter to be highlighted when the former is
   WUC).  */

class HiliteAddin_WordUnderCursor : public HiliteAddin {
   void VCursorMoved( bool fUpdtWUC ) override;
// void VFbufLinesChanged( LINE yMin, LINE yMax )  { /* d_wucbuf[0] = '\0'; */ }
   bool VHilitLineSegs( LINE yLine,              LineColorsClipped &alcc ) override;
public:
   HiliteAddin_WordUnderCursor( PView pView )
      : HiliteAddin( pView ) {
      clear();
      }
   ~HiliteAddin_WordUnderCursor() {}
   PCChar Name() const override { return "WUC"; }
private:
   StringsBuf<BUFBYTES> d_sb;
   void   clear()               {        d_sb.clear();         }
   PCChar AddKey( stref sr )    { return d_sb.AddString( sr ); }
   void   SetNewWuc( stref src, LINE lin, COL col );
   std::string  d_stCandidate;
   std::string  d_stSel;     // d_stSel content must look like StringsBuf content, which means an extra/2nd NUL marks the end of the last string
   LINE         d_yWuc = -1; // BUGBUG if edits occur, need to set d_yWuc = -1 (or d_wucbuf[0] = 0)
   COL          d_xWuc = -1;
   };

void HiliteAddin_WordUnderCursor::SetNewWuc( stref src, LINE lin, COL col ) { enum { DBG_HL_EVENT=0 };
   d_stSel.clear();
   if( d_yWuc == lin && d_sb.find( src ) ) { /* assume transitivity */                                          DBG_HL_EVENT && DBG("unch->%s", d_sb.data() );
      if( d_xWuc != col ) {
          d_xWuc  = col;
          DispNeedsRedrawAllLines();
          }
      return;
      }
   clear(); // aaa aaa aaa aaa
   stref wuc( AddKey( src ) );
   if( !wuc.data() ) {                                                                                          DBG_HL_EVENT && DBG( "%s toolong", __func__);
      return;
      }                                                                                                         DBG_HL_EVENT && DBG( "wuc=%" PR_BSR, BSR(wuc) );
   d_yWuc = lin; d_xWuc = col;
   if( lin >= 0 ) {                                                                                                                             auto keynum( 1 );
      if( !wuc.starts_with( "$" )) { // experimental
         char scratch[81]; PCChar key;
               bcat( bcpy( scratch, "$"  ).length(), scratch, wuc );                          key = AddKey( scratch );  DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key );   ++keynum;
         bcat( bcat( bcpy( scratch, "$(" ).length(), scratch, wuc ).length(), scratch, ")" ); key = AddKey( scratch );  DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key );   ++keynum;
         bcat( bcat( bcpy( scratch, "${" ).length(), scratch, wuc ).length(), scratch, "}" ); key = AddKey( scratch );  DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key );   ++keynum;
         }
      if(0) { // GCCARM variations: funcname -> __funcname_veneer
         STATIC_CONST char vnr_pfx[] = { "__"      };  CompileTimeAssert( 2 == KSTRLEN(vnr_pfx) );
         STATIC_CONST char vnr_sfx[] = { "_veneer" };  CompileTimeAssert( 7 == KSTRLEN(vnr_sfx) );
         const auto vnr_fx_len( KSTRLEN(vnr_pfx)+KSTRLEN(vnr_sfx) );
         if( (wuc.length() > vnr_fx_len) && wuc.starts_with( vnr_pfx ) && wuc.ends_with( vnr_sfx ) ) {
            PCChar key( AddKey( wuc.substr( KSTRLEN(vnr_pfx), wuc.length() - vnr_fx_len ) ) );                  DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key );   ++keynum;
            }
         else {
            char scratch[61];
            if( (wuc.length() > 1) && (wuc.length() < sizeof(scratch)-vnr_fx_len) && isalpha( wuc[0] ) ) {
               PCChar key( AddKey( bcat( bcat( bcpy( scratch, vnr_pfx ).length(), scratch, wuc ).length(), scratch, vnr_sfx ) ) );  DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key );   ++keynum;
               }
            }
         }
      if(0) { // MWDT hexnum variations: 0xdeadbeef, 0xdead_beef (<- old MWDT elfdump format), deadbeef
         if( (wuc.length() > 2) && 0==strnicmp_LenOfFirstStr( "0x", wuc ) ) {
            auto hex( wuc ); hex.remove_prefix( 2 );
            const int xrun( consec_xdigits( hex ) );                                                            DBG_HL_EVENT && DBG( "xrun=%d",xrun );
            if( (8==hex.length()) && (8==xrun) ) {                                                              DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, hex.data() );
               PCChar key( AddKey( hex ) );                                                                     DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key );   ++keynum;
               }
            else if( (9==hex.length()) && (4==xrun) && ('_'==hex[4]) && (4==consec_xdigits( hex.data()+5 )) ) {
               char kb[] = { hex[0], hex[1], hex[2], hex[3], hex[5], hex[6], hex[7], hex[8], 0 };               DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, kb );
               PCChar key( AddKey( kb ) );                                                                      DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key );   ++keynum;
               }
            }
         else if( (8==wuc.length()) && (8==consec_xdigits( wuc )) ) {       //                                  DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, wuc+2 );
            char kb[] = { '0','x', wuc[0], wuc[1], wuc[2], wuc[3], wuc[4], wuc[5], wuc[6], wuc[7], 0, 0 };
            stref key( AddKey( kb ) );                                                                          DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key.data() );   ++keynum;
            auto ix( 6 );
            kb[ix++] = '_';
            kb[ix++] = wuc[4];
            kb[ix++] = wuc[5];
            kb[ix++] = wuc[6];
            kb[ix++] = wuc[7];
            kb[ix++] = 0;
            key = AddKey( kb );                                                                                 DBG_HL_EVENT && DBG( "WUC[%d]='%s'", keynum, key.data() );   ++keynum;
            }
         }
      }
   DispNeedsRedrawAllLines();                                                                                // DBG_HL_EVENT && DBG( "WUC='%s'", wuc );
   }

GLOBAL_VAR int g_iWucMinLen = 2;

stref GetWordUnderPoint( PCFBUF pFBuf, Point *cursor ) {
   const auto yCursor( cursor->lin );
   const auto xCursor( cursor->col );
   const auto rl( pFBuf->PeekRawLine( yCursor ) );
   if( !rl.empty() ) { 0 && DBG( "newln=%" PR_BSR, BSR(rl) );
      const auto tw( pFBuf->TabWidth() );                             // abc   abc
      if( xCursor < ColOfFreeIdx( tw, rl, rl.length() ) ) {
         const auto ixC( CaptiveIdxOfCol( tw, rl, xCursor ) );
         if( isWordChar( rl[ixC] ) ) {
            const auto ixFirst   ( IdxFirstWordCh   ( rl, ixC   ) );
            const auto ixPastLast( FirstNonWordOrEnd( rl, ixC   ) );  0 && DBG( "ix[%" PR_SIZET "/%" PR_SIZET "/%" PR_SIZET "]", ixFirst, ixC, ixPastLast );
            const auto xMin( ColOfFreeIdx( tw, rl, ixFirst      ) );
            const auto xMax( ColOfFreeIdx( tw, rl, ixPastLast-1 ) );  0 && DBG( "x[%d..%d]", xMin, xMax );
            const auto wordCols ( xMax - xMin + 1 );
            const auto wordChars( ixPastLast - ixFirst );
            // this degree of paranoia only matters if the definition of a WORD includes a tab
            if( 0 && wordCols != wordChars ) { DBG( "%s wordCols=%d != wordChars=%" PR_PTRDIFFT, __func__, wordCols, wordChars ); }
            // return everything
            cursor->col = xMin;
            return stref( rl.data() + ixFirst, wordChars );
            }
         }
      }
   return "";
   }

std::string GetDQuotedStringUnderPoint( PCFBUF pFBuf, const Point &cursor ) {
   /*
    search upstream for a ["quote-word boundary]; if found, search downstream
    for a [word-quote boundary"]; it might straddle a line-break; return the
    space-normalized contatenation of the text between these two boundaries.
    NB: The DQuotedString can span NO MORE THAN TWO (adjacent) LINES
    */
   std::string rv;
   const auto yCursor( cursor.lin );
   const auto xCursor( cursor.col );
   const auto rlpt( pFBuf->PeekRawLine( yCursor ) );
   if( !rlpt.empty() ) { 0 && DBG( "newln=%" PR_BSR, BSR(rlpt) );
      const auto tw( pFBuf->TabWidth() );                             // abc   abc
      const auto ixPt( CaptiveIdxOfCol( tw, rlpt, xCursor ) );
      auto cat_rv = [&]( stref st ) {
         DBG( "cat_rv=%" PR_BSR "'", BSR(st) );
         for( auto ix(0) ; ix<st.length() ; ++ix ) {
            const auto ch( st[ix] );
            if( isspace( ch ) && (rv.length() == 0 || isspace(rv.back())) ) {
               }
            else {
               rv.push_back( ch );
               }
            }
         };
      {
      auto ixUpstream = [&]( stref rl, const COL ixC ) {
         for( auto ix(ixC) ; ix > 0 ; --ix ) {
            if( !isspace( rl[ix] ) && ('"'==rl[ix-1]) ) {
               const auto rlUp( rl.substr( ix, ixC-ix+1 ) );   DBG( "up=%" PR_BSR "'", BSR(rlUp) );
               cat_rv( rlUp );
               return true;
               }
            }
         return false;
         };
      if( !ixUpstream( rlpt, ixPt ) && yCursor > 0 ) {
         const auto rlp( pFBuf->PeekRawLine( yCursor-1 ) );
         ixUpstream( rlp, rlp.length()-1 );
         cat_rv( " " );
         cat_rv( rlpt.substr(0,ixPt) );
         }
      if( rv.empty() ) { return ""; } // ********** upstream done
      }
            /*
            "hello
               there"
             */
      {
      auto ixDnstream = [&]( stref rl, const COL ixC ) {          0&&DBG( "dn0 %d [%" PR_BSR "]", ixC, BSR(rl) );
         for( auto ix(ixC) ; ix < rl.length()-1 ; ++ix ) {  0&&DBG( "dn[%d]>%c", ixC, rl[ixC] );
            if( !isspace( rl[ix] ) && ('"'==rl[ix+1]) ) {
               const auto rlDn( rl.substr(ixC,ix-ixC+1) );     DBG( "dn=%" PR_BSR "'", BSR(rlDn) );
               cat_rv( rlDn );
               return true;
               }
            }
         return false;
         };
      if( !ixDnstream( rlpt, ixPt+1 ) ) {
         cat_rv( " " );
         ixDnstream( pFBuf->PeekRawLine( yCursor+1 ), 0 );
         }
      }
      }
   return rv;
   }

#ifdef fn_dquc
bool ARG::dquc() {
   const auto st( GetDQuotedStringUnderPoint( g_CurFBuf(), g_Cursor() ) );
   Msg( "%" PR_BSR "'", BSR(st) );
   return true;
   }
#endif

void HiliteAddin_WordUnderCursor::VCursorMoved( bool fUpdtWUC ) {
   if( d_view.GetBOXSTR_Selection( d_stCandidate ) && !IsStringBlank( d_stCandidate ) ) {
      if( d_stSel != d_stCandidate ) {
         d_stSel = d_stCandidate;
         d_stSel.push_back( 0 );  // d_stSel content must look like StringsBuf content, which means an extra/2nd NUL marks the end of the last string
         0 && DBG( "BOXSTR=%s|", d_stSel.c_str() );
         d_yWuc = -1;
         d_xWuc = -1;
         clear();
         DispNeedsRedrawAllLines();
         }
      }
   else if( fUpdtWUC ) {
      const auto yCursor( Cursor().lin );
      auto start( Cursor() );
      const auto wuc( GetWordUnderPoint( CFBuf(), &start ) );
      if( !wuc.empty() ) {
         if( wuc.length() >= g_iWucMinLen ) { // WUC is long enough?
            SetNewWuc( wuc, start.lin, start.col );
            }
         }
      else { // NOT ON A WORD
         if( !d_sb.empty() && yCursor == d_yWuc ) {
            DispNeedsRedrawAllLines(); // BUGBUG s/b optimized
            }
         }
      }
   }

bool HiliteAddin_WordUnderCursor::VHilitLineSegs( LINE yLine, LineColorsClipped &alcc ) {
   auto fb( CFBuf() );
   const auto rl( fb->PeekRawLine( yLine ) );
   if( !rl.empty() ) {
      const auto keyStart( !d_sb.empty() ? d_sb.data() : (d_stSel.empty() ? nullptr : d_stSel.c_str()) );
      if( keyStart ) {
         const auto tw( fb->TabWidth() );
         for( size_t ofs( 0 ) ; ofs < rl.length() ; ) {
            auto ixBest( stref::npos ); auto mlen( 0u );
            auto haystack( rl ); haystack.remove_prefix( ofs );
            for( auto pNeedle(keyStart) ; *pNeedle ;  ) {
               const stref needle( pNeedle );
               auto ixFind( haystack.find( needle ) );
               if( ixFind != stref::npos ) {
                  ixFind += ofs;
                  if( ixBest == stref::npos || ixFind < ixBest ) {
                     ixBest = ixFind; mlen = needle.length();
                     }
                  } // xWUCX xWUCX xWUCX xWUCX xWUCX xWUCX xWUCX xWUCX xWUCX
               pNeedle += needle.length() + 1;
               }
            if( ixBest == stref::npos ) { break; }
            const auto xFound( ColOfFreeIdx( tw, rl, ixBest ) );
            if(   -1 == d_yWuc // is a selection pseudo-WUC?
               || // or a true WUC
                 (  (ixBest == 0 || !isWordChar( rl[ixBest-1] )) && (ixBest+mlen >= rl.length() || !isWordChar( rl[ixBest+mlen] )) // only match _whole words_ matching d_wucbuf
                 && (yLine != Cursor().lin || Cursor().col < xFound || Cursor().col > xFound + mlen - 1)  // DON'T hilite actual WUC (it's visually annoying)
                 )
              ) {
               alcc.PutColor( xFound, mlen, ColorTblIdx::WUC );
               }
            ofs = ixBest + mlen;   // xWUCXxWUCXxWUCXxWUCX
            }
         }
      }
   return false;
   }

struct CppCondEntry_t {
   stref  nm;
   cppc   val;
   };

STATIC_FXN cppc IsCppConditional( stref src, int *pxMin ) { // *pxMin indexes into src[] which is RAW line text
   const auto o1( FirstNonBlankOrEnd( src, 0      ) );  if( o1 >= src.length() || !('#'   ==   src[o1]) ) { return cppcNone; }
   const auto o2( FirstNonBlankOrEnd( src, o1 + 1 ) );  if( o2 >= src.length() || !isWordChar( src[o2]) ) { return cppcNone; }
   const auto o3( FirstNonWordOrEnd ( src, o2 + 1 ) );  const auto word( src.substr( o2, o3-o2 ) );
   STATIC_CONST CppCondEntry_t cond_keywds[] = {
         { "if"      , cppcIf   },
         { "ifdef"   , cppcIf   },
         { "ifndef"  , cppcIf   },
         { "else"    , cppcElse },
         { "elif"    , cppcElif },
         { "endif"   , cppcEnd  },
      };
   for( const auto &cc : cond_keywds ) {
      if( eq( word, cc.nm ) ) {
         *pxMin = o1;
         return cc.val;
         }
      }
   return cppcNone;
   }

STATIC_FXN cppc IsGnuMakeConditional( stref src, int *pxMin ) { // *pxMin indexes into src[] which is RAW line text
   if( !src.empty() && src[0] == HTAB ) { return cppcNone; }
   const auto o2( FirstNonBlankOrEnd( src, 0      ) );  if( o2 >= src.length() || !isWordChar( src[o2] ) ) { return cppcNone; }
   const auto o3( FirstNonWordOrEnd ( src, o2 + 1 ) );  const auto word( src.substr( o2, o3-o2 ) );
   STATIC_CONST CppCondEntry_t cond_keywds[] = {
         { "ifeq"    , cppcIf   },
         { "ifneq"   , cppcIf   },
         { "ifdef"   , cppcIf   },
         { "ifndef"  , cppcIf   },
         { "else"    , cppcElif }, // gmake else has elseif behavior
         { "endif"   , cppcEnd  },
         { "define"  , cppcIf   }, // not truly cond, but syntactically ...
         { "endef"   , cppcEnd  }, // ... same and worth hiliting
      };
   for( const auto &cc : cond_keywds ) {
      if( eq( word, cc.nm ) ) {
         *pxMin = o2;
         return cc.val;
         }
      }
   return cppcNone;
   }

cppc FBOP::IsCppConditional( PCFBUF fb, LINE yLine ) {
   const auto rl( fb->PeekRawLine( yLine ) );
   COL xPound;
   return ::IsCppConditional( rl, &xPound );
   }

class HiliteAddin_cond_CPP : public HiliteAddin {
   void VFbufLinesChanged( LINE yMin, LINE yMax ) override { refresh( yMin, yMax ); }
   bool VHilitLine   ( LINE yLine, COL xIndent, LineColorsClipped &alcc ) override;
   void VWinResized() override {
      d_PerViewableLine.resize( ViewLines() );
      d_need_refresh = true;
      }
   virtual cppc IsCppConditional( stref src, int *pxPound ) { return ::IsCppConditional( src, pxPound ); }
public:
   HiliteAddin_cond_CPP( PView pView )
      : HiliteAddin( pView ) {
      d_PerViewableLine.resize( ViewLines() );
      d_need_refresh = true;
      }
   ~HiliteAddin_cond_CPP() {}
   PCChar Name() const override { return "cond_CPP"; }
private:
   bool d_need_refresh = false;
   struct PerViewableLineInfo {
      struct    {
         cppc   acppc;
         COL    xPound;
         COL    xMax;
         int    level_ix;
         }      line;
      struct    {
         int    containing_level_idx;
         LINE   yMin;
         LINE   yMax;
         COL    xBox;
         }      level;
      };
   std::vector<PerViewableLineInfo> d_PerViewableLine ;
   int close_level( int level_ix, int yLast );
   void refresh( LINE, LINE );
   };

int HiliteAddin_cond_CPP::close_level( int level_ix, int yLast ) {
   if( level_ix < 0 ) { return -1; } // CID128055
   auto &level( d_PerViewableLine[ level_ix ].level );
   level.yMax = yLast;
   for( auto ixLine(level.yMin) ; ixLine <= level.yMax ; ++ixLine ) {
      auto &line( d_PerViewableLine[ ixLine ].line );
      NoLessThan( &level.xBox, line.xMax+1 );
      }
   // push out xBox of containing levels so they don't collide with this level's xBox
   for( auto bump(0) ; level_ix > -1 ; level_ix = d_PerViewableLine[ level_ix ].level.containing_level_idx, bump += 2 ) {
      auto &cont_level( d_PerViewableLine[ level_ix ].level );
      NoLessThan( &cont_level.xBox, level.xBox + bump );
      }
   return level.containing_level_idx;
   }

void HiliteAddin_cond_CPP::refresh( LINE, LINE ) {
   // pass 1: fill in line.{ xMax, cppc?, xPound? }
   auto maxUnIfdEnds(0);
   {
   auto not_elses(0), elses(0);
   auto upDowns(0);
   auto fb( CFBuf() );
   const auto tw( fb->TabWidth() );
   for( auto iy(0) ; iy < ViewLines() ; ++iy ) {
      auto &line( d_PerViewableLine[ iy ].line );
      const auto yFile( Origin().lin + iy );
      const auto rl( fb->PeekRawLine( yFile ) );
      line.xMax = ColOfFreeIdx( tw, rl, rl.length() );
      line.acppc = IsCppConditional( rl, &line.xPound );
      if( cppcNone != line.acppc ) {
         line.xPound = ColOfFreeIdx( tw, rl, line.xPound );
         switch( line.acppc ) {
            default:       break;
            case cppcIf  : --upDowns;
                           ++not_elses;
                           break;
            case cppcEnd : ++upDowns; maxUnIfdEnds = Max( maxUnIfdEnds, upDowns );
                           ++not_elses;
                           break;
            case cppcElif:
            case cppcElse: ++elses;
                           break;
            }
         }
      }
   if( 0 == not_elses && elses > 0 ) {
      maxUnIfdEnds = 1;
      }
   }
   // pass 2:
   auto level_alloc_idx(-1);  // increases monotonically
   auto level_idx(-1);        // goes up and down
   while( level_idx < maxUnIfdEnds-1 ) {
      auto &level( d_PerViewableLine[ (level_idx = ++level_alloc_idx) ].level );
      level.containing_level_idx = level_idx-1; level.yMin = 0; level.yMax = 0; level.xBox = 0;
      }
   for( auto iy(0) ; iy < ViewLines() ; ++iy ) {
      auto &line( d_PerViewableLine[ iy ].line );
      line.level_ix = level_idx;
      switch( line.acppc ) {
         case cppcIf  :{const auto containing_level_idx( level_idx );
                        auto &level = d_PerViewableLine[ (line.level_ix = level_idx = ++level_alloc_idx) ].level;
                        level.containing_level_idx = containing_level_idx; level.yMin = iy; level.yMax = iy; level.xBox = 0;
                       } break;
         case cppcEnd : level_idx = close_level( level_idx, iy );  break;
         default      : break;
         }
      }
   while( level_idx > -1 ) {
      level_idx = close_level( level_idx, ViewLines()-1 );
      }
   d_need_refresh = false;
   }

bool HiliteAddin_cond_CPP::VHilitLine( LINE yLine, COL xIndent, LineColorsClipped &alcc ) {
   try {
      if( d_need_refresh ) { // BUGBUG fix this!!!!!!!!!!
         refresh( 0, 0 );
         }
      const auto lineInWindow( yLine - Origin().lin );
      Assert( lineInWindow >= 0 && lineInWindow < ViewLines() );
      const auto &line( d_PerViewableLine[ lineInWindow ].line );
      // highlight any CPPcond that occurs on this line
      if( cppcNone != line.acppc ) {
         alcc.PutColor( line.xPound, d_PerViewableLine[ line.level_ix ].level.xBox - line.xPound, ColorTblIdx::CPP );
         }
      // continue any "surrounds" of other highlit CPPconds above/below
      for( auto level_idx(line.level_ix) ; level_idx > -1 ; level_idx = d_PerViewableLine[ level_idx ].level.containing_level_idx ) {
         const auto xBar( d_PerViewableLine[ level_idx ].level.xBox );
         alcc.PutColor( xBar, 1, ColorTblIdx::CPP );
         }
      }
   catch( const std::out_of_range& exc ) {
      Msg( "%s caught std::out_of_range %s", __func__, exc.what() );
      }
   catch( ... ) {
      Msg( "%s caught other exception", __func__ );
      }
   return false;
   }

#if 1

   #if 1

   #else

      #if 1

      #else

      #endif

   #endif

#else

   #if 1

   # elif 1==1

   # else

   #endif

#endif

class HiliteAddin_cond_gmake : public HiliteAddin_cond_CPP {
   cppc IsCppConditional( stref src, int *pxPound ) override { return ::IsGnuMakeConditional( src, pxPound ); }
public:
   HiliteAddin_cond_gmake( PView pView )
      : HiliteAddin_cond_CPP( pView )
      {
      }
   ~HiliteAddin_cond_gmake() {}
   PCChar Name() const override { return "cond_make"; }
   };

void View::Set_LineCompile( LINE yLine ) {
   if( !d_LineCompile_isValid || d_LineCompile != yLine ) {
      HiliteAddins_Init();
      MoveCursor_NoUpdtWUC( yLine, 0 );
      d_LineCompile = yLine;
      }
   d_LineCompile_isValid = true;
   }

#define  USE_HiliteAddin_CompileLine  1
#if      USE_HiliteAddin_CompileLine
class HiliteAddin_CompileLine : public HiliteAddin {
   bool VHilitLine   ( LINE yLine, COL xIndent, LineColorsClipped &alcc ) override;
public:
   HiliteAddin_CompileLine( PView pView ) : HiliteAddin( pView ) { }
   ~HiliteAddin_CompileLine() {}
   PCChar Name() const override { return "CompileLine"; }
   };
bool HiliteAddin_CompileLine::VHilitLine( LINE yLine, COL xIndent, LineColorsClipped &alcc ) {
   if( d_view.Get_LineCompile() == yLine ) {
      alcc.PutColor( 0, COL_MAX, ColorTblIdx::CXY );
      }
   return false;
   }
#endif//USE_HiliteAddin_CompileLine

//=============================================================================

// HiliteAddin_EolComment REQUIRES existence of d_view.CFBuf()->GetFTypeSettings()->d_eolCommentDelim
// HiliteAddin_EolComment & HiliteAddin_StreamParse are mutually exclusive; see HiliteAddins_Init()

class HiliteAddin_EolComment : public HiliteAddin {
   bool VHilitLine   ( LINE yLine, COL xIndent, LineColorsClipped &alcc ) override;
   std::string       d_eolCommentDelim;
   stref d_eolCommentDelimWOTrailSpcs;
public:
   HiliteAddin_EolComment( PView pView )
   : HiliteAddin( pView )
   , d_eolCommentDelim( d_view.CFBuf()->GetFTypeSettings() ? d_view.CFBuf()->GetFTypeSettings()->d_eolCommentDelim : "?" )
      { /* 20140630
        all the following annoying hackiness is to allow detection of EOL
        comments occurring at EOL, in the case where the d_eolCommentDelim has
        been kludged to contain trailing spaces in order to avoid false positives
        (the sole example of this case is DOS/Windows bat files' "rem " to-eol
        comment delim (which really isn't to-eol anyway!)

        The better part of valor might be to delete support for this particular comment!
        */
      auto trailSpcs = 0;
      for( auto it( d_eolCommentDelim.crbegin() ) ; it != d_eolCommentDelim.crend() && *it == ' ' ; ++it ) {
         ++trailSpcs;
         }
      if( trailSpcs ) { d_eolCommentDelimWOTrailSpcs = stref( d_eolCommentDelim.data(), d_eolCommentDelim.length() - trailSpcs ); }
      }
   ~HiliteAddin_EolComment() {}
   PCChar Name() const override { return "EolComment"; }
   };

bool HiliteAddin_EolComment::VHilitLine( LINE yLine, COL xIndent, LineColorsClipped &alcc ) {
   const auto rl( CFBuf()->PeekRawLine( yLine ) );
   if( !rl.empty() ) {
      /* shortcoming: literal strings ARE NOT parsed (in order to ignore comment
         delimiters occurring within them).  HiliteAddin_StreamParse and
         children do this and are thus preferred if/when the language of the file
         is known */
      auto ixTgt( rl.find( d_eolCommentDelim ) );
      if( ixTgt == stref::npos && !d_eolCommentDelimWOTrailSpcs.empty() ) {
         if( rl.ends_with( d_eolCommentDelimWOTrailSpcs ) ) {
            ixTgt = rl.length() - d_eolCommentDelimWOTrailSpcs.length();
            }
         }
      if( ixTgt != stref::npos ) {
         const auto tw( CFBuf()->TabWidth() );
         const auto xC  ( ColOfFreeIdx( tw, rl, ixTgt                        ) );
         const auto xPWS( ColOfFreeIdx( tw, rl, rl.find_last_not_of( SPCTAB ) ) );
         alcc.PutColor( xC, xPWS - xC + 1, ColorTblIdx::COM ); // len extends 1 char into dead space beyond line text: is cosmetically appealing
         }
      }
   return false;
   }

/* 20140625 HiliteAddin_StreamParse -> HiliteAddin_C_Comment, HiliteAddin_Lua_Comment
this is a brute force design: the entire file content is scanned TWICE each time refresh() is called:
PASS 1: determine # of comments, allocate array of comment entries
PASS 2: initialize array of comment entries with actual file-location values

Issues:

A. refresh() is called each time the user edits a file (enters a single character), and calls
   scan_pass() 2x!!!  Obvly some strategies could optz this:
   1. scan only from edit point (not from BoF); obvly if edit point is top of a huge file, no optzn occurs.
   2. IMPLEMENTED: scan only past end of current view (not to EoF).
   3. IMPLEMENTED: refresh() recycles array heap alloc when possible (which it typically is).
   4. IMPLEMENTED: minimize computation occurring on PASS 1: arg "efficiency hack!" psearch
   5. IMPLEMENTED: don't resize array downward; reuse (larger than necessary) array.
   6. IMPLEMENTED: alloc more than min necessary to avoid freq reallocs on scroll-down.

B. Array-based impl forces 2-pass design; a dlink design was tried first, but of course causes (for
   a typically well-commented source file) 100's or 1000's of allocs (and frees), with alloc
   granule-size increased from 4 to 6 32-bit entries (50% size increase); also these tiny allocs tend
   be inefficient consumers of RAM (due to heap alloc-atom policies and per-block-allocated heap mgmt
   overhead).  This plus the CPU-cache/-read-lookahead benefit described by Sutter (concl: prefer
   arrays, to discontig/scattered dstructs in basically ALL cases where they are traversed linearly)
   led to the 2-pass array-alloc design.

C. a serious underlying problem: we do not have robust detection of "FBUF content changed" event
   vs. "redrawing screen" event, which leads to excessive numbers of the latter being used in lieu
   of the former.
*/

class HiliteAddin_StreamParse : public HiliteAddin {
   bool VHilitLine   ( LINE yLine, COL xIndent, LineColorsClipped &alcc ) override;
   void VFbufLinesChanged( LINE yMin, LINE yMax ) override;
   bool d_counting = false; // scan_pass() called 2x: 1: counting, 2: collecting
   unsigned d_num_hl_rgns_found = 0;
   std::vector<hl_rgn_t> d_hl_rgns;
   void add_hl_rgn( int color, LINE yulc, COL xulc, LINE ylrc, COL xlrc ) {
      if( d_counting ) {
         ++d_num_hl_rgns_found;
         }
      else { 0 && DBG( "%02X: %d,%d-%d,%d", color, yulc, xulc, ylrc, xlrc );
         d_hl_rgns.emplace_back( color, yulc, xulc, ylrc, xlrc );
         }
      }
   unsigned d_FbufContentRev = 0;
   enum { SCAN_ALL_LINES=1 }; // SCAN_ALL_LINES is set to 1 so that, when we scroll downward in a file, the hilite metadata will be there (BUGBUG need fixing)
protected:
   void refresh();
   virtual void scan_pass( LINE yMaxScan ) = 0; // yes this is an ABSTRACT BASE CLASS!
   void add_comment( LINE yulc, COL xulc, LINE ylrc, COL xlrc ) { add_hl_rgn( ColorTblIdx::COM, yulc, xulc, ylrc, xlrc ); }
   void add_litstr ( LINE yulc, COL xulc, LINE ylrc, COL xlrc ) { add_hl_rgn( ColorTblIdx::STR, yulc, xulc, ylrc, xlrc ); }
   // "caching" speeds VHilitLine lookup
   unsigned  d_cacheIdx = 0; // d_hl_rgns[d_cacheIdx] is ...
   LINE      d_cacheValAtIdx = 0;  // ... entry for d_cacheValAtIdx
public:
   HiliteAddin_StreamParse( PView pView ) : HiliteAddin( pView ) {}
   ~HiliteAddin_StreamParse() {}
   size_t VGetStreamParse( LINE yLine, hl_rgn_t *&hlrt ) override;
   };

size_t HiliteAddin_StreamParse::VGetStreamParse( LINE yLine, hl_rgn_t *&hlrt ) { return 0; }

bool HiliteAddin_StreamParse::VHilitLine( const LINE yLine, const COL xIndent, LineColorsClipped &alcc ) {
   if( 0 == d_hl_rgns.size() ) { return false; }
   const auto ixStart( (d_cacheValAtIdx <= yLine) ? d_cacheIdx : 0 );
   auto hl_line = [&]( unsigned ix ) { 0 && DBG( "yLine=%d [%d->%d]: [%d..%d]", yLine, ixStart, ix, d_hl_rgns[ix].rgn.flMin.lin, d_hl_rgns[ix].rgn.flMax.lin );
      d_cacheIdx = ix;
      if( yLine < d_hl_rgns[ix].rgn.flMin.lin ) { return false; }
      const auto pFile( CFBuf() );
      const auto rl( pFile->PeekRawLine( yLine ) );
      if( !IsStringBlank( rl ) ) {
         const auto tw( pFile->TabWidth() );
         const auto xMaxOfLine( ColOfFreeIdx( tw, rl, rl.length() - 1 ) );
         for( ; ix < d_hl_rgns.size() && 0==d_hl_rgns[ix].rgn.cmp_line( yLine ) ; ++ix ) {
            const auto &hl( d_hl_rgns[ix] );
            const auto xMin( hl.rgn.flMin.lin == yLine ? ColOfFreeIdx( tw, rl, hl.rgn.flMin.col ) :          0 );
            const auto xMax( hl.rgn.flMax.lin == yLine ? ColOfFreeIdx( tw, rl, hl.rgn.flMax.col ) : xMaxOfLine );
            0 && DBG( "hl %d [%d] %d L %d", yLine, ix, xMin, xMax-xMin+1 );
            alcc.PutColor( xMin, xMax-xMin+1, hl.color );
            }
         }
      return false;
      };
   d_cacheIdx = d_hl_rgns.size()+1; // invalid
   d_cacheValAtIdx = yLine;
   for( auto ix=ixStart ; ix < d_hl_rgns.size() ; ++ix ) {
      const auto cmp( d_hl_rgns[ix].rgn.cmp_line( yLine ) );
      if( cmp ==0 ) { 0 && DBG("direct hit"); return hl_line( ix );               }
      if( cmp < 0 ) { 0 && DBG("past hit"  ); return hl_line( ix==0 ? 0 : ix-1 ); }
      }
   return false;
   }

void HiliteAddin_StreamParse::refresh() { 0 && DBG( "%s", __FUNCTION__ );
   // try to reuse alloc: transfer to save_, act as if deleted
   const auto yMaxScan( SCAN_ALL_LINES ? CFBuf()->LastLine() : MaxVisibleFbufLine() );
   d_counting = true;
   d_num_hl_rgns_found = 0;
   scan_pass( yMaxScan );
   d_counting = false;
   d_hl_rgns.clear();
   d_hl_rgns.reserve( d_num_hl_rgns_found );
   if( d_num_hl_rgns_found ) {
      d_cacheIdx = 0;
      scan_pass( yMaxScan );
      }
   }

void HiliteAddin_StreamParse::VFbufLinesChanged( LINE yMin, LINE yMax ) {
   const auto newRev( CFBuf()->CurrContentRevision() );
   if( newRev != d_FbufContentRev ) { 0 && DBG( "refresh %u != %u", newRev, d_FbufContentRev );
      d_FbufContentRev = newRev;
      refresh();
      }
   }

//=============================================================================

class HiliteAddin_clang : public HiliteAddin_StreamParse {
   void scan_pass( LINE yMaxScan ) override;
   enum scan_rv { atEOF, in_code, in_1Qstr, in_2Qstr, in_comment };
   // scan_pass() methods; all must have same proto as called via pfx
   scan_rv find_end_code    ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_1Qstr   ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_2Qstr   ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_comment ( PCFBUF pFile, Point &pt ) ;
   Point d_start_C; // where last /* comment started
public:
   HiliteAddin_clang( PView pView ) : HiliteAddin_StreamParse( pView ) { refresh(); }
   ~HiliteAddin_clang() {}
   PCChar Name() const override { return "C_Comment"; }
   };

#define START_LINE_X()  const auto rl( pFile->PeekRawLine( pt.lin ) );

HiliteAddin_clang::scan_rv HiliteAddin_clang::find_end_code( PCFBUF pFile, Point &pt ) {
/* some */0 && DBG(/* tests */"FNNC @y=%d x=%d", pt.lin, pt.col );/* here */
   0 && DBG("FNNC @y=%d x=%d", pt.lin, pt.col );
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()
      for( ; pt.col < rl.length() ; ++pt.col ) {
         switch( rl[pt.col] ) {
            default:      break;
            case chQuot1: ++pt.col; return in_1Qstr;
            case chQuot2: ++pt.col; return in_2Qstr;
            case '/':     if( pt.col+1 < rl.length() ) switch( rl[pt.col+1] ) { default: break;
                             case '/': { // start of to-EOL comment?
                                add_comment( pt.lin, pt.col, pt.lin, rl.length() );
                                goto NEXT_LINE;
                                }
                             case '*': { // start of C comment?
                                d_start_C.Set( pt.lin, pt.col );
                                pt.col += 2;
                                return in_comment;
                                }
                             }
                          break;
            }
         }
NEXT_LINE: ;
      }
   return atEOF;
   }

HiliteAddin_clang::scan_rv HiliteAddin_clang::find_end_comment( PCFBUF pFile, Point &pt ) {
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()
      for( ; pt.col < rl.length() ; ++pt.col ) {
         switch( rl[pt.col] ) {
            default:   break;
            case '*':  if( pt.col+1 < rl.length() ) switch( rl[pt.col+1] ) { default: break;
                          case '/': { // end of C comment?
                             pt.col += 2;
                             add_comment( d_start_C.lin, d_start_C.col, pt.lin, pt.col-1 );
                             return in_code;
                             }
                          }
                       break;
            }
         }
      }
   return atEOF;
   }

// although C/C++ requirements for single and double quoted literals are different,
// they are similar enough that identical parsing code can be used for both:
#define find_end_Qstr( class, nm, delim )                                      \
class::scan_rv class::nm( PCFBUF pFile, Point &pt ) {                          \
   const auto start( pt );                                                     \
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()  \
      for( ; pt.col < rl.length() ; ++pt.col ) {                               \
         switch( rl[pt.col] ) {                                                \
            default:     break;                                                \
            case chESC:  ++pt.col; break; /* skip escaped char */              \
            case delim:  add_litstr( start.lin, start.col, pt.lin, pt.col-1 ); \
                         ++pt.col;                                             \
                         return in_code;                                       \
            }                                                                  \
         }                                                                     \
      }                                                                        \
   return atEOF;                                                               \
   }
       find_end_Qstr( HiliteAddin_clang, find_end_1Qstr, chQuot1 )
       find_end_Qstr( HiliteAddin_clang, find_end_2Qstr, chQuot2 )

void HiliteAddin_clang::scan_pass( LINE yMaxScan ) {
   auto fb( CFBuf() );
   Point pt( 0, 0 );  // start @ top of file
   typedef scan_rv (HiliteAddin_clang::*pfx_findnext)( PCFBUF pFile, Point &pt );
   pfx_findnext findnext = &HiliteAddin_clang::find_end_code;
   scan_rv prevret = in_code;
   while( pt.lin <= yMaxScan ) {
      const auto ret( CALL_METHOD( *this, findnext )( fb, pt ) );
      0 && DBG( "@y=%d x=%d: %d", pt.lin, pt.col, ret );
      if( prevret == ret ) {    DBG("internal error seql==rv" )            ; return; }
      switch( ret ) { default : DBG("internal error unknwn ret" )          ; return;
         case in_code    : findnext = &HiliteAddin_clang::find_end_code    ; break;
         case in_1Qstr   : findnext = &HiliteAddin_clang::find_end_1Qstr   ; break;
         case in_2Qstr   : findnext = &HiliteAddin_clang::find_end_2Qstr   ; break;
         case in_comment : findnext = &HiliteAddin_clang::find_end_comment ; break;
         case atEOF      : if( in_comment==prevret ) { 0 && DBG( "atEOF+in_comment @y=%d x=%d", pt.lin, pt.col );
                              add_comment( d_start_C.lin, d_start_C.col, pt.lin, 0 );
                              }
                           return;
         }
      prevret = ret;
      }
   }

//=============================================================================

class HiliteAddin_lua : public HiliteAddin_StreamParse {
   void scan_pass( LINE yMaxScan ) override;

   enum scan_rv { atEOF, in_code, in_1Qstr, in_2Qstr, in_long };
   // scan_pass() methods; all must have same proto as called via pfx
   scan_rv find_end_code    ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_1Qstr   ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_2Qstr   ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_long    ( PCFBUF pFile, Point &pt ) ;
   Point    d_start_C; // where last /* comment started
   unsigned d_long_level = 0;
   bool     d_long_comment = false; // if false, is long string

public:
   HiliteAddin_lua( PView pView ) : HiliteAddin_StreamParse( pView ) { refresh(); }
   ~HiliteAddin_lua() {}
   PCChar Name() const override { return "Lua_Comment"; }
   };

STATIC_FXN int Lua_long_level( stref rl, sridx ix ) { // ix indexes PAST first LSQ (which has already been detected, thus Lua_long_level is being called)
   for( auto num_eqs( 0 ) ; ix < rl.length() ; ++ix ) {
      switch( rl[ix] ) {
         case '='  :      ++num_eqs; break;
         case chLSQ: return num_eqs;
         default   : return -1;
         }
      }
   return -1;
   }

HiliteAddin_lua::scan_rv HiliteAddin_lua::find_end_code( PCFBUF pFile, Point &pt ) {
   0 && DBG("FNNC @y=%d x=%d", pt.lin, pt.col );
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()
      for( ; pt.col < rl.length() ; ++pt.col ) {
         switch( rl[pt.col] ) {
            default:     break;
            case chQuot1: ++pt.col;  return in_1Qstr;
            case chQuot2: ++pt.col;  return in_2Qstr;
            case chLSQ: { const auto long_level( Lua_long_level( rl, pt.col+1 ) );
                          if( long_level >= 0 ) { // Lua long string
                             d_long_comment = false;
                             d_long_level = long_level; // may == 0
                             if( pt.col+1+d_long_level+2 < rl.length() ) {
                                const auto lenSkip( d_long_level+2 );
                                d_start_C.Set( pt.lin, pt.col+lenSkip );
                                pt.col += (1+lenSkip) + 1;
                                }
                             else { // Lua long string opening delim immed followed by \n discards this initial \n
                                pt.lin++;
                                pt.col = 0;
                                d_start_C.Set( pt.lin, pt.col );
                                }
                             return in_long;
                             }
                        } break;
            case '-':     if( pt.col+1 < rl.length() && '-'==rl[1+pt.col] ) {
                             auto ie( pt.col+2 );
                             if( ie < rl.length() && chLSQ==rl[ie++] ) {
                                const auto long_level( Lua_long_level( rl, ie ) );
                                if( long_level >= 0 ) {
                                   d_long_comment = true;
                                   d_long_level = long_level; // may == 0
                                   d_start_C.Set( pt.lin, pt.col );
                                   pt.col += (1+d_long_level+2) + 1;
                                   return in_long;
                                   }
                                }
                             add_comment( pt.lin, pt.col, pt.lin, rl.length() );
                             goto NEXT_LINE;
                             }
                       break;
            }
         }
NEXT_LINE: ;
      }
   return atEOF;
   }

HiliteAddin_lua::scan_rv HiliteAddin_lua::find_end_long( PCFBUF pFile, Point &pt ) {
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()
      for( ; pt.col < rl.length() ; ++pt.col ) {
         switch( rl[pt.col] ) {
            default:     break;
            case chRSQ:  if( pt.col+d_long_level+1 < rl.length() ) { // long enuf to contain what we're looking for?
                            auto ix( 0 ); // look for d_long_level '=' chars followed by chRSQ
                            for( ; ix < d_long_level ; ++ix ) {
                               if( '=' != rl[1+ix+pt.col] ) {
                                  break;
                                  }
                               }
                            if( ix == d_long_level && chRSQ == rl[1+ix+pt.col] ) { // end of long comment/string
                               pt.col += d_long_level + 2;
                               if( d_long_comment ) { add_comment( d_start_C.lin, d_start_C.col, pt.lin, pt.col-1 ); }
                               else                 { add_litstr ( d_start_C.lin, d_start_C.col, pt.lin, pt.col-3 ); }
                               return in_code;
                               }
                            }
                         break;
            }
         }
      }
   return atEOF;
   }

       find_end_Qstr( HiliteAddin_lua, find_end_1Qstr, chQuot1 )
       find_end_Qstr( HiliteAddin_lua, find_end_2Qstr, chQuot2 )

#undef find_end_Qstr

void HiliteAddin_lua::scan_pass( LINE yMaxScan ) {
   auto fb( CFBuf() );
   Point pt( 0, 0 );  // start @ top of file
   typedef scan_rv (HiliteAddin_lua::*pfx_findnext)( PCFBUF pFile, Point &pt );
   pfx_findnext findnext = &HiliteAddin_lua::find_end_code;
   scan_rv prevret = in_code;
   while( pt.lin <= yMaxScan ) {
      const auto ret( CALL_METHOD( *this, findnext )( fb, pt ) );
      0 && DBG( "@y=%d x=%d: %d", pt.lin, pt.col, ret );
      if( prevret == ret ) {    DBG("internal error seql==rv" )                ; return; }
      switch( ret ) { default : DBG("internal error unknwn ret" )              ; return;
         case in_code    : findnext = &HiliteAddin_lua::find_end_code  ; break;
         case in_1Qstr   : findnext = &HiliteAddin_lua::find_end_1Qstr ; break;
         case in_2Qstr   : findnext = &HiliteAddin_lua::find_end_2Qstr ; break;
         case in_long    : findnext = &HiliteAddin_lua::find_end_long  ; break;
         case atEOF      : if( in_long==prevret ) { 0 && DBG( "atEOF+in_long @y=%d x=%d", pt.lin, pt.col );
                              add_comment( d_start_C.lin, d_start_C.col, pt.lin, 0 );
                              }
                           return;
         }
      prevret = ret;
      }
   }
//=============================================================================

class HiliteAddin_python : public HiliteAddin_StreamParse {
   void scan_pass( LINE yMaxScan ) override;
   enum scan_rv { atEOF, in_code,
      in_1Qstr    , // https://docs.python.org/2/reference/lexical_analysis.html#string-literals
      in_1Qrstr   ,
      in_1Q3str   ,
      in_1Q3rstr  ,
      in_2Qstr    ,
      in_2Qrstr   ,
      in_2Q3str   ,
      in_2Q3rstr  ,
   };
   // scan_pass() methods; all must have same proto as called via pfx
   scan_rv find_end_code    ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_1Qstr   ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_1Qrstr  ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_1Q3str  ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_1Q3rstr ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_2Qstr   ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_2Q3str  ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_2Qrstr  ( PCFBUF pFile, Point &pt ) ;
   scan_rv find_end_2Q3rstr ( PCFBUF pFile, Point &pt ) ;
   Point    d_start_C; // where last /* comment started
   bool     d_in_3str = false;
public:
   HiliteAddin_python( PView pView ) : HiliteAddin_StreamParse( pView ) { refresh(); }
   ~HiliteAddin_python() {}
   PCChar Name() const override { return "Python_Comment"; }
   };

#define SEQ3( first )   (pt.col+2 < rl.length() && first==rl[1+pt.col] && first==rl[2+pt.col])

HiliteAddin_python::scan_rv HiliteAddin_python::find_end_code( PCFBUF pFile, Point &pt ) {
   0 && DBG("FNNC @y=%d x=%d", pt.lin, pt.col );
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()
      for( ; pt.col < rl.length() ; ++pt.col ) {
         switch( rl[pt.col] ) {
            default:      break;
            case chQuot1: //lint -fallthrough
            case chQuot2: {
                          const auto qch( rl[pt.col] );
                          const auto raw( pt.col>0 && 'r'==tolower( rl[pt.col-1] ) );
                          const auto nquotes( SEQ3( qch ) ? 3 : 1 );
                          pt.col += nquotes;
                          if( 1==nquotes ) {
                             if( chQuot1==qch ) { return raw ? in_1Qrstr  : in_1Qstr  ; }
                             else               { return raw ? in_2Qrstr  : in_2Qstr  ; }
                             }
                          else {
                             d_start_C.Set( pt.lin, pt.col ); d_in_3str = true;
                             if( chQuot1==qch ) { return raw ? in_1Q3rstr : in_1Q3str ; }
                             else               { return raw ? in_2Q3rstr : in_2Q3str ; }
                             }
                          }
                          break;
            case '#':     add_comment( pt.lin, pt.col, pt.lin, rl.length() );
                          goto NEXT_LINE;
            }
         }
NEXT_LINE: ;
      }
   return atEOF;
   }

#define find_end_Qstr1e( class, nm, delim )                                    \
class::scan_rv class::nm( PCFBUF pFile, Point &pt ) {                          \
   const auto start( pt );                                                     \
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()  \
      for( ; pt.col < rl.length() ; ++pt.col ) {                               \
         switch( rl[pt.col] ) {                                                \
            default:     break;                                                \
            case chESC:  ++pt.col; break; /* skip escaped char */              \
            case delim:  add_litstr( start.lin, start.col, pt.lin, pt.col-1 ); \
                         ++pt.col;                                             \
                         return in_code;                                       \
            }                                                                  \
         }                                                                     \
      }                                                                        \
   return atEOF;                                                               \
   }

#define find_end_Qstr1r( class, nm, delim )                                    \
class::scan_rv class::nm( PCFBUF pFile, Point &pt ) {                          \
   const auto start( pt );                                                     \
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()  \
      for( ; pt.col < rl.length() ; ++pt.col ) {                               \
         switch( rl[pt.col] ) {                                                \
            default:     break;                                                \
            case delim:  add_litstr( start.lin, start.col, pt.lin, pt.col-1 ); \
                         ++pt.col;                                             \
                         return in_code;                                       \
            }                                                                  \
         }                                                                     \
      }                                                                        \
   return atEOF;                                                               \
   }

find_end_Qstr1e( HiliteAddin_python, find_end_1Qstr , chQuot1 )
find_end_Qstr1e( HiliteAddin_python, find_end_2Qstr , chQuot2 )
find_end_Qstr1r( HiliteAddin_python, find_end_1Qrstr, chQuot1 )
find_end_Qstr1r( HiliteAddin_python, find_end_2Qrstr, chQuot2 )

#undef find_end_Qstr1e
#undef find_end_Qstr1r

#define find_end_Qstr3e( class, nm, delim, backslash_escapes )                                    \
class::scan_rv class::nm( PCFBUF pFile, Point &pt ) {                                             \
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()                     \
      for( ; pt.col < rl.length() ; ++pt.col ) {                                                  \
         switch( rl[pt.col] ) {                                                                   \
            default:     break;                                                                   \
            case chESC:  if( backslash_escapes ) { ++pt.col; } /* skip escaped char */            \
                         break;                                                                   \
            case delim:  const auto first( rl[pt.col] );                                          \
                         if( SEQ3( first ) ) {                                                    \
                            add_litstr( d_start_C.lin, d_start_C.col, pt.lin, pt.col-1 );         \
                            pt.col += 4;                                                          \
                            d_in_3str = false;                                                    \
                            return in_code;                                                       \
                            }                                                                     \
                         break;                                                                   \
            }                                                                                     \
         }                                                                                        \
      }                                                                                           \
   return atEOF;                                                                                  \
   }

       find_end_Qstr3e( HiliteAddin_python, find_end_1Q3str , chQuot1, true  );
       find_end_Qstr3e( HiliteAddin_python, find_end_2Q3str , chQuot2, true  );
       find_end_Qstr3e( HiliteAddin_python, find_end_1Q3rstr, chQuot1, false );
       find_end_Qstr3e( HiliteAddin_python, find_end_2Q3rstr, chQuot2, false );
#undef find_end_Qstr3e

void HiliteAddin_python::scan_pass( LINE yMaxScan ) {
   auto fb( CFBuf() );
   Point pt( 0, 0 );  // start @ top of file
   typedef scan_rv (HiliteAddin_python::*pfx_findnext)( PCFBUF pFile, Point &pt );
   pfx_findnext findnext = &HiliteAddin_python::find_end_code;
   d_in_3str = false;
   scan_rv prevret = in_code;
   while( pt.lin <= yMaxScan ) {
      const auto ret( CALL_METHOD( *this, findnext )( fb, pt ) );
      0 && DBG( "@y=%d x=%d: %d", pt.lin, pt.col, ret );
      if( prevret == ret ) {    DBG("internal error seql==rv" )                     ; return; }
      switch( ret ) { default : DBG("internal error unknwn ret" )                   ; return;
         case in_code    : findnext = &HiliteAddin_python::find_end_code    ; break;
         case in_1Qstr   : findnext = &HiliteAddin_python::find_end_1Qstr   ; break;
         case in_1Qrstr  : findnext = &HiliteAddin_python::find_end_1Qrstr  ; break;
         case in_1Q3str  : findnext = &HiliteAddin_python::find_end_1Q3str  ; break;
         case in_1Q3rstr : findnext = &HiliteAddin_python::find_end_1Q3rstr ; break;
         case in_2Qstr   : findnext = &HiliteAddin_python::find_end_2Qstr   ; break;
         case in_2Qrstr  : findnext = &HiliteAddin_python::find_end_2Qrstr  ; break;
         case in_2Q3str  : findnext = &HiliteAddin_python::find_end_2Q3str  ; break;
         case in_2Q3rstr : findnext = &HiliteAddin_python::find_end_2Q3rstr ; break;
         case atEOF      : if( d_in_3str ) { 0 && DBG( "atEOF+d_in_3str @y=%d x=%d", pt.lin, pt.col );
                              add_litstr( d_start_C.lin, d_start_C.col, pt.lin, 0 );
                              }
                           return;
         }
      prevret = ret;
      }
   }

//=============================================================================
#if 1

// bash string quoting resembles Perl's (" supports \escaping,' does not), however a major difference is that
// quoting is NESTED:
// "this 'is "a" quoted' string"
//
class HiliteAddin_bash : public HiliteAddin_StreamParse {
   enum { DBBASH=0 };
   void scan_pass( LINE yMaxScan ) override;
   enum scan_rv { atEOF, in_code,
      in_1Qstr    , // https://docs.python.org/2/reference/lexical_analysis.html#string-literals
      in_2Qstr    ,
      };
   // scan_pass() methods; all must have same proto as called via pfx
   scan_rv find_end_code    ( PCFBUF pFile, Point &pt, int nest );
   scan_rv find_end_chQuot1 ( PCFBUF pFile, Point &pt, int nest );
   scan_rv find_end_chQuot2 ( PCFBUF pFile, Point &pt, int nest );
public:
   HiliteAddin_bash( PView pView ) : HiliteAddin_StreamParse( pView ) { refresh(); }
   ~HiliteAddin_bash() {}
   PCChar Name() const override { return "Python_Comment"; }
   };

HiliteAddin_bash::scan_rv HiliteAddin_bash::find_end_code( PCFBUF pFile, Point &pt, int nest ) { enum { DB=DBBASH };
   0 && DBG("FNNC @y=%d x=%d", pt.lin, pt.col );
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()
      for( ; pt.col < rl.length() ; ++pt.col ) {                                   DB && DBG( "%s[:%c] y/x=%d,%d", __func__, rl[pt.col], pt.lin, pt.col );
         switch( rl[pt.col] ) {
            default:      break;
            case chQuot1: ++pt.col; return in_1Qstr;
            case chQuot2: ++pt.col; return in_2Qstr;
            case '#':     add_comment( pt.lin, pt.col, pt.lin, rl.length() );
                          goto NEXT_LINE;
            }
         }
NEXT_LINE: ;
      }
   return atEOF;
   }

#define def_find_end_str_nest( class, pri, sec ) \
class::scan_rv class::find_end_ ## pri( PCFBUF pFile, Point &pt, int nest ) { enum { DB=DBBASH };                                                     \
   const auto start( pt );                                                         DB && DBG( "%s[+%d] y/x=%d,%d", __func__, nest, pt.lin, pt.col );  \
   for( ; pt.lin <= pFile->LastLine() ; ++pt.lin, pt.col=0 ) { START_LINE_X()                                                                         \
      for( ; pt.col < rl.length() ; ++pt.col ) {                                   DB && DBG( "%s[:%c] y/x=%d,%d", __func__, rl[pt.col], pt.lin, pt.col ); \
         switch( rl[pt.col] ) {                                                                                                                       \
            default:      break;                                                                                                                      \
            case chESC:   ++pt.col; /* skip escaped char */ break;                                                                                    \
            case sec:     ++pt.col;                                                                                                                   \
                          find_end_ ## sec( pFile, pt, nest+1 );                   DB && DBG( "%s[=%d] y/x=%d,%d", __func__, nest, pt.lin, pt.col );  \
                          --pt.col; /* compensate for ++pt.col in 3d for clause */                                                                    \
                          break;                                                                                                                      \
            case pri:     if( 0==nest ) {                                                                                                             \
                             add_litstr( start.lin, start.col, pt.lin, pt.col-1 );                                                                    \
                             }                                                     DB && DBG( "%s[-%d] y/x=%d,%d", __func__, nest, pt.lin, pt.col );  \
                          ++pt.col;                                                                                                                   \
                          return in_code;                                                                                                             \
            }                                                                                                                                         \
         }                                                                                                                                            \
      }                                                                                                                                               \
   return atEOF;                                                                                                                                      \
   }

def_find_end_str_nest( HiliteAddin_bash, chQuot2, chQuot1 )
def_find_end_str_nest( HiliteAddin_bash, chQuot1, chQuot2 )

#undef def_find_end_str_nest

void HiliteAddin_bash::scan_pass( LINE yMaxScan ) {
   auto fb( CFBuf() );
   Point pt( 0, 0 );  // start @ top of file
   typedef scan_rv (HiliteAddin_bash::*pfx_findnext)( PCFBUF pFile, Point &pt, int nest );
   pfx_findnext findnext = &HiliteAddin_bash::find_end_code;
   scan_rv prevret = in_code;
   while( pt.lin <= yMaxScan ) {
      const auto ret( CALL_METHOD( *this, findnext )( fb, pt, 0 ) );
      DBBASH && DBG( "@y=%d x=%d: %d", pt.lin, pt.col, ret );
      if( prevret == ret ) {    DBG("internal error seql==rv" )           ; return; }
      switch( ret ) { default : DBG("internal error unknwn ret" )         ; return;
         case in_code    : findnext = &HiliteAddin_bash::find_end_code    ; break;
         case in_1Qstr   : findnext = &HiliteAddin_bash::find_end_chQuot1 ; break;
         case in_2Qstr   : findnext = &HiliteAddin_bash::find_end_chQuot2 ; break;
         case atEOF      : return;
         }
      prevret = ret;
      }
   }

#endif
//=============================================================================

class HiliteAddin_Diff : public HiliteAddin {
   bool VHilitLine   ( LINE yLine, COL xIndent, LineColorsClipped &alcc ) override;
public:
   STATIC_FXN bool Applies( PFBUF pFile );
   HiliteAddin_Diff( PView pView ) : HiliteAddin( pView ) {}
   ~HiliteAddin_Diff() {}
   PCChar Name() const override { return "Diff"; }
   };

bool HiliteAddin_Diff::VHilitLine( LINE yLine, COL xIndent, LineColorsClipped &alcc ) {
   const auto rl( CFBuf()->PeekRawLine( yLine ) );
   if( !rl.empty() ) {
           if( '+' == rl[0] ) { alcc.PutColorRaw( 0, COL_MAX, bgBLK|fgGRN|FGhi ); /*DBG( "+" );*/ }
      else if( '-' == rl[0] ) { alcc.PutColorRaw( 0, COL_MAX, bgBLK|fgRED|FGhi ); /*DBG( "-" );*/ }
      }
   return false;
   }

/*
   HiLite SORT ORDER

     HiLite is the attribute (color) of a region.  We store HiLites in a
     linklist.  The sort order of this list impacts two different list-related
     algorithms (redraw, insert) slightly differently:

   REDRAW

     When we're redrawing the screen, the sort order of the HiLites in the list
     must cause the relevant HiLites (those whose regions affect the redrawn
     area) to be presented in a single pass, preferrably one which terminates
     without requiring a traversal of the entire HiLite list.  The search key
     for a redraw is a single line: we're searching for all HiLites having
     regions containing the key line.

   INSERT

     When we're inserting a HiLite, we need to preserve the sort order required
     for REDRAW, and update the speed table.

   So what does REDRAW require of a sort order?  The ideal goal is that, once
   we've identified the FIRST HiLite in the list which contains the key line,
   as we traverse the list further, we will not encounter any HiLites that
   DON'T contain the key line before we encounter the last HiLite which contains
   the key line.  But this seems impossible; consider three hilites, sorted on
   yFirst:

              yFirst     yLast
              ------     -----
      HL1:      Y        Y+400
      HL2:      Y+1      Y+  2
      HL3:      Y+3      Y+397

   If key line = Y+10, HL1 and HL3 contain the key line, and HL2 does not!  We
   won't find HL3 unless our search algorithm continues searching past a "miss
   HL", HL2.  Our REDRAW search algo needs to potentially consider all HL's
   having yLast >= yKey; unfortunately, we aren't sorting on yLast, but rather
   yFirst!  Looks like a linear search of the whole list is required!

   ----

   But what if we don't have to handle the general case where HiLite regions can
   be overlapping and of arbitrary size?  After all, what kinds of HiLites are
   multiline?  Only the selection HiLite, AFAICT.  The remaining cases hilite
   single line regions.  This is expected to be true "going forward".

   We have codified this use-pattern by offering two APIs for inserting a
   HiLite: InsHiLite1Line(LINE, COL, COL) vs. InsHiLiteBox(Rect).  InsHiLiteBox
   is _only_ called in ExtendSelectionHilite().  The multiline HiLites generated
   by ExtendSelectionHilite() DO NOT OVERLAP.

   Selection HiLites never occur at the same time as any other Hiliting.

   So the invariants that currently exist, but need to be stated explicitly so
   we can be aware when they change, are:

   1) Multiline HiLites NEVER OVERLAP in the Y (line) dimension.

   2) Multiple single-line HiLites MAY occur on the same line, and MAY overlap
      columnwise.

   So the upshot is that, given the above invariants, if we sort on yFirst,
   we'll be OK if for REDRAW we search for first flMin.lin >= yKey, and we end
   our REDRAW scan when flMin.lin >= yKey

   Next question is, how to handle the SpeedTable?
*/

struct HiLiteRec {
   // 'regions' of hilite_rec are WINDOW-relative (ie.  if window is scrolled
   // horizontally, hilite col 0 is the leftmost visible column).
   //
   Rect                  rect;
   int                   colorIndex;
   DLinkEntry<HiLiteRec> dlink;
   HiLiteRec( const Rect &rect_, int colorIndex_ )
      : rect(rect_)
      , colorIndex( colorIndex_ )
      {
      DBGHILITE && DBG( "[HL%p] + HiliteAllc", this );
      }
   };

typedef DLinkHead<HiLiteRec> HiLiteHead;

struct HiLiteSpeedTable {
   const HiLiteRec *pHL = nullptr;  // references, does NOT own!
   };

enum { HILITE_SPEEDTABLE_LINE_INCR = 16 * 1024 };
STIL int SpeedTableIndex( LINE yLine ) {
   const int idx( yLine / HILITE_SPEEDTABLE_LINE_INCR );
   0 && DBG( "SpeedIdx  ==============> %d -> %d", yLine, idx );
   return idx;
   }

STIL int SpeedTableIndexFirstLine( int idx )  { return idx * HILITE_SPEEDTABLE_LINE_INCR; }

class ViewHiLites {
   friend class View;
   const FBUF       &d_FBuf;
   const int         d_SpdTblEls;
   HiLiteSpeedTable *d_SpeedTable;
   HiLiteHead        d_HiLiteList;
   void        UpdtSpeedTbl( HiLiteRec *pThis );
   const HiLiteRec *FindFirstEntryAffectingOrAfterLine( LINE yLine ) const;
   ViewHiLites( PCFBUF pFBuf );
   int  SpeedTableIndex( LINE yLine ) const { return Min( ::SpeedTableIndex( yLine ), d_SpdTblEls-1 ); }
   void vhInsHiLiteBox(   int newColorIdx, Rect newRgn );
   void PrimeRedraw() const;
   int  InsertHiLitesOfLineSeg( LINE yLine, COL Indent, COL xMax, LineColorsClipped &alcc, const HiLiteRec * &pFirstPossibleHiLite ) const;
public:
   ~ViewHiLites(); // the ONLY public member, and it's only needed because View::FreeHiLiteRects calls Delete0 (an STIL utility fxn)
   };

ViewHiLites::ViewHiLites( PCFBUF pFBuf )
   : d_FBuf(*pFBuf)
   , d_SpdTblEls( 1 + ::SpeedTableIndex( d_FBuf.LineCount() ) )
   , d_SpeedTable( new HiLiteSpeedTable [ d_SpdTblEls ] )
   { 0 && DBG( ">>>>>>>>>>> new ViewHiLites[ %d ]", d_SpdTblEls );
   }

//-----------------------------------------------------------------------------

ViewHiLites::~ViewHiLites() {
   PrimeRedraw();
   delete [] d_SpeedTable;  d_SpeedTable = nullptr;  // cannot use Delete0 due to []
   while( auto pEl=d_HiLiteList.front() ) { // zap d_HiLiteList list
      DLINK_REMOVE_FIRST( d_HiLiteList, pEl, dlink );
      delete pEl;
      }
   }

void View::FreeHiLiteRects() {
   Delete0( d_pHiLites );
   }

//-----------------------------------------------------------------------------

STIL void ShowHilite( const HiLiteRec &hl, PCChar str ) {
   DBG( "%s HLy=[%d..%d]"
      , str
      , hl.rect.flMin.lin
      , hl.rect.flMax.lin
      );
   }

const HiLiteRec *ViewHiLites::FindFirstEntryAffectingOrAfterLine( LINE yLine ) const {
   0 && DBG( "FFEAoAL+ %d", yLine );
   for( auto idx(SpeedTableIndex( yLine )) ; idx < d_SpdTblEls ; ++idx ) {
      if( d_SpeedTable[ idx ].pHL ) {
         0 && DBG( "1NZ=%d", idx );
         for( auto pHL=d_SpeedTable[ idx ].pHL; pHL ; pHL=DLINK_NEXT( pHL, dlink ) ) {
            if( yLine <= pHL->rect.flMax.lin ) { // ShowHilite( *pHL, "FFEAoAL-" );
               return pHL;
               }
            }
         break;
         }
      0 && DBG( "FFEAoAL-" );
      }
   return nullptr;
   }

//-----------------------------------------------------------------------------

void ViewHiLites::PrimeRedraw() const {
   DLINKC_FIRST_TO_LASTA( d_HiLiteList, dlink, pHL ) {
      FBOP::PrimeRedrawLineRangeAllWin( &d_FBuf, pHL->rect.flMin.lin, pHL->rect.flMax.lin );
      }
   }

void View::RedrawHiLiteRects() {
   if( d_pHiLites ) {
       d_pHiLites->PrimeRedraw();
       }
   }

//-----------------------------------------------------------------------------

STATIC_FXN bool Rect1ContainsRect2( const Rect &r1, const Rect &r2 ) {
   return (r1.flMin.lin <= r2.flMin.lin)
       && (r1.flMax.lin >= r2.flMax.lin)
       && (r1.flMin.col <= r2.flMin.col)
       && (r1.flMax.col >= r2.flMax.col)
       ;
   }

void ViewHiLites::UpdtSpeedTbl( HiLiteRec *pThis ) {
   const auto idx( SpeedTableIndex( pThis->rect.flMax.lin ) );
   auto &stEntry( d_SpeedTable[ idx ].pHL );
   if( !stEntry ||  pThis->rect.flMax.lin < stEntry->rect.flMax.lin ) {
        stEntry = pThis;
        }
   }

void ViewHiLites::vhInsHiLiteBox( int newColorIdx, Rect newRgn ) {
   DbgHilite( '+' );
   FBOP::PrimeRedrawLineRangeAllWin( &d_FBuf, newRgn.flMin.lin, newRgn.flMax.lin );
   DBGHILITE && DBG( "SetHiLite   c=%02X [%d-%d] [%d-%d]", newColorIdx, newRgn.flMin.lin, newRgn.flMax.lin, newRgn.flMin.col, newRgn.flMax.col );
   auto &Head( d_HiLiteList );
   // walk last to first: THIS IS A LOT FASTER in the heaviest-usage case (searchall) where hilites are added at ever-higher line numbers
   int ix = 0;
   DLINKC_LAST_TO_FIRST( Head, dlink, pThis ) {
      auto &thisRn( pThis->rect );  0 && DBG( "Rn[%d]=%p [%d-%d] [%d-%d]", ++ix, pThis, thisRn.flMin.lin, thisRn.flMax.lin, thisRn.flMin.col, thisRn.flMax.col );
      if( newColorIdx == pThis->colorIndex ) {
         if( Rect1ContainsRect2( newRgn, thisRn ) ) {
            thisRn = newRgn;  // expand
            UpdtSpeedTbl( pThis );
            DbgHilite( '-' );
            return;
            }
         if( Rect1ContainsRect2( thisRn, newRgn ) ) { // already exists: do nothing
            DbgHilite( '-' );
            return;
            }
         }
      if( newRgn.flMin > thisRn.flMin ) {
         auto pNew( new HiLiteRec( newRgn, newColorIdx ) );
         DLINK_INSERT_AFTER( Head, pThis, pNew, dlink );
         UpdtSpeedTbl( pNew );
         DbgHilite( '-' );
         return;
         }
      }
   auto pNew( new HiLiteRec( newRgn, newColorIdx ) );
   DLINK_INSERT_FIRST( Head, pNew, dlink );
   UpdtSpeedTbl( pNew );
   DbgHilite( '-' );
   }

void View::InsHiLiteBox( int newColorIdx, Rect newRgn ) {
   if( !d_pHiLites ) {
        d_pHiLites = new ViewHiLites( CFBuf() );
        }
   d_pHiLites->vhInsHiLiteBox( newColorIdx, newRgn );
   }

void View::InsHiLite1Line( int newColorIdx, LINE yLine, COL xLeft, COL xRight ) {
   Rect newRgn;
   newRgn.flMin.lin = newRgn.flMax.lin = yLine;
   newRgn.flMin.col = xLeft;
   newRgn.flMax.col = xRight;
   InsHiLiteBox( newColorIdx, newRgn );
   }

//-----------------------------------------------------------------------------

int  ViewHiLites::InsertHiLitesOfLineSeg( LINE yLine, COL xIndent, COL xMax, LineColorsClipped &alcc, const HiLiteRec * &pFirstPossibleHiLite ) const
   { 0 && DBG( "IHLoS+ %d", yLine );
   if( !pFirstPossibleHiLite ) {
        pFirstPossibleHiLite = FindFirstEntryAffectingOrAfterLine( yLine );
        }
   auto rv(0);
   for( auto pHL(pFirstPossibleHiLite) ; pHL ; pHL=DLINK_NEXT( pHL, dlink ) ) {
      const auto &Rn( pHL->rect );              0 && DBG( "HL L %5d: c=%02X [%d-%d] [%d-%d]", yLine, pHL->colorIndex, Rn.flMin.lin, Rn.flMax.lin, Rn.flMin.col, Rn.flMax.col );
      const auto lcmp( Rn.cmp_line( yLine ) );
      if( lcmp < 0 ) {  // performance improv: since list is sorted, there's NO reason
         break;         // to walk the whole damned thing when we'll never have another match!
         }
      if( lcmp > 0 ) {
         continue;
         }
      0 && DBG( "HL C? %d %d", Rn.flMin.col, Rn.flMax.col );
      auto rn_xMin( Rn.flMin.col );
      auto rn_xMax( Rn.flMax.col );
      if( rn_xMax < rn_xMin ) {
         const auto tmp( rn_xMin - 1 ); // NOT a simple swap!
         rn_xMin = rn_xMax;
         rn_xMax = tmp;
         }
      if( !(xIndent > rn_xMax || xMax < rn_xMin) ) {
         // be VERY careful when trying to optimize/clarify the xLeft and Len calcs
               auto xLeft( Max( xIndent, rn_xMin )          );
         const auto Len  ( Min( xMax, rn_xMax ) - xLeft + 1 );
                    xLeft -= xIndent;
         0 && DBG( "HL C! %d L %d ci=%02X", xLeft, Len, pHL->colorIndex );
         alcc.PutColor( xLeft+xIndent, Len, pHL->colorIndex );
         ++rv;
         }
      }
   0 && DBG( "IHLoS- %d", yLine );
   return rv;
   }

//************************************************************************************************************

// 20070527 kgoodwin this should be in a DisplayAddin, EXCEPT it has an
// external interface (it's not an automaton like the others), and I haven't
// decided how (if) to support that aspect yet.
//
STATIC_VAR bool                               s_fDrawVerticalCursorHilite;
STIL void ndVertCursorDraw()         {        s_fDrawVerticalCursorHilite = true; }
STIL bool DrawVerticalCursorHilite() { return s_fDrawVerticalCursorHilite; }

class HiliteAddin_CursorLine : public HiliteAddin {
   bool VHilitLine   ( LINE yLine, COL xIndent, LineColorsClipped &alcc ) override;
public:
   HiliteAddin_CursorLine( PView pView ) : HiliteAddin( pView ) { }
   ~HiliteAddin_CursorLine() {}
   PCChar Name() const override { return "CursorLine"; }
   };

bool HiliteAddin_CursorLine::VHilitLine( LINE yLine, COL xIndent, LineColorsClipped &alcc ) {
   // handle cursor-line and cursor-column hiliting
   const auto cxy( ColorIdx2Attr( ColorTblIdx::CXY ) );
   const auto fg ( ColorIdx2Attr( ColorTblIdx::TXT ) );
   const auto isActiveWindow_( isActiveWindow() );
   const auto isCursorLine( isActiveWindow_ && g_CursorLine() == yLine );
   if( isCursorLine ) {
      alcc.   PutColorRaw( 0            , COL_MAX, cxy );
      if( DrawVerticalCursorHilite() ) {
         return true;
         }
      }
   else {
      if( isActiveWindow_ && DrawVerticalCursorHilite() ) {
         alcc.PutColorRaw( 0            , COL_MAX, fg  );
         alcc.PutColorRaw( g_CursorCol(),       1, cxy );
         return true;
         }
      if( !USE_HiliteAddin_CompileLine && Get_LineCompile() == yLine ) {
         alcc.PutColor(    0            , COL_MAX, ColorTblIdx::STS );
         }
      }
   return false;
   }

//************************************************************************************************************
//************************************************************************************************************

void View::MoveAndCenterCursor( const Point &pt, COL xWidth ) {
   ScrollOriginAndCursor_(
        Max( 0, pt.lin - (d_pWin->d_Size.lin/2)             )
      , Max( 0, pt.col - (d_pWin->d_Size.col/2) + (xWidth/2) )
      ,         pt.lin
      ,         pt.col
      , true
      );
   }

void FBUF::LinkView( PView pv ) {
   DLINK_INSERT_LAST( d_dhdViewsOfFBUF, pv, dlinkViewsOfFBUF );
   }

void View::CommonInit() {
   FBuf()->LinkView( this );
   }

void FBUF::Push_yChangedMin() {
   const auto yMax( LastLine() );
   if( d_yChangedMin > yMax ) {
      return;
      }
   DBG_HL_EVENT && DBG( "%s(%d,%d)", __func__, d_yChangedMin, yMax );
   DLINKC_FIRST_TO_LASTA( d_dhdViewsOfFBUF, dlinkViewsOfFBUF, pv ) {
      pv->HiliteAddin_Event_FBUF_content_changed( d_yChangedMin, yMax );
      }
   d_yChangedMin = yMax+1;
   }

enum { DBADIN=0 };

bool View::InsertAddinLast( HiliteAddin *pAddin ) {
   const auto nm( pAddin->Name() );                   DBADIN && DBG( "%s %s", __PRETTY_FUNCTION__, nm );
   if( pAddin->VWorthKeeping() ) {                    DBADIN && DBG( "%s %s is VWorthKeeping", __PRETTY_FUNCTION__, nm );
      DLINK_INSERT_LAST( d_addins, pAddin, dlinkAddins );
      return true;
      }
   DBADIN && DBG( "%s %s not WorthKeeping", __PRETTY_FUNCTION__, nm );
   delete pAddin;
   return false;
   }

GLOBAL_VAR bool g_fLangHilites = true;

void View::HiliteAddins_Init() {
   DBADIN && DBG( "******************* %s+ %s hilite-addins %s lines %s", __PRETTY_FUNCTION__, d_addins.empty() ? "no": "has" , CFBuf()->HasLines() ? "has" : "no", CFBuf()->Name() );
   if(   g_fLangHilites
      && d_addins.empty()
      && (CFBuf()->HasLines() || CFBuf()->FnmIsPseudo())
      && !CFBuf()->FType().empty()
      && CFBuf()->GetFTypeSettings()
     ) { DBADIN && DBG( "%s [%s] ================================================================", __func__, CFBuf()->FType().c_str() );
      const auto pFTS( CFBuf()->GetFTypeSettings() );
     #define IAL( ainm ) InsertAddinLast( new ainm( this ) )
             /* ALWAYS */              IAL( HiliteAddin_Pbal            );
      switch( pFTS->d_hl_id ) {
         case FTypeSetting::HL_C     : IAL( HiliteAddin_cond_CPP        );
                                       IAL( HiliteAddin_clang           );  break;
         case FTypeSetting::HL_MAKE  : IAL( HiliteAddin_cond_gmake      );
                                       IAL( HiliteAddin_python          );  break;
         case FTypeSetting::HL_LUA   : IAL( HiliteAddin_lua             );  break;
         case FTypeSetting::HL_PYTHON: IAL( HiliteAddin_python          );  break;
         case FTypeSetting::HL_BASH  : IAL( HiliteAddin_python          );  break;
         case FTypeSetting::HL_DIFF  : IAL( HiliteAddin_Diff            );  break;
         default: if( pFTS->d_eolCommentDelim[0] ) {
                     IAL( HiliteAddin_EolComment );
                     }
                  break;
         }
             /* ALWAYS */               { IAL( HiliteAddin_WordUnderCursor ); }
      if( USE_HiliteAddin_CompileLine ) { IAL( HiliteAddin_CompileLine     ); }
      /* later IAL's have "last say" and therefore take precedence.  Because I want
         HiliteAddin_CursorLine, to be visible in all cases, it's IAL'd last */
                                        { IAL( HiliteAddin_CursorLine      ); }
      DBADIN && DBG( "******************* %s- %s hilite-addins %s lines %s", __PRETTY_FUNCTION__, d_addins.empty() ? "no": "has" , CFBuf()->HasLines() ? "has" : "no", CFBuf()->Name() );
     #undef IAL
      }
   }

View::View( const View &src, PWin pWin )
   : d_pWin              ( pWin )
   , d_vwToPFBuf         ( src.FBuf()               )
   , d_current           ( src.d_current            )
   , d_prev              ( src.d_prev               )
   , d_saved             ( src.d_saved              )
   , d_LastSelect_isValid( src.d_LastSelect_isValid )
   , d_LastSelectBegin   ( src.d_LastSelectBegin    )
   , d_LastSelectEnd     ( src.d_LastSelectEnd      )
   {
   CommonInit();
   }

// pardon me while I go insane  20160428 still a problem
#if (defined(__x86_64__) || defined(__ppc64__)) && defined(_WIN32)
    #define PR_TIMET_FOR_THIS_MODULE_ONLY_ARRRRGH "ll"
#else
    #define PR_TIMET_FOR_THIS_MODULE_ONLY_ARRRRGH PR_TIMET
#endif

View::View( PFBUF pFBuf_, PWin pWin_, PCChar szViewOrdinates )
   : d_pWin ( pWin_  )
   , d_vwToPFBuf( pFBuf_ )
   , d_LastSelect_isValid( false )
   {
   d_current.Cursor.Set( 0, 0 );
   d_current.Origin.Set( 0, 0 );
   if( szViewOrdinates ) {
      time_t temptv( 0 );
      sscanf( szViewOrdinates, " %d %d %d %d %" PR_TIMET_FOR_THIS_MODULE_ONLY_ARRRRGH "d"
         , &d_current.Origin.col
         , &d_current.Origin.lin
         , &d_current.Cursor.col
         , &d_current.Cursor.lin
         , &temptv
         );
      FBuf()->Set_TmLastWrToDisk( temptv );
      }
   d_prev = d_saved = d_current;
   CommonInit();
   }

void View::HiliteAddins_Delete() {
   while( auto pEl=d_addins.front() ) { // zap list
      DLINK_REMOVE_FIRST( d_addins, pEl, dlinkAddins );
      delete pEl;
      }
   }

View::~View() {
   HiliteAddins_Delete();
   FreeHiLiteRects();
   }

void View::Write( FILE *fout ) const {
   fprintf( fout, " %s|%d %d %d %d %" PR_TIMET_FOR_THIS_MODULE_ONLY_ARRRRGH "d\n"
       , FBuf()->Name()
       , Origin().col, Origin().lin
       , Cursor().col, Cursor().lin
       , FBuf()->TmLastWrToDisk()
       );
   }

struct direct_vid_seg {
   Point       d_origin;
   int         d_colorIndex;
   std::string d_str;
   direct_vid_seg( Point origin, int colorIndex, stref sr )
     : d_origin( origin )
     , d_colorIndex( colorIndex )
     , d_str( sr2st(sr) )
     {}
   };

static std::vector<direct_vid_seg> s_direct_vid_segs;

STATIC_FXN bool AddLineDelta( LINE &yLineVar, LINE yLine, LINE lineDelta ) {
   const auto fAffected( yLine <= yLineVar );
   if( fAffected ) {
      yLineVar += lineDelta;
      NoLessThan( &yLineVar, 0 );
      }
   return fAffected;
   }

void View::ViewEvent_LineInsDel( LINE yLine, LINE lineDelta ) {
   /* Why is this needed?  And why does it NOT affect g_CurView()?

      The answer lies in the architecture of the undo/redo system: Undo
      information (which is maintained per-FBUF) _includes_ cursor location.
      However, different Views on the same FBUF maintain separate cursor
      positions.  If the user executes an un/redo step, the _active_ View will
      suffer a cursor location change (in a way that makes sense) using the
      cursor location stored with the Undo information.  However, since the
      other Views on the file maintain separate cursor positions, what is
      needed is to maintain the apparent/relative location of that View's
      cursor.  This is done below.
   */
   if( lineDelta && this != g_CurView() ) {
      0 && DBG( "%s checking: %d %+d", __func__, yLine, lineDelta );
      const auto fOriginMoved( AddLineDelta( d_current.Origin.lin, yLine, lineDelta ) );
      const auto fCursorMoved( AddLineDelta( d_current.Cursor.lin, yLine, lineDelta ) );
      if( (fOriginMoved || fCursorMoved) && this == Win()->ViewHd.front() ) {
         0 && DBG( "%s %s %s moved!", __func__, fOriginMoved?"Origin":"", fCursorMoved?"Cursor":"" );
         // MoveCursor( Cursor().lin, Cursor().col );
         }
      AddLineDelta( d_prev.Origin.lin, yLine, lineDelta );
      AddLineDelta( d_prev.Cursor.lin, yLine, lineDelta );
      }
   }

void View::ULC_Cursor::EnsureWinContainsCursor( Point winSize ) {
   if( !fValid ) {
      return;
      }
#define CONSTRAIN_FIELD( xxx ) \
   if(              (Cursor.xxx - Origin.xxx) > (winSize.xxx-1) ) \
      Origin.xxx += (Cursor.xxx - Origin.xxx) - (winSize.xxx-1) ;

   CONSTRAIN_FIELD( lin )
   CONSTRAIN_FIELD( col )

#undef CONSTRAIN_FIELD
   }

void View::EnsureWinContainsCursor() {
   d_prev   .EnsureWinContainsCursor( Win()->d_Size );
   d_saved  .EnsureWinContainsCursor( Win()->d_Size );
   d_current.EnsureWinContainsCursor( Win()->d_Size );
   }

// we defer the initialization of certain View fields until the View is actually
// used to display an FBUF.  View::PutFocusOn()'s responsibility is to
// initialize these fields
//
void View::PutFocusOn() { enum { DBG_OK=0 }; DBG_OK && DBG( "%s+ %s", __func__, this->FBuf()->Name() );
   // BUGBUG This causes the View list to link to self; don't know why!?
   // ViewHead &cvwHd = g_CurViewHd();
   // DLINK_INSERT_FIRST( cvwHd, this, dlink );
   EnsureWinContainsCursor();  // window resize may have occurred
   ForceCursorMovedCondition();
   HiliteAddins_Init();
   d_tmFocusedOn = time( nullptr );
   }

void View::HiliteAddin_Event_WinResized() {
   DLINKC_FIRST_TO_LASTA( d_addins, dlinkAddins, pDispAddin ) {
      pDispAddin->VWinResized();
      }
   }

void View::HiliteAddin_Event_If_CursorMoved() {
   if( d_fCursorMoved || d_fUpdtWUC_pending ) {  0 && DBG( "%s %p", __func__, this );
      DBG_HL_EVENT && DBG( "%s(%s)", __func__, d_fUpdtWUC_pending?"t":"f" );
      DLINKC_FIRST_TO_LASTA( d_addins, dlinkAddins, pDispAddin ) {
         pDispAddin->VCursorMoved( d_fUpdtWUC_pending );
         }
      d_fCursorMoved = d_fUpdtWUC_pending = false;
      }
   }

void View::HiliteAddin_Event_FBUF_content_changed( LINE yMinChangedLine, LINE yMaxChangedLine ) {
   const auto newRev( CFBuf()->CurrContentRevision() );
   if( 1 || newRev != d_HiliteAddin_Event_FBUF_content_rev ) { 0 && DBG( "refresh %u != %u", newRev, d_HiliteAddin_Event_FBUF_content_rev );
      d_HiliteAddin_Event_FBUF_content_rev = newRev;
      DBG_HL_EVENT && DBG( "%s(%d,%d)", __func__, yMinChangedLine, yMaxChangedLine );
      DLINKC_FIRST_TO_LASTA( d_addins, dlinkAddins, pDispAddin ) {
         pDispAddin->VFbufLinesChanged( yMinChangedLine, yMaxChangedLine );
         }
      }
   }

void FlushKeyQueuePrimeScreenRedraw() {
   ConIn::FlushKeyQueueAnythingFlushed();
   DispNeedsRedrawTotal();
   }

STATIC_FXN void DispErrMsg( PCChar emsg ) { DBG( "!!! '%s'", emsg );
   SetCmdAbend();
   ConOut::Bell();
   ConIn::FlushKeyQueueAnythingFlushed();
   // HideCursor();
   VidWrStrColorFlush( DialogLine(), 0, emsg, Strlen(emsg), g_colorError, true );
   if( g_fErrPrompt ) {
      STATIC_CONST char szPAK[] = "Press any key...";
      VidWrStrColorFlush( DialogLine(), EditScreenCols() - KSTRLEN(szPAK) - 1, szPAK, KSTRLEN(szPAK), g_colorError, true );
      ConIn::WaitForKey();
    //VidWrStrColorFlush( DialogLine(), EditScreenCols() - KSTRLEN(szPAK) - 1, " ", 1, g_colorError, true );
    //VidWrStrColorFlush( DialogLine(), 0, " ", 1, g_colorError, true );
      }
   else {
      WaitForKey( 1 );
      }
   // UnhideCursor();
   }

void VErrorDialogBeepf( PCChar format, va_list args ) {
   linebuf buffer;
   chkdVsnprintf( BSOB(buffer), format, args );
   DispErrMsg( buffer );
   }

bool ErrorDialogBeepf( PCChar format, ... ) {
   linebuf buffer;
   va_list args;
   va_start(args, format);
   chkdVsnprintf( BSOB(buffer), format, args );
   va_end(args);
   DispErrMsg( buffer );
   return false;
   }

int DispRawDialogStr( PCChar st ) {
   const auto showCols( Min( Strlen( st ), EditScreenCols() ) );
   VidWrStrColorFlush( DialogLine(), 0, st, showCols, g_colorInfo, true );
   return showCols;
   }

int VMsg( PCChar pszFormat, va_list val ) {
   if( !pszFormat ) { pszFormat = ""; }
   Xbuf xb; xb.vFmtStr( pszFormat, val );
   WL(0,1) && DBG( "*** '%s'", xb.c_str() );  // <-- enable this for more UI visibility
   return DispRawDialogStr( xb.c_str() );
   }

bool Msg( PCChar pszFormat, ... ) {
   va_list args;
   va_start(args, pszFormat);
   VMsg( pszFormat, args );
   va_end(args);
   return false;
   }

void MsgClr() {
   DispRawDialogStr( "" );
   }

bool ARG::debug() {  // like message but used for debug
   switch( d_argType ) {
      default:        return BadArg();
      case NOARG:                         Msg( "ARG::debug: meta=%d (NOARG)", d_fMeta );
                      return true;
      case TEXTARG:                       Msg( "ARG::debug: meta=%d d_textarg=%s", d_fMeta, d_textarg.pText );
                      return true;
      }
   }

bool ARG::message() {
   linebuf mbuf; // meta arg "test message" message
   switch( d_argType ) {
      default:        return BadArg();
      case NOARG:                         MsgClr();
                      return true;
      case TEXTARG:                       bcpy( mbuf, d_textarg.pText );
                                          if( d_fMeta ) { DispNeedsRedrawTotal(); } // brute-force redraw EVERYTHING
                                          Msg( "%s", mbuf );
                      return true;
      }
   }

STATIC_VAR int s_dispNoiseMaxLen;

STATIC_FXN void conDisplayNoise( PCChar buffer ) {
   const auto len( Strlen( buffer ) );
   NoLessThan( &s_dispNoiseMaxLen, len );
   VidWrStrColorFlush( StatusLine(), EditScreenCols() - len, buffer, len, g_colorError, true );
   }

STATIC_FXN void conDisplayNoiseBlank() {
   if( s_dispNoiseMaxLen && EditScreenCols() > s_dispNoiseMaxLen ) {
      VidWrStrColorFlush( StatusLine(), EditScreenCols() - s_dispNoiseMaxLen, "", 0, g_colorInfo, true );
      s_dispNoiseMaxLen = 0;
      }
   }

//*************************************************************************************************************************

#if     DISP_LL_STATS
#define DISP_LL_STAT_COLLECT(x)    (x)
   STATIC_VAR struct {
      int cursorScrolls;
      int statLnUpdates;
      int screenRedraws;
      } d_stats;
#else
#define DISP_LL_STAT_COLLECT(x)
#endif

//=============================================================================

STATIC_VAR auto                   s_fStatLnRedraw = true;
void DispNeedsRedrawStatLn_()   { s_fStatLnRedraw = true; }

/*
   see also USE_HiliteAddin_CompileLine

   right now there is, in terms of what HiliteAddin events get generated, an
   admixture of "current View only" and "all visible Views": in this fxn, only
   the current View receives HiliteAddin_Event_If_CursorMoved and
   HiliteAddin_Event_FBUF_content_changed.  As usual, the counterexample (for
   non-current Views needing events) is CompileLine (changes).  There s/b some
   (not cascadingly complex) way for non-current Views to see events when they
   need them.  But what does that look like???  _Event_CursorMoved event will not
   "cover" a CompileLineChanged situation.  Do I create a new event
   _Event_CompileLineChanged just for this case???

   20070611 kgoodwin

   hmmm; I've gone a fair way toward getting this stuff working, but run into
   some challenges

   mainly, how fbuf-scope events get translated into view scope events, etc.

   the xlation challenge is prob related to the fact that the hilighting
   addins "architecture" was not designed as a full screen-redraw engine,
   which is where I'm pushing it.

   20080317 kgoodwin
*/
#include "BitVector.h"

typedef BitVector<uint_machineword_t> srd_linevect;
STATIC_VAR srd_linevect   *s_paScreenLineNeedsRedraw;

STATIC_FXN bool NeedRedrawScreen() {
   return s_paScreenLineNeedsRedraw ? s_paScreenLineNeedsRedraw->IsAnyBitSet() : false;//TODO
   }

void DispNeedsRedrawAllLinesAllWindows_() { FULL_DB && DBG( "%s", FUNC );
   s_paScreenLineNeedsRedraw->SetAllBits();
   }

void Win::DispNeedsRedrawAllLines() const { FULL_DB && DBG( "%s All=0..%d", FUNC, g_CurWin()->d_Size.lin );
   for( auto lineWithinWin(0); lineWithinWin < d_Size.lin; ++lineWithinWin ) {
      const auto yLine( lineWithinWin + d_UpLeft.lin );
      Assert( yLine < EditScreenLines() );
      s_paScreenLineNeedsRedraw->SetBit( yLine );
      }
   }

STATIC_FXN void RedrawScreen() {
// #define  SHOW_DRAWS  defined(_WIN32)
#define  SHOW_DRAWS  0
   #if SHOW_DRAWS
   linebuf lbf;  PChar pLbf = lbf;  *pLbf++ = chLSQ;
   #define  ShowDraws( code )  code
   #else
   #define  ShowDraws( code )
   #endif
   const auto yDispMin( MinDispLine() );
   const auto scrnCols( EditScreenCols() );
   STATIC_VAR std::string buf;
   const auto yTop(0), yBottom( EditScreenLines() );
   ShowDraws( DBG( "%s+ [%2d..%2d)", __func__, yTop, yBottom ); )
   const HiLiteRec *pFirstPossibleHiLite(nullptr);
   auto dvsit( s_direct_vid_segs.cbegin() );
   for( auto yLine(yTop) ; yLine < yBottom; ++yLine ) { ShowDraws( char ch = ' '; )
      if( s_paScreenLineNeedsRedraw->IsBitSet( yLine ) ) {
         ShowDraws( ch = '0' + (yLine % 10); )
         LineColors alc;
         { FULL_DB && DBG( "%s y=%d", FUNC, yLine );
         buf.assign( scrnCols, H__ ); //***** initial assumption: this line is a horizontal border ('Õ')
         alc.PutColor( 0, scrnCols, g_colorWndBorder );
         for( auto ix(0) ; ix < g_iWindowCount(); ++ix ) {
            g_Win(ix)->GetLineForDisplay( ix, buf, alc, pFirstPossibleHiLite, yLine );
            }
         }
         for( ; dvsit != s_direct_vid_segs.cend() && dvsit->d_origin.lin < yLine ; ++dvsit ) {
            }
         for( ; dvsit != s_direct_vid_segs.cend() && yLine == dvsit->d_origin.lin ; ++dvsit ) {
           #if 1
            buf.replace ( dvsit->d_origin.col, dvsit->d_str.length(), dvsit->d_str        ); 0 && DBG( "%" PR_BSR "'", BSR(dvsit->d_str) );
            alc.PutColor( dvsit->d_origin.col, dvsit->d_str.length(), dvsit->d_colorIndex );
           #else
            const auto ix( dvsit->d_origin.col );
            (ix < 0) && DBG( "IX NEGATIVE! %d", ix );
            if( ix >= 0 && ix < scrnCols ) {
               const auto maxXwr( ix + dvsit->d_str.length() - 1 );
               const auto len( maxXwr > scrnCols ? dvsit->d_str.length() - (maxXwr - scrnCols) : dvsit->d_str.length() );
               buf.replace ( ix, len, dvsit->d_str        ); 0 && DBG( "%" PR_BSR "'", BSR(dvsit->d_str) );
               alc.PutColor( ix, len, dvsit->d_colorIndex );
               }
           #endif
            }
         (buf.length() != scrnCols) && DBG( "buf.length() != scrnCols: %" PR_SIZET "!=%u", buf.length(), scrnCols );
         VidWrStrColors( yDispMin+yLine, 0, buf.data(), scrnCols, &alc, false );
         }
      ShowDraws( *pLbf++ = ch; )
      }
   s_paScreenLineNeedsRedraw->ClrAllBits();
   #if SHOW_DRAWS
   *pLbf++ = chRSQ;  *pLbf = '\0';  DBG( "%s- [%2d..%2d): %s", __func__, yTop, yBottom, lbf );
   #endif
   }

void FBOP::PrimeRedrawLineRangeAllWin( PCFBUF fb, LINE yMin, LINE yMax ) { // for single-line Prime, yMin == yMax
   if( yMin > yMax ) {
      std::swap( yMin, yMax );
      }
   FULL_DB && DBG( "Prime FL [%d..%d]", yMin, yMax );
   for( int ix=0 ; ix < g_iWindowCount() ; ++ix ) {
      const auto pWin ( g_Win(ix)         );
      const auto pView( pWin->CurViewWr() );
      if( pView && pView->FBuf() == fb ) {
         NewScope { // hilighting: if the word moves under the cursor, it's the same as the cursor moving
            Point cursor( pView->GetCursor() );
            if( yMin <= cursor.lin && yMax >= cursor.lin ) {
               pView->ForceCursorMovedCondition();
               }
            }
         FULL_DB && DBG( "Prime win%d [%d..%d]", ix, yMin, yMax );
         for( auto wyMin( pWin->d_UpLeft.lin + LargerOf(                     0, yMin - pView->Origin().lin ) ),
                   wyMax( pWin->d_UpLeft.lin + SmallerOf( pWin->d_Size.lin - 1, yMax - pView->Origin().lin ) )
            ; wyMin <= wyMax
            ; ++wyMin
            ) {
            s_paScreenLineNeedsRedraw->SetBit( wyMin );
            }
         }
      }
   #if TRACE_DISP_NEEDS
   DBG( "%s by %s L %d", "ndScreenRedraw", __func__, __LINE__ );
   #endif
   }

void DispNeedsRedrawAllLinesCurWin_() {
   g_CurWin()->DispNeedsRedrawAllLines();
   }

// dialogtop:invert
// dialogtop:-
// dialogtop:yes
// dialogtop:no

GLOBAL_VAR bool g_fDialogTop = true;

GLOBAL_VAR LINE s_iHeight;  // global read, local write
GLOBAL_VAR COL  s_iWidth ;  // global read, local write

LINE MinDispLine() { return g_fDialogTop ? 2 : 0          ; }
LINE DialogLine()  { return g_fDialogTop ? 1 : s_iHeight-2; }
LINE StatusLine()  { return g_fDialogTop ? 0 : s_iHeight-1; }

void Event_ScreenSizeChanged( const Point &newSize ) { DBG( "%s %d,%d->%d,%d", __func__, s_iHeight, s_iWidth, newSize.lin, newSize.col );
   s_iHeight = newSize.lin; // THE ONLY PLACE WHERE THIS VARIABLE IS ASSIGNED!!!
   s_iWidth  = newSize.col; // THE ONLY PLACE WHERE THIS VARIABLE IS ASSIGNED!!!
   DeleteUp( s_paScreenLineNeedsRedraw, new srd_linevect ( EditScreenLines() ) );
   Wins_ScreenSizeChanged( newSize );
   DispNeedsRedrawTotal();
   }

STATIC_VAR auto  s_CursorLocnOutsideView_isValid( false );
STATIC_VAR Point s_CursorLocnOutsideView;

void CursorLocnOutsideView_Set_( LINE y, COL x, PCChar from ) {
   s_CursorLocnOutsideView.lin = y;
   s_CursorLocnOutsideView.col = x;
   s_CursorLocnOutsideView_isValid = true;
   FULL_DB && DBG( "%s(y=%d,x=%d)", __func__, y, x );
   ConOut::SetCursorLocn( y, x );
   // DispNeedsRedrawCursorMoved();  this has the effect of HIDING the cursor when the dialog line is accepting typed input
   }

void CursorLocnOutsideView_Unset() { 0 && DBG( "%s", __func__ );
   s_CursorLocnOutsideView_isValid = false;
   }

bool CursorLocnOutsideView_Get( Point *pt ) {
   if( s_CursorLocnOutsideView_isValid ) {
      *pt = s_CursorLocnOutsideView;
      }
   return s_CursorLocnOutsideView_isValid;
   }

STATIC_FXN void DrawStatusLine();
STATIC_FXN void UpdtDisplay() { // NB! called by IdleThread, so must run to completion w/o blocking (calling anything that calls GlobalVariableLock)
   FULL_DB && DBG( "%s+", __func__ );
   PCWV;
   if( !(g_CurFBuf() && pcv) ) {
      return;
      }
   FULL_DB && DBG( "%s: working", __func__ );
   // PerfCounter pc;
   VideoFlusher vf;
   STATIC_VAR bool s_fHideVerticalCursorHilite;
   Point cursorNew;
   bool  fCursorMovePending;
   {
   YX_t cursorNow; bool fCursorVisible;
   CursorLocnOutsideView_Get( &cursorNew ) || pcw->GetCursorForDisplay( &cursorNew );
   ConOut::GetCursorState( &cursorNow, &fCursorVisible );
   fCursorMovePending = (cursorNew.lin != cursorNow.lin || cursorNew.col != cursorNow.col);
   }
   auto did(0);
   pcv->HiliteAddin_Event_If_CursorMoved();
   auto fScreenRedrawPending( NeedRedrawScreen() );
   s_fStatLnRedraw = s_fStatLnRedraw || fCursorMovePending || fScreenRedrawPending;
   if(    s_fDrawVerticalCursorHilite
      || (s_fHideVerticalCursorHilite && (s_fStatLnRedraw || fCursorMovePending || fScreenRedrawPending))
     ) {
      did |= 0xA0000000;
      DispNeedsRedrawAllLinesCurWin();
      fScreenRedrawPending = true;
      }
   for( auto ix(0) ; ix < g_iWindowCount(); ++ix ) {
      g_Win(ix)->CurView()->FBuf()->Push_yChangedMin(); // used by HiliteAddin methods
      }
   if( fScreenRedrawPending ) {
      did |= 0x000A0000;
      pcv->HiliteAddin_Event_FBUF_content_changed( 0, 0 ); // bugbug needed to make cpp hilites update correctly
      RedrawScreen();
      s_fHideVerticalCursorHilite = s_fDrawVerticalCursorHilite;
                                    s_fDrawVerticalCursorHilite = false;
      }
   if( s_fStatLnRedraw ) { s_fStatLnRedraw = false;
      did |= 0x00000A00;
      DrawStatusLine();
      DISP_LL_STAT_COLLECT(++d_stats.statLnUpdates);
      }
   if( fCursorMovePending ) {
      did |= 0x0000000C;
      ConOut::SetCursorLocn( cursorNew.lin, cursorNew.col );
      DISP_LL_STAT_COLLECT(++d_stats.cursorScrolls);
      }
   if( FULL_DB && did ) { DBG( "%s did=%08X", __func__, did ); }
   }

// public layer around DispUpdt

#if     DISP_LL_STATS
int DispCursorMoves()   { return d_stats.cursorScrolls; }
int DispStatLnUpdates() { return d_stats.statLnUpdates; }
int DispScreenRedraws() { return d_stats.screenRedraws; }
#endif

// BUGBUG note that if DispRefreshWholeScreenNow_ is called really often (like every time a key is hit, in autorepeat mode), the process(/thread?) hangs

void DispNeedsRedrawCurWin_()                { DispNeedsRedrawAllLinesCurWin(); }
void DispNeedsRedrawTotal_()                 { DispNeedsRedrawAllLinesAllWindows(); }
void DispNeedsRedrawVerticalCursorHilite_()  { ndVertCursorDraw(); }
void DispDoPendingRefreshes_()               { MEM_CBP( true ); UpdtDisplay(); MEM_CBP( false );  }
void DispDoPendingRefreshesIfNotInMacro_()   { if( !Interpreter::Interpreting() ) { DispDoPendingRefreshes_(); } }
void DispRefreshWholeScreenNow_()            { DispNeedsRedrawTotal_(); DispDoPendingRefreshes_(); }

//-----------------------------------------------------------------------------------------------------------
//
// Color/attribute notes
//
// Some API's "think" in ColorTblIdx::codes.  Chief among these are the InsHiLite
// (Box, Lines) family.  Internally, these directly xlat ColorTblIdx::codes to
// attributes using ColorIdx2Attr().
//

// NOTE: xCol & yLine are WITHIN CONSOLE WINDOW !!!

STATIC_FXN COL conVidWrStrColors( LINE yLine, COL xCol, PCChar pszStringToDisp, COL maxCharsToDisp, const LineColors * alc, bool fFlushNow ) {
   VideoFlusher vf( fFlushNow );
   FULL_DB && DBG( "%s+", __func__ );
   auto lastColor(0);
   for( auto ix(0) ; maxCharsToDisp > 0 && alc->inRange( ix ) ; ) {
      lastColor = alc->colorAt( ix );
      const auto segLen( Min( alc->runLength( ix ), maxCharsToDisp ) );
      FULL_DB && DBG( "%s Len=%d", __func__, segLen );
      VidWrStrColor( yLine, xCol, pszStringToDisp, segLen, lastColor, 0 );
      ix              += segLen;
      xCol            += segLen;
      pszStringToDisp += segLen;
      maxCharsToDisp  -= segLen;
      }
   if( ScreenCols() > xCol ) { VidWrStrColor( yLine, xCol, " "    , 1, lastColor, true  ); }
// else                      { VidWrStrColor( yLine, xCol, nullptr, 0, lastColor, false ); }
   FULL_DB && DBG( "%s-", __func__ );
   return xCol;
   }

// STATIC_CONST char EntabDispChar[] = "nyY";
// static_assert( KSTRLEN(EntabDispChar) == MAX_ENTAB_INVALID, "KSTRLEN(EntabDispChar) == MAX_ENTAB_INVALID" );

void LineColors::Cat( const LineColors &rhs ) {  0 && DBG( "CAT[%3d]", cols() );
   auto iy(0);
   for( auto ix(cols()) ; inRange( ix ) && rhs.inRange( iy ) && (END_MARKER != rhs.colorAt( iy )) ; ++ix, ++iy ) {
      b[ ix ] = rhs.colorAt( iy );
      }
   }

COL VidWrColoredStrefs( LINE yLine, COL xCol, const ColoredStrefs &csrs, bool fFlushNow ) {
   VideoFlusher vf( fFlushNow );
   for( const auto &csr : csrs ) {
      xCol += csr.VidWr( yLine, xCol );
      }
   return xCol;
   }

class ColoredLine {
   size_t      d_curLen;
   linebuf     d_charBuf;
   LineColors  d_alc;
public:
   ColoredLine()
      : d_curLen( 0 )
      {
      d_charBuf[0] = '\0';
      }
   int  textcols() const { return d_curLen; }
   void Cat( int ColorIdx, stref src );
   void Cat( const ColoredLine &rhs );
   void VidWrLine( LINE line ) const { VidWrStrColors( line, 0, d_charBuf, textcols(), &d_alc, true ); }
   };

void ColoredLine::Cat( int ColorIdx, stref src ) {
   const auto attr( g_CurView()->ColorIdx2Attr( ColorIdx ) );
   const auto cpyLen( SmallerOf( src.length(), (int(sizeof(d_charBuf))-1) - d_curLen) );
   if( cpyLen > 0 ) { 0 && DBG( "Cat:PC=[%3" PR_SIZET "..%3" PR_SIZET "] %02X %" PR_BSR "", d_curLen, d_curLen+cpyLen-1, attr, BSR(src) );
      d_alc.PutColor( d_curLen, cpyLen, attr );
      memcpy( d_charBuf + d_curLen, src.data(), cpyLen );
      d_curLen += cpyLen;
      d_charBuf[ d_curLen ] = '\0';
      }
   }

void ColoredLine::Cat( const ColoredLine &rhs ) {
   memcpy( d_charBuf + d_curLen, rhs.d_charBuf, rhs.d_curLen );
   d_curLen += rhs.d_curLen;
   d_charBuf[ d_curLen ] = '\0';
   d_alc.Cat( rhs.d_alc );
   }

STATIC_FXN void DrawStatusLine() { FULL_DB && DBG( "*************> UpdtStatLn" );
   ColoredLine cl;
   cl.Cat( ColorTblIdx::HIL, (EditorLoadCount() > 1) ? FmtStr<15>( " +%u", EditorLoadCount()-1 ).k_str() : "" );
   const auto pfh( g_CurFBuf() );
   if( 0 ) {
      int BRTowardUndo, ARTowardUndo, BRTowardRedo, ARTowardRedo;
      auto BRTotal( pfh->GetUndoCounts( &BRTowardUndo, &ARTowardUndo, &BRTowardRedo, &ARTowardRedo ) );
      if( BRTotal > 1 || pfh->IsDirty() ) {
         cl.Cat( ColorTblIdx::INF,
            FmtStr<50>( "%s%d:%d|%d:%d"
              , pfh->IsDirty() ? "*" : ""
              , BRTowardUndo, ARTowardUndo
              , BRTowardRedo, ARTowardRedo
              ).k_str()
            );
         }
      }
   else {
      cl.Cat( ColorTblIdx::INF, pfh->IsDirty() ? "*" : "" );
      }
   if( pfh->FnmIsDiskWritable() && pfh->EolMode()!=platform_eol ) {
      if( g_fForcePlatformEol )                                          { cl.Cat( ColorTblIdx::ERRM, "!" ); }
                                                                           cl.Cat( ColorTblIdx::INF , pfh->EolName() );
      }
   if( pfh->IsNoEdit() )                                                 { cl.Cat( ColorTblIdx::ERRM, " !edit!" ); }
#if defined(_WIN32)
   if( pfh->IsDiskRO() )                                                 { cl.Cat( ColorTblIdx::ERRM, " DiskRO" ); }
#endif
   cl.Cat( ColorTblIdx::SEL , FmtStr<45>( "X=%u Y=%u/%u", 1+g_CursorCol(), 1+g_CursorLine()   , pfh->LineCount() ).k_str() );
// cl.Cat( ColorTblIdx::INF , FmtStr<60>( "[%" PR_BSR "%s]", BSR( pfh->FType() ), LastRsrcLdFileSectionNm() ).k_str() );
// cl.Cat( ColorTblIdx::INF , FmtStr<60>( "[%s]", LastRsrcLdFileSectionNm() ).k_str() );
// cl.Cat( ColorTblIdx::INF , FmtStr<60>( "%s", LastRsrcLdFileSectionNm() ).k_str() );
// cl.Cat( ColorTblIdx::INF , FmtStr<60>( "[%" PR_BSR "]", BSR(LastRsrcLdFileSectionNmTruncd()) ).k_str() );
   { const auto ftypset( pfh->FType().c_str() ); const auto lastrsrcftypset( LastRsrcFileLdSectionFtypeNm() );
   if( eq( ftypset, lastrsrcftypset ) ) { // avoid redundant status display
      cl.Cat( ColorTblIdx::INF , FmtStr<60>( "[%" PR_BSR ":%s]", BSR(pfh->GetRsrcExt()), ftypset ).k_str() );
      }
   else {
      cl.Cat( ColorTblIdx::INF , FmtStr<60>( "[%" PR_BSR ":ft=%s:ldd=%s]", BSR(pfh->GetRsrcExt()), ftypset, lastrsrcftypset ).k_str() );
      }
   }
// cl.Cat( ColorTblIdx::ERRM, FmtStr<30>( "t%ue%d "      , pfh->TabWidth(), pfh->Entab() ).k_str() );
// cl.Cat( ColorTblIdx::ERRM, FmtStr<30>( "%ce%dw%ui%d " , g_fRealtabs?'R':'r', pfh->Entab(), pfh->TabWidth(), pfh->IndentIncrement() ).k_str() );
   cl.Cat( ColorTblIdx::ERRM, FmtStr<30>( "%ce%dw%u"     , g_fRealtabs?'R':'r', pfh->Entab(), pfh->TabWidth()                         ).k_str() );
// cl.Cat( ColorTblIdx::INF , FmtStr<20>( "%s"           , g_fCase ? "E!=e" : "E==e" ).k_str() );
// cl.Cat( ColorTblIdx::INF , FmtStr<20>( "%s"           , g_fCase ? "Q!=q" : "Q==q" ).k_str() );
   cl.Cat( ColorTblIdx::INF , FmtStr<20>( "%s"           , g_fCase ? "A!=a" : "A==a" ).k_str() );
   if( 0 ) { // 20150105 KG: seems superfluous
      if( g_pFbufClipboard && g_pFbufClipboard->LineCount() ) {
         PCChar st;
         switch( g_ClipboardType ) {
            case LINEARG:   st = "Lin"; break;
            case STREAMARG: st = "Str"; break;
            case BOXARG:    st = "Box"; break;
            default:        st = "???"; break;
            }
         cl.Cat( ColorTblIdx::SEL, FmtStr<20>( "clp(%s%d)", st, g_pFbufClipboard->LineCount() ).k_str() );
         }
      else {
         cl.Cat( ColorTblIdx::SEL, "clp()" );
         }
      cl.Cat( ColorTblIdx::INF, " " );
      }
   cl.Cat( ColorTblIdx::INF, FmtStr<40>( "%s%s"
            , g_fMeta                  ? " META"      : ""
            , IsMacroRecordingActive() ? (IsCmdXeqInhibitedByRecord() ? " NOX-RECORDING" : " RECORDING") : ""
            ).k_str()
         );
   //-----------------------------------------------------------------------
   // Display that part of pfh->Namestr() which is common with the cwd in a
   // different color than the remainder of pfh->Namestr().  Display any part
   // of the path of pfh->Namestr() that diverges from the cwd in an again
   // different color.  Purpose: so user can tell at a glance whether
   // curfile is (or more importantly, is not) within cwd subtree, and if
   // not, how deep without.
   //
   const auto cwdbuf( Path::GetCwd_ps() );
   const auto cwdlen( cwdbuf.length() );
   const auto fnLen( pfh->Namestr().length() );
   const auto cfpath( Path::RefDirnm( pfh->Namestr() ) );
   const auto commonLen( Path::CommonPrefixLen( cwdbuf, cfpath ) ); // 0 && DBG( "%s|%s (%" PR_SIZET ")", cwdbuf.c_str(), cfpath.c_str(), commonLen );
   const auto divergentPath( commonLen < cwdlen );
   const auto uniqPathLen( divergentPath ? cfpath.length() - commonLen : 0 );
   const auto uniqLen( fnLen - commonLen - uniqPathLen );
   //
   //-----------------------------------------------------------------------
   ColoredLine out;
   const auto maxFnLen( EditScreenCols() - cl.textcols() );
   if( fnLen > maxFnLen ) {
      // _WIN32: _DO NOT_ try to use PathCompactPathEx here http://forums.microsoft.com/MSDN/ShowPost.aspx?PostID=203814&SiteID=1
      // TODO copy pfh->Name() into pathbuf, adjust commonLen, uniqLen accordingly, fall into else case
      STATIC_CONST char s_dot3[] = "...";
      const auto truncBy( fnLen - (maxFnLen - KSTRLEN(s_dot3)) );
      if( truncBy > fnLen ) {
         out.Cat( ColorTblIdx::TXT, s_dot3 );
         }
      else {
         out.Cat( ColorTblIdx::TXT, FmtStr<_MAX_PATH>( "%s%s", s_dot3, pfh->Name()+truncBy ).k_str() );
         }
      }
   else {
      auto pfnm( pfh->Name() );
      if( commonLen   ) { out.Cat( ColorTblIdx::HIL , stref(pfnm, commonLen  ) ); pfnm += commonLen  ; }
      if( uniqPathLen ) { out.Cat( ColorTblIdx::ERRM, stref(pfnm, uniqPathLen) ); pfnm += uniqPathLen; }
      if( uniqLen     ) { out.Cat( ColorTblIdx::TXT , stref(pfnm, uniqLen    ) ); pfnm += uniqLen    ; }
      }
   out.Cat( cl );                       0 && DBG( "%s+", __func__ );
   out.VidWrLine( StatusLine() );       FULL_DB && DBG( "%s-", __func__ );
   }

//--------------------------------------------------------------------------------------
void DirectVidClear() {
   s_direct_vid_segs.clear();
   DispNeedsRedrawAllLinesAllWindows();
   UpdtDisplay();
   }

void DirectVidWrStrColorFlush( LINE yLine, COL xCol, stref sr, int colorIndex ) {
   if( !(yLine >= 0 && yLine < ScreenLines()) ) { return; }
   if( xCol < 0 ) {
      sr.remove_prefix( -xCol );
      xCol = 0;
      }
   if( sr.empty() ) { return; }
   if( xCol + sr.length() > ScreenCols() ) {
      sr.remove_suffix( (xCol + sr.length()) - ScreenCols() );
      }
   if( sr.empty() ) { return; }
   const Point tgt( yLine, xCol );
   auto it( s_direct_vid_segs.begin() );
   for( ; it != s_direct_vid_segs.end() ; ++it ) {
      if( tgt == it->d_origin && it->d_str.length() == sr.length() ) {
         auto fChanged( false );
         if( it->d_colorIndex != colorIndex ) {
            it->d_colorIndex = colorIndex;
            fChanged = true;
            }
         if( !eq( it->d_str, sr ) ) {
            it->d_str.assign( sr2st(sr) ); // overwrite same-length string with new
                           0 && DBG( "%s [%" PR_SIZET "]=y/x=%d/%d C=%02X '%" PR_BSR "'", __func__, std::distance( s_direct_vid_segs.begin(), it ), it->d_origin.lin, it->d_origin.col, it->d_colorIndex, BSR(it->d_str) );
            fChanged = true;
            }
         if( fChanged ) { // mark line dirty
            s_paScreenLineNeedsRedraw->SetBit( it->d_origin.lin );
            UpdtDisplay();
            }
         return;
         }
      else if( tgt <= it->d_origin ) {
         break;
         }
      }
   const auto new_it( s_direct_vid_segs.emplace( it, tgt, colorIndex, sr ) );
                           0 && DBG( "%s @[%" PR_SIZET "]^y/x=%d/%d C=%02X '%" PR_BSR "'", __func__, std::distance( s_direct_vid_segs.begin(), new_it ), new_it->d_origin.lin, new_it->d_origin.col, new_it->d_colorIndex, BSR(new_it->d_str) );
   s_paScreenLineNeedsRedraw->SetBit( new_it->d_origin.lin );
   UpdtDisplay();
   }
//--------------------------------------------------------------------------------------

void swid_ch( PChar dest, size_t sizeofDest, char ch ) {
   safeSprintf( dest, sizeofDest, "0x%02X (%c)", ch, ch );
   }

STATIC_FXN bool swixChardisp( stref param, char &charVar ) {
   DBG( "%s: param=%" PR_BSR "'", __PRETTY_FUNCTION__, BSR( param ) );
   charVar = char(StrToInt_variable_base( param, 10 ));
   if( char(-1) == charVar ) {
      charVar = param.length() == 1 ? param[0] : ' ';
      }
   DispNeedsRedrawAllLinesAllWindows();
   return true;
   }

GLOBAL_VAR char g_chTabDisp  = BIG_BULLET; // g_chTabDisp == BIG_BULLET has special (cool!) behavior
bool swixTabdisp(   stref param ) { return swixChardisp( param, g_chTabDisp        ); }
void swidTabdisp( PChar dest, size_t sizeofDest, void *src ) {
   swid_ch( dest, sizeofDest, static_cast<int>(g_chTabDisp) );
   }

GLOBAL_VAR char g_chTrailSpaceDisp = ' ';
bool swixTraildisp( stref param ) { return swixChardisp( param, g_chTrailSpaceDisp ); }
void swidTraildisp( PChar dest, size_t sizeofDest, void *src ) {
   swid_ch( dest, sizeofDest, static_cast<int>(g_chTrailSpaceDisp) );
   }

GLOBAL_VAR char g_chTrailLineDisp = '~';
bool swixTrailLinedisp( stref param ) { return swixChardisp( param, g_chTrailLineDisp ); }
void swidTrailLinedisp( PChar dest, size_t sizeofDest, void *src ) {
   swid_ch( dest, sizeofDest, static_cast<int>(g_chTrailLineDisp) );
   }

//--------------------------------------------------------------------------------------

void View::ScrollByPages( int pages ) {
   ScrollOriginAndCursorNoUpdtWUC(
        Origin().lin + (d_pWin->d_Size.lin * pages), Origin().col
      , Cursor().lin + (d_pWin->d_Size.lin * pages), Cursor().col
      );
   }

void View::ScrollOriginAndCursor_( LINE yOrigin, COL xOrigin, LINE yCursor, COL xCursor, bool fUpdtWUC ) { enum { SHOWDBG=0 };
   SHOWDBG && DBG( "ScrollW&C+ IS: UlcYX=(%d,%d) CurYX=(%d,%d) -> REQUESTED: UlcYX=(%d,%d) CurYX=(%d,%d) fUpdtWUC=%c ++++++++++++++"
      , d_current.Origin.lin, d_current.Origin.col, d_current.Cursor.lin, d_current.Cursor.col
      , yOrigin, xOrigin, yCursor, xCursor, fUpdtWUC?'t':'f'
      );
   SHOWDBG && DBG( "lastline=%d, winsize=%d, vscroll=%d, 1/3=%d", CFBuf()->LastLine(), d_pWin->d_Size.lin, g_iVscroll, (d_pWin->d_Size.lin/3) ); // max 1/3 of window contains past-EOF
   NewScope { // constrain x
      Constrain( COL(0), &xCursor, COL_MAX );                 SHOWDBG && DBG( "xCursor = %d", xCursor );
      Assert( COL_MAX > d_pWin->d_Size.col );
      const auto xOriginMax( COL_MAX - d_pWin->d_Size.col );
      Constrain( COL(0), &xOrigin, xOriginMax );
      }
   NewScope { // constrain yOrigin
      const auto yOriginMax( CFBuf()->LastLine() // d_pWin value defines the bottom scroll limit
         #if 1
            - d_pWin->d_Size.lin + g_iVscroll + (d_pWin->d_Size.lin/3) // max 1/3 of window contains past-EOF
         #elif 0
            - d_pWin->d_Size.lin + g_iVscroll + 1 // new behavior: only show ONE nonexistent line at bottom.
         #else
             // old behavior: only last line is visible (top line)
         #endif
         );                                                      SHOWDBG && DBG( "yOriginMax = %d", yOriginMax );
      Constrain( LINE(0), &yOrigin, yOriginMax );                      SHOWDBG && DBG( "yOrigin = %d", yOrigin );
      }

   // save initial values for future change detection
   const auto yOriginInitial( d_current.Origin.lin );
   const auto yCursorInitial( d_current.Cursor.lin );
   const auto xCursorInitial( d_current.Cursor.col );  0 && DBG( "cc0=%d", d_current.Cursor.col );

   const auto yOriginDelta( yOrigin - d_current.Origin.lin );
   const auto xOriginDelta( xOrigin - d_current.Origin.col );
   d_current.Origin.lin = yOrigin; // don't use yOrigin below here; use d_current.Origin.lin
   d_current.Origin.col = xOrigin; // don't use xOrigin below here; use d_current.Origin.col
   LINE yMin, yMax;
   if( xOriginDelta || yOriginDelta ) {
      // CopyCurrentCursLocnToSaved(); 15-Jul-2004 klg d_pWin makes savecur/restcur less than useful
      if( yOriginDelta > 0 ) {
         yMin = d_current.Origin.lin + d_pWin->d_Size.lin;
         yMax = yMin + yOriginDelta;
         }
      else {
         yMax = d_current.Origin.lin;
         yMin = yMax + yOriginDelta;
         }
      }
   else { // origin has not moved in y dimension
      yMin = yMax = yCursorInitial;
      }
              d_current.Cursor.col = d_current.Origin.col + d_pWin->d_Size.col - 1;   0 && DBG( "ccF=%d", d_current.Cursor.col );
   Constrain( d_current.Origin.col, &d_current.Cursor.col, xCursor );                 0 && DBG( "ccG=%d", d_current.Cursor.col );
              d_current.Cursor.lin = d_current.Origin.lin + d_pWin->d_Size.lin - 1;
   Constrain( d_current.Origin.lin, &d_current.Cursor.lin, yCursor );
   //##########################################################################################
   //##########################################################################################
   //##
   //## calcs done: propagate changes to display-updater as necessary
   //##
   //##########################################################################################
   //##########################################################################################
   SHOWDBG && DBG( "ScrollW&C-  UlcYX=(%d,%d) CurYX=(%d,%d) --------------", d_current.Origin.lin, d_current.Origin.col, d_current.Cursor.lin, d_current.Cursor.col );
   if( ActiveInWin() ) { // this is the current view of a(ny) window?
                         // push display updates out
      const auto fVertScreenScroll( yOriginInitial != d_current.Origin.lin );
      const auto fVertCursorMove( yOriginDelta || yCursorInitial != d_current.Cursor.lin );
      const auto fHorzCursorMove( xOriginDelta || xCursorInitial != d_current.Cursor.col );
      if( fVertCursorMove ) {
         // this is done to update the "current line hilite" which follows the
         // cursor, making it easier to track it when in high res mode (or for
         // older eyes)
         //
         // calling PrimeRedrawLine**AllWin** because we COULD be moving the
         // cursor in the non-ACTIVE window.  EX: nextmsg causes cursor to move
         // in primary window/View _AND_ in search-results FBUF which is
         // probably being viewed in another window
         //
         FBOP::PrimeRedrawLineRangeAllWin( FBuf(), yCursorInitial      , yCursorInitial       );
         FBOP::PrimeRedrawLineRangeAllWin( FBuf(), d_current.Cursor.lin, d_current.Cursor.lin );
         }
      if( fUpdtWUC && (fVertCursorMove || fHorzCursorMove) ) {
         d_fUpdtWUC_pending = true;
         }
      if( xOriginDelta || yOriginDelta ) { // origin has changed, so "scrolling" (screen rewrite) is REQUIRED
         DispNeedsRedrawStatLn();
         d_pWin->DispNeedsRedrawAllLines();
         }
      }
   }

void View::ScrollOriginYX( LINE yLine, COL xCol ) {
   ScrollOriginAndCursor( yLine, xCol, d_current.Cursor.lin, d_current.Cursor.col );
   }

STATIC_VAR struct {
   bool fDidVideoWrite;
   } s_VideoFlushData;

VideoFlusher::~VideoFlusher() {
   if( d_fWantToFlush && s_VideoFlushData.fDidVideoWrite ) {
      ConOut::BufferFlushToScreen();
      s_VideoFlushData.fDidVideoWrite = false;
      }
   }

STATIC_FXN COL conVidWrStrColor( LINE yConsole, COL xConsole, PCChar src, COL srcChars, int attr, bool fPadWSpcs ) { WL( 0, 0 ) && DBG( "VidWrStrColor Y=%3d X=%3d L %3d C=%02X pad=%d '%s'", yConsole, xConsole, srcChars, attr, fPadWSpcs, src );
   if( src ) {
      const auto charsWritten( ConOut::BufferWriteString( src, srcChars, yConsole, xConsole, attr, fPadWSpcs ) );
      if( charsWritten ) {
         DISP_LL_STAT_COLLECT(++d_stats.screenRedraws);
         s_VideoFlushData.fDidVideoWrite = true;
         return charsWritten;
         }
      }
   return 0;
   }

COL VidWrStrColorFlush( LINE yConsole, COL xConsole, PCChar src, size_t srcChars, int attr, bool fPadWSpcs ) {
   VideoFlusher vf;
   return VidWrStrColor( yConsole, xConsole, src, srcChars, attr, fPadWSpcs );
   }

//***********************************************************************************************

STATIC_FXN void streamDisplayNoise( PCChar buffer ) {}
STATIC_FXN void streamDisplayNoiseBlank() {}
STATIC_FXN COL streamVidWrStrColor( LINE, COL, PCChar src, COL, int, bool ) {
   fprintf( stderr, "%s\n", src );
   return Strlen( src );
   }
STATIC_FXN COL streamVidWrStrColors( LINE, COL, PCChar src, COL, const LineColors *, bool ) {
   fprintf( stderr, "%s\n", src );
   return Strlen( src );
   }

GLOBAL_VAR DisplayDriverApi g_DDI =
   { streamDisplayNoise
   , streamDisplayNoiseBlank
   , streamVidWrStrColor
   , streamVidWrStrColors
   };

STATIC_FXN void ddi_console() {
   g_DDI.DisplayNoise      = conDisplayNoise       ;
   g_DDI.DisplayNoiseBlank = conDisplayNoiseBlank  ;
   g_DDI.VidWrStrColor     = conVidWrStrColor      ;
   g_DDI.VidWrStrColors    = conVidWrStrColors     ;
   }

bool ConIO_InitOK( bool fForceNewConsole ) {
   if( ConIO::StartupOk( fForceNewConsole ) ) {
      ddi_console();
      return true;
      }
   return false;
   }

void ConIO_Shutdown() {
   ConIO::Shutdown();
   }

//***********************************************************************************************

STATIC_FXN bool EditorScreenSizeAllowed( const Point &newSize ) { // checks EDITOR rules for whether new screen size is OK
   if( newSize.col <= g_iHscroll ) { return Msg( "newSize.col (%d) <= g_iHscroll (%d)", newSize.col, g_iHscroll ); }
   return Wins_CanResizeContent( newSize );
   }

// if newSize is not supported, and a supported size can be switched to:
//    it will be switched to, newSize will be updated, "OK" status will be returned
bool VideoSwitchModeToXYOk( Point &newSize ) {
   YX_t pass; pass.lin = newSize.lin; pass.col = newSize.col;
   const auto rv( EditorScreenSizeAllowed( newSize ) && ConOut::SetScreenSizeOk( pass ) );
   newSize.lin = pass.lin; newSize.col = pass.col;
   return rv;
   }

void DispNeedsRedrawCursorMoved() { if( g_CurView() ) { g_CurView()->ForceCursorMovedCondition(); } }

void UnhideCursor()  { if( ConOut::SetCursorVisibilityChanged( true  ) ) { DispNeedsRedrawCursorMoved(); } }
void HideCursor()    { if( ConOut::SetCursorVisibilityChanged( false ) ) { DispNeedsRedrawCursorMoved(); } }

GLOBAL_VAR int g_iCursorSize = 1;

void swidCursorsize( PChar dest, size_t sizeofDest, void *src ) {
   swid_int( dest, sizeofDest, g_iCursorSize );
   }

PCChar swixCursorsize( stref param ) {
   if( param.length() != 1 ) { return "CursorSize: Value must be 0 or 1 (!len)"; }
   switch( param[0] ) {
      default:  return "CursorSize: Value must be 0 or 1";
      case '0': g_iCursorSize = false; ConOut::SetCursorSize( ToBOOL(g_iCursorSize) ); return nullptr;
      case '1': g_iCursorSize = true ; ConOut::SetCursorSize( ToBOOL(g_iCursorSize) ); return nullptr;
      }
   }

void View::InsertHiLitesOfLineSeg
   ( const LINE                yLine
   , const COL                 xIndent
   , const COL                 xMax
   ,       LineColorsClipped  &alcc
   ,       const HiLiteRec *  &pFirstPossibleHiLite
   , const bool                isActiveWindow
   , const bool                isCursorLine
   ) const {
   enum { EXCLUSIVE_VIEWHILITES = 0 };
   if( EXCLUSIVE_VIEWHILITES && d_pHiLites ) {
      const int hls( d_pHiLites->InsertHiLitesOfLineSeg( yLine, xIndent, xMax, alcc, pFirstPossibleHiLite ) );  0 && DBG( "%d hls=%d", yLine, hls );
      // if( hls > 0 ) return;
      }
   DLINKC_FIRST_TO_LASTA( d_addins, dlinkAddins, pDispAddin ) {
      if( pDispAddin->VHilitLine( yLine, xIndent, alcc ) ) {
         return;
         }
      }
   NewScope {
      DLINKC_FIRST_TO_LASTA( d_addins, dlinkAddins, pDispAddin ) {
         if( pDispAddin->VHilitLineSegs( yLine, alcc ) ) {
            if( EXCLUSIVE_VIEWHILITES ) {
               return;
               }
            else {
               break;
               }
            }
         }
      }
   if( !EXCLUSIVE_VIEWHILITES && d_pHiLites ) {
      const auto hls( d_pHiLites->InsertHiLitesOfLineSeg( yLine, xIndent, xMax, alcc, pFirstPossibleHiLite ) );  0 && DBG( "%d hls=%d", yLine, hls );
      // if( hls > 0 ) return;
      }
   }

// Would be nice to get rid of GetLineForDisplay's monster param list; already
// considered throwing them all into a struct, and passing a ptr to it up & down
// the call tree.  Problem is the code growth (and speed loss) caused by this
// "repackaging".

void View::GetLineForDisplay
   ( std::string             &dest
   , const COL                xLeft
   , const COL                xWidth
   ,       LineColorsClipped &alcc
   ,       const HiLiteRec * &pFirstPossibleHiLite
   , const LINE               yLineOfFile
   , const bool               isActiveWindow
   ) const {
   const auto isActiveLine( isActiveWindow && g_CursorLine() == yLineOfFile );
   dest.replace( xLeft, xWidth, xWidth, ' ' ); // dflt for line seg is spaces (overwrites border-assign: buf.assign( scrnCols, H__ ))
   if( yLineOfFile > CFBuf()->LastLine() ) {
      alcc.PutColorRaw( Origin().col, xWidth, 0x07 );
      dest[xLeft] = (0 == g_chTrailLineDisp || 255 == g_chTrailLineDisp) ? ' ' : g_chTrailLineDisp;
      }
   else {
      alcc.PutColor( Origin().col, xWidth, ColorTblIdx::TXT );
      const auto showBlanks( CFBuf()->RevealBlanks() || isActiveLine );
      PrettifyMemcpy( dest, xLeft, xWidth, CFBuf()->PeekRawLine( yLineOfFile ), Origin().col
         ,              CFBuf()->TabWidth()
         , showBlanks ? CFBuf()->TabDispChar()   : ' '
         , showBlanks ? CFBuf()->TrailDispChar() : 0
         );
      enum { PCT_WIDTH=7 };
      if( DrawVerticalCursorHilite() && (xWidth > PCT_WIDTH) && isActiveLine ) {
         const auto percent( static_cast<UI>((100.0 * yLineOfFile) / CFBuf()->LastLine()) );
         FmtStr<PCT_WIDTH+1> pctst( " %u%% ", percent );
         stref pct( pctst.k_str() );
         dest.replace( xLeft + xWidth - pct.length(), pct.length(), pct.data() );
         }
      }
   InsertHiLitesOfLineSeg(  // patch in any hilites [which MAY be present when yLineOfFile > CFBuf()->LastLine(); think "cursorline highlight" or selecting beyond EOF]
        yLineOfFile
      , Origin().col
      , Origin().col + xWidth - 1
      , alcc, pFirstPossibleHiLite, isActiveWindow, isActiveLine
      );
   }

void Win::GetLineForDisplay
   ( const int          winNum
   , std::string       &dest
   , LineColors        &alc
   , const HiLiteRec * &pFirstPossibleHiLite
   , const LINE         yLineOfDisplay
   ) const {
   const auto oRightBorder( d_UpLeft.col + d_Size.col );  0 && DBG( "L%05d w%d [%03d..%03d]", yLineOfDisplay, winNum, this->d_UpLeft.col, oRightBorder - 1 );
   if( VisibleOnDisplayLine( yLineOfDisplay ) ) {
      NewScope {
         const auto pView( this->CurView() );
         const auto yLineOfFile( pView->Origin().lin - d_UpLeft.lin + yLineOfDisplay );
         LineColorsClipped alcc( *pView, alc, d_UpLeft.col, pView->Origin().col, d_Size.col );
         pView->GetLineForDisplay( dest, d_UpLeft.col, d_Size.col, alcc, pFirstPossibleHiLite, yLineOfFile, this == g_CurWin() );
         }
      if( d_UpLeft.col > 0 ) { // this window not on left edge? (i.e. window has visible left border?) plug in a line-draw char to make the border
         auto    &chLeftBorder( dest[ d_UpLeft.col - 1 ] );
         if     ( chLeftBorder == H__                               // 'Õ' -> 'π'
                ||chLeftBorder == HT_                               // ' ' -> 'π'
                ||chLeftBorder == HV_ ) { chLeftBorder = uint8_t(LV_); } // 'Œ' -> 'π'
         else if( chLeftBorder == RV_ ) { chLeftBorder = uint8_t(_V_); } // 'Ã' -> '∫'
         }
      auto    &chRightBorder( dest[ oRightBorder ] );
      if     ( chRightBorder == H__                                 // 'Õ' -> 'Ã'
             ||chRightBorder == HV_ ) { chRightBorder = uint8_t(RV_); }  // 'Œ' -> 'Ã'
      else if( chRightBorder == LV_ ) { chRightBorder = uint8_t(_V_); }  // 'π' -> '∫'
      }
   else if( yLineOfDisplay == d_UpLeft.lin - 1 ) {  // window's top border line?
      auto    &chRightBorder( dest[ oRightBorder ] );
      if     ( chRightBorder == H__ ) { chRightBorder = uint8_t(HB_); }  // 'Õ' -> 'À'
      else if( chRightBorder == HT_ ) { chRightBorder = uint8_t(HV_); }  // ' ' -> 'Œ'
      }
   else if( yLineOfDisplay == d_UpLeft.lin + d_Size.lin ) { // window's bottom border line?
      auto    &chRightBorder( dest[ oRightBorder ] );
      if     ( chRightBorder == H__ ) { chRightBorder = uint8_t(HT_); }  // 'Õ' -> ' '
      else if( chRightBorder == HB_ ) { chRightBorder = uint8_t(HV_); }  // 'À' -> 'Œ'
      }
   }

//
//   ViewList manipulations    ViewList manipulations    ViewList manipulations
//
PView FBUF::PutFocusOnView() { enum{ DD=0 };
   // if there is a View associated with this FBUF in the current Window's View list,
   // move it to the head of that list.  Else, create a new View.
   //
   DD&&DBG("%s(%s)", __func__, Name() );
   PView myView( nullptr );
   {
   PView pNxtView;
   auto &cvwHd( g_CurViewHd() );
   DLINK_FIRST_TO_LAST( cvwHd, dlinkViewsOfWindow, myView, pNxtView ) { // remove(), push_front()
      if( myView->FBuf() == this ) { DD&&DBG("%s(%s) found", __func__, Name() );
         DLINK_REMOVE( cvwHd, myView, dlinkViewsOfWindow );
         break;
         }
      }
   if( !myView ) { DD&&DBG("%s(%s) not found, creating", __func__, Name() ); // a View for this was not found?  Create and insert at head
      myView = new View( this, g_CurWinWr() );
      }
   DLINK_INSERT_FIRST( cvwHd, myView, dlinkViewsOfWindow );
   }
   myView->PutFocusOn();
   return myView;
   }

void FBUF::UnlinkView( PView pv ) {
   DLINK_REMOVE( d_dhdViewsOfFBUF, pv, dlinkViewsOfFBUF );
   if( d_dhdViewsOfFBUF.empty() ) {
      private_RemovedFBuf();
      }
   }

STATIC_FXN void KillView( PView pv ) { // destroy an arbitrary View
   DLINK_REMOVE( pv->wr_Win()->ViewHd, pv, dlinkViewsOfWindow );
   pv->FBuf()->UnlinkView( pv );
   delete pv;
   }

void DestroyViewList( ViewHead *pViewHd ) {
   while( auto cur=pViewHd->front() ) {
      KillView( cur );
      }
   Assert( pViewHd->empty() );
   }

void KillTheCurrentView() {
   KillView( g_CurView() );
   g_UpdtCurFBuf( g_CurView()->FBuf() );
   if( g_CurView() ) {
       g_CurView()->FBuf()->PutFocusOn();
       }
   }

bool FBUF::UnlinkAllViews() {
   while( auto pEl=d_dhdViewsOfFBUF.front() ) {
      DLINK_REMOVE_FIRST( d_dhdViewsOfFBUF     , pEl, dlinkViewsOfFBUF   );
      DLINK_REMOVE      ( pEl->wr_Win()->ViewHd, pEl, dlinkViewsOfWindow );
      delete pEl;
      }
   return private_RemovedFBuf();
   }

bool DeleteAllViewsOntoFbuf( PFBUF pFBuf ) {
   const auto fEntirelyRmvd( pFBuf->UnlinkAllViews() ); // pFBuf MAY NOT be valid after this call
   // if fEntirelyRmvd==false pFBuf's GlobalPtr is set and WE WILL return with *pFBuf still existing ()!
   SetWindowSetValidView( -1 );
   return fEntirelyRmvd;
   }

void Display_hilite_regex_err( PCChar errMsg, PCChar searchStr, int errOffset ) {
   const auto searchStrLen( Strlen(searchStr) );
   const auto tail( searchStrLen > errOffset+1 );
   ColoredStrefs         csrs;csrs.reserve( 6 );
                         csrs.emplace_back( g_colorStatus, "Regex err" );
                         csrs.emplace_back( g_colorError , errMsg );
                         csrs.emplace_back( g_colorStatus, ": " );
   if( errOffset > 1 ) { csrs.emplace_back( g_colorInfo  , stref( searchStr            , errOffset -1 )        ); } // leading OK part of pattern
                         csrs.emplace_back( g_colorError , stref( searchStr+errOffset  , 1            ), !tail );   // err char of pattern
   if( tail )          { csrs.emplace_back( g_colorInfo  , searchStr+errOffset+1                       ,  tail ); } // post-err part of pattern
   VidWrColoredStrefs( DialogLine(), 0, csrs );
   }

//
// Confirm with "Cancel" button
//
// 20050910 klg wrote
// 20150207 klg portable Lua version
//

STATIC_FXN ConfirmResponse Confirm_( int MBox_uType, PCChar pszPrompt ) {
   DBG( "Confirm_: %s", pszPrompt );
   MainThreadPerfCounter::PauseAll();
   const auto mboxrv(
#if 1
      Lua_ConfirmYes( pszPrompt ) ? crYES : crNO
#else
      Win32::Confirm_MsgBox( pszPrompt )
#endif
      );
   MainThreadPerfCounter::ResumeAll();
   return mboxrv;
   }

bool            ConIO::Confirm        ( PCChar pszPrompt ) { return Confirm_( WL( MB_YESNO      , 0 ), pszPrompt ) == crYES; }
ConfirmResponse ConIO::Confirm_wCancel( PCChar pszPrompt ) { return Confirm_( WL( MB_YESNOCANCEL, 0 ), pszPrompt ); }
