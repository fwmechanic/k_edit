//
// Copyright 1991 - 2013 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

void swidHscroll( PChar dest, size_t sizeofDest, void *src ) { swid_int( dest, sizeofDest, g_iHscroll ); }
void swidVscroll( PChar dest, size_t sizeofDest, void *src ) { swid_int( dest, sizeofDest, g_iVscroll ); }

bool swixHscroll( PCChar param ) {
   g_iHscroll = atoi( param );
   Constrain( 1, &g_iHscroll, EditScreenCols() - 1 );
   return true;
   }

bool swixVscroll( PCChar param ) {
   g_iVscroll = atoi( param );
   Constrain( 1, &g_iVscroll, EditScreenLines() - 1 );
   return true;
   }

STATIC_FXN COL ConstrainCursorX( PFBUF pFBuf, LINE yPos, COL xPos, COL xBias ) {
   PCChar ptr,eos;
   if( CursorCannotBeInTabFill() && pFBuf->PeekRawLineExists( yPos, &ptr, &eos ) ) {
      return TabAlignedCol( pFBuf->TabWidth(), ptr, eos, xPos, xBias );
      }
   return xPos + xBias;
   }

STATIC_FXN bool CurView_MoveCursor_fMoved( LINE yLine, COL xColumn ) {
   const FBufLocnNow cp;
   g_CurView()->MoveCursor( yLine, xColumn );
   return cp.Moved();
   }

STIL COL CurLineLenTabs2Spcs() {
   return FBOP::LineCols( g_CurFBuf(), g_CursorLine() );
   }

bool ARG::right() { PCWrV;
   const auto xNewCol( d_fMeta
                    ? pcw->d_Size.col + pcv->Origin().col - 1
                    : ConstrainCursorX( pcv->FBuf(), g_CursorLine(), g_CursorCol(), +1 )
                    );
   const auto g_CursorCol_was( g_CursorCol() );
   pcv->MoveCursor( g_CursorLine(), xNewCol );
   0 && DBG( "xNewCol=%d -> %d -> %d", g_CursorCol_was, xNewCol, g_CursorCol() );
   return g_CursorCol() == CurLineLenTabs2Spcs();
   }

STATIC_FXN COL ColOfFirstNonBlankChar( PCFBUF fb, LINE yLine ) {
   const auto rl( fb->PeekRawLine( yLine ) );
   if( IsStringBlank( rl ) )           return -1;
   const auto ix( ToNextNonBlankOrEnd( rl ) );
   return ColOfFreeIdx( fb->TabWidth(), rl, ix );
   }

bool ARG::begline() {
   const auto xFirstNonBlank( ColOfFirstNonBlankChar( g_CurFBuf(), g_CursorLine() ) );
   const auto xTgt( d_fMeta ? 0 : (g_CursorCol() == xFirstNonBlank ? 0 : xFirstNonBlank) );
   return CurView_MoveCursor_fMoved( g_CursorLine(), xTgt );
   }

bool ARG::endline() {
   const auto xEoLn(  CurLineLenTabs2Spcs() );
   const auto xEoWin( g_CurView()->Origin().col + g_CurWin()->d_Size.col - 1 );
   return CurView_MoveCursor_fMoved( g_CursorLine(), d_fMeta || (g_CursorCol() == xEoLn) ? xEoWin : xEoLn );
   }

bool ARG::home() { PCWrV;
   const FBufLocnNow cp;
   if( d_fMeta ) {
      pcv->MoveCursor(
           pcv->Origin().lin + pcw->d_Size.lin - 1
         , pcv->Origin().col + pcw->d_Size.col - 1
         );
      }
   else {
      pcv->Origin().ScrollTo();
      }
   return cp.Moved();
   }

bool ARG::left() { PCV ; return CurView_MoveCursor_fMoved( pcv->Cursor().lin, d_fMeta ? pcv->Origin().col : pcv->Cursor().col - 1 ); }

bool ARG::up()   { PCV ; return CurView_MoveCursor_fMoved( d_fMeta ?                   pcv->Origin().lin     : g_CursorLine()-1, g_CursorCol() ); }
bool ARG::down() { PCWV; return CurView_MoveCursor_fMoved( d_fMeta ? pcw->d_Size.lin + pcv->Origin().lin - 1 : g_CursorLine()+1, g_CursorCol() ); }

bool ARG::begfile() { return CurView_MoveCursor_fMoved( 0                       , g_CursorCol() ); }
bool ARG::endfile() { return CurView_MoveCursor_fMoved( g_CurFBuf()->LineCount(), g_CursorCol() ); }

bool ARG::backtab() { return CurView_MoveCursor_fMoved( g_CursorLine(), ColPrevTabstop( g_CurFBuf()->TabWidth(), g_CursorCol() ) ); }
bool ARG::tab()     { return CurView_MoveCursor_fMoved( g_CursorLine(), ColNextTabstop( g_CurFBuf()->TabWidth(), g_CursorCol() ) ); }

STATIC_FXN bool pmpage( int dir ) {
   const FBufLocnNow cp;
   g_CurView()->ScrollByPages( dir );
   return cp.Moved();
   }

bool ARG::mpage() { return pmpage( -1 ); }
bool ARG::ppage() { return pmpage( +1 ); }

void View::MoveCursor_( LINE yCursor, COL xCursor, COL xWidth, bool fUpdtWUC ) {
   0 && DBG( "%s(%d,%d) fUpdtWUC=%c", __func__, yCursor, xCursor, fUpdtWUC?'t':'f' );
   NoLessThan( &xCursor, 0 );
   NoLessThan( &yCursor, 0 );
   xCursor = ConstrainCursorX( d_pFBuf, yCursor, xCursor, 0 );
   const auto winHeight( d_pWin->d_Size.lin );
   const auto winWidth ( d_pWin->d_Size.col );

   // HORIZONTAL WINDOW SCROLL HANDLING
   //
   // hscroll: The number of columns that the editor scrolls the text left or
   // right when the you move the cursor out of the window.  When the window
   // does not occupy the full screen, the amount scrolled is in proportion to
   // the window size.
   //
   // Text is never scrolled in increments greater than the size of the window.

   const auto hscrollCols( Max( 1, (g_iHscroll * winWidth) / EditScreenCols() ) );
         auto xWinOrigin( Origin().col );
   const auto xWinCursor( xCursor - xWinOrigin );
   if( xWinCursor <         0 ) { xWinOrigin -= hscrollCols;  if( xWinCursor < -hscrollCols            )  xWinOrigin += xWinCursor + 1       ; } // hscroll left?
   else
   if( xWinCursor >= winWidth ) { xWinOrigin += hscrollCols;  if( xWinCursor >= hscrollCols + winWidth )  xWinOrigin += xWinCursor - winWidth; } // hscroll right?
   {
   const auto  xWinCurs( xCursor - xWinOrigin );
   0 && DBG( "xWinCurs=%d", xWinCurs );
   0 && DBG( "xWidth=%d", xWidth );
   NoMoreThan( &xWidth, winWidth );
   const auto xMax  ( xWinOrigin + winWidth - 1 );
   const auto xRight( xCursor    +   xWidth - 1 );
   const auto xShort( xRight     -         xMax );
   if( xShort > 0 )  xWinOrigin += xShort;
   }

   // VERTICAL WINDOW SCROLL HANDLING
   //
   // vscroll: The number of lines scrolled when you move the cursor out of the
   // window.  When the window is smaller than the full screen, the amount
   // scrolled is in proportion to the window size.
   //
   // Text is never scrolled in increments greater than the size of the window.

   const auto vscrollCols( Max( 1, (g_iVscroll * winHeight) / EditScreenLines() ) );

         auto yWinOrigin( Origin().lin );
   const auto yWinCursor( yCursor - yWinOrigin );

   if( (yWinCursor >= -vscrollCols) && (yWinCursor < winHeight + vscrollCols) ) { // within one vscroll of the window boundaries?
      if( yWinCursor <          0 ) { yWinOrigin -= vscrollCols; }
      else
      if( yWinCursor >= winHeight ) { yWinOrigin += vscrollCols; }
      }
   else {
      //  hike: the distance from the cursor to the top/bottom of the window
      //  if you move the cursor out of the window by more than the number of
      //  lines specified by vscroll, specified in percent of window size
      //
      Constrain( 1, &g_iHike, 99 );
      const auto yHikeLines( Max( 1, (winHeight * g_iHike) / 100 ) );

#if 1
      yWinOrigin = yCursor - yHikeLines;
#else
         // Using the calcs below, hike is directional: my idea was that in many
         // monotonic-access cases like m/psearch, m/ppara, etc. this makes sense
         // (you can see where you're going/where you've been), but for random
         // access cases like mfgrep where a file is entered at a match point and
         // the current cursor position is irrelevant, this can create random
         // behavior.
         //
         // I could probably add a global variable which could be set to control
         // the intent/context of the scroll, but the benefit is probably not
         // worth it.
         //
         // 20070326 kgoodwin
         //
      yWinOrigin = yWinCursor < 0
                 ? yCursor - yHikeLines                // scrolling toward line 0
                 : yCursor - (winHeight - yHikeLines)  // scrolling toward maxline
                 ;
#endif
      }

   NoLessThan( &xWinOrigin, 0 );
   NoLessThan( &yWinOrigin, 0 );

   ScrollOriginAndCursor_( yWinOrigin, xWinOrigin, yCursor, xCursor, fUpdtWUC );
   }

#if 0

// BUGBUG need to define the appropriate place to "drop this anchor"!

STATIC_FXN void CopyCurrentCursLocnToSaved() { PCV;
   pcv->SavePrevCur();
   }

#endif

void View::ScrollToPrev() {
   ULC_Cursor svCur = d_current;
   ScrollOriginAndCursor( d_prev );
   d_prev = svCur;
   }

bool View::RestCur() { // selecting
   if( !d_saved.fValid )
      return false;

   ScrollOriginAndCursor( d_saved );
   return true;
   }

bool ARG::savecur() { PCV;  // left here for macro programming
   pcv->SaveCur();
   return true;
   }

bool ARG::restcur() { PCV;
   if( ArgCount() > 0 ) {
      if( !pcv->RestCur() )
         return fnMsg( "no cursor location saved" );
      }
   else {
      pcv->SaveCur();
      Msg( "saved cursor location" );
      }
   return true;
   }

bool ARG::newline() {
   g_CurView()->MoveCursor( g_CursorLine() + 1, d_fMeta ? 0 : FBOP::GetSoftcrIndent( g_CurFBuf() ) );
   return true;
   }

bool ARG::setwindow() { PCV;
   switch( d_argType ) {
    default:      DispNeedsRedrawTotal();  // bug-masker: if color is changed interactively or in a startup macro the change did not affect all lines w/o this change
                  return true;

    case NULLARG: pcv->ScrollOriginYX( g_CursorLine(), d_cArg == 1 ? g_CursorCol() : pcv->Origin().col );
                  return true;
    }
   }

bool ARG::pmlines( int direction ) { PCWrV;
   switch( d_argType ) {
    default:      return BadArg();
    case NULLARG: pcv->ScrollOrigin_Y_Abs( direction > 0 ? g_CursorLine() : g_CursorLine() - pcw->d_Size.lin + 1 );
                  return true;
    case NOARG:   {
                  const LINE scroll( Max( 1, (pcw->d_Size.lin * g_iVscroll) / EditScreenLines() ) );
                  pcv->ScrollOrigin_Y_Rel( direction * scroll );
                  return true;
                  }
    case TEXTARG: if( !StrSpnSignedInt( d_textarg.pText ) )
                     return BadArg();

                  pcv->ScrollOrigin_Y_Rel( direction * atoi( d_textarg.pText ) );
                  return true;
    }
   }

bool ARG::plines() { return pmlines( +1 ); }
bool ARG::mlines() { return pmlines( -1 ); }

bool MoveCursorToEofAllWindows( PFBUF pFBuf, bool fIncludeCurWindow ) {
   enum { CURSOR_ON_LINE_AFTER_LAST = 1 }; // = 0 to have "tailing cursor" sit atop last line
   auto retVal( false );
   for( auto ix(0), max=g_iWindowCount() ; ix < max ; ++ix ) {
      const auto pWin( g_Win(ix) );
      if( (fIncludeCurWindow || pWin != g_CurWin()) && pWin->CurView()->FBuf() == pFBuf ) {
         const auto pView( pWin->CurViewWr() );
         const auto yCursor( pFBuf->LastLine() + CURSOR_ON_LINE_AFTER_LAST );
         const auto yOrigin( Max( yCursor - pWin->d_Size.lin + 1, 0 ) );
         if(   0       != pView->Cursor().col
            || 0       != pView->Origin().col
            || yCursor != pView->Cursor().lin
            || yOrigin != pView->Origin().lin
           ) {
            pView->ScrollOriginAndCursor( yOrigin, 0, yCursor, 0 );
            0 && DBG( "CurTRUE %d", pFBuf->LastLine() );
            retVal = true;
            }
         }
      }

   if( retVal )
      FBOP::PrimeRedrawLineRangeAllWin( pFBuf, 0, pFBuf->LineCount() );

   return retVal;
   }

void FBUF::MoveCursorAllViews( LINE yLine, COL xCol ) { // NB: this affects _all Views_ referencing this!
   DLINKC_FIRST_TO_LASTA( d_dhdViewsOfFBUF, dlinkViewsOfFBUF, pv ) {
      pv->MoveCursor( yLine, xCol );
      }
   }

STIL bool LineFollowsPara( PFBUF fb, LINE yLine )  { return !FBOP::IsLineBlank( fb, yLine - 1 ) &&  FBOP::IsLineBlank( fb, yLine ); }
STIL bool FirstLineOfPara( PFBUF fb, LINE yLine )  { return  FBOP::IsLineBlank( fb, yLine - 1 ) && !FBOP::IsLineBlank( fb, yLine ); }
STIL bool LastLineOfPara ( PFBUF fb, LINE yLine )  { return  FBOP::IsLineBlank( fb, yLine + 1 ) && !FBOP::IsLineBlank( fb, yLine ); }

bool ARG::ppara() {
   const auto origLine( g_CursorLine() );
   const auto pFBuf( g_CurFBuf() );
   if( pFBuf->LineCount() <= g_CursorLine() )
      return false;

   if( ArgCount() > 0 )  d_fMeta = !d_fMeta; // makes this fn more useful for selection purposes

   auto yLine( g_CursorLine() + 1 );
   for( ; yLine < pFBuf->LineCount(); ++yLine )
      if( d_fMeta ? LastLineOfPara( pFBuf, yLine ) : FirstLineOfPara( pFBuf, yLine ) )
          break;

   g_CurView()->MoveCursor_NoUpdtWUC( yLine, g_CursorCol() );
   return g_CursorLine() != origLine;
   }

bool ARG::mpara() {
   const auto origLine( g_CursorLine() );
   if( g_CursorLine() == 0 )
      return false;

   // if( ArgCount() > 0 )  d_fMeta = !d_fMeta; // makes this fn more useful for selection purposes

   auto yLine( g_CursorLine() - 1 );
   const auto pFBuf( g_CurFBuf() );
   for( ; yLine; --yLine )
      if( d_fMeta ? LineFollowsPara( pFBuf, yLine ) : FirstLineOfPara( pFBuf, yLine ) )
          break;

   g_CurView()->MoveCursor_NoUpdtWUC( yLine, g_CursorCol() );
   return g_CursorLine() != origLine;
   }

STATIC_FXN bool SameIndentRange( PCFBUF fb, LINE *yMin, LINE *yMax, LINE start ) { // skip language-specific lines
   *yMin = *yMax = -1;
   auto goal(-1);
   auto yLine( start );
   for( ; yLine; --yLine ) {
      goal = ColOfFirstNonBlankChar( fb, yLine );
      if( goal >= 0 )
         break;
      }
   if( goal < 0 ) return false;

   for( ; yLine ; --yLine ) {
      const COL dent( ColOfFirstNonBlankChar( fb, yLine ) );
      if( dent >= 0 ) {
         if     ( dent == goal ) *yMin = yLine;
         else if( dent <  goal ) break;
         }
      }
   if( *yMin < 0 ) *yMin = 0;

   for( yLine = start ; yLine < fb->LineCount(); ++yLine ) {
      const auto dent( ColOfFirstNonBlankChar( fb, yLine ) );
      if( dent >= 0 ) {
         if     ( dent == goal ) *yMax = yLine;
         else if( dent <  goal ) break;
         }
      }
   if( *yMax < 0 ) *yMax = fb->LineCount() - 1;

   return true;
   }

STATIC_FXN bool cursorToBoundOfIndentBlock( int bottom ) {
   LINE yMin, yMax;
   const auto rv( SameIndentRange( g_CurFBuf(), &yMin, &yMax, g_CursorLine() ) );
   if( !rv ) return false;
   g_CurView()->MoveCursor_NoUpdtWUC( (bottom>0)?yMax:yMin, g_CursorCol() );
   return true;
   }

bool ARG::mib() { return cursorToBoundOfIndentBlock( -1 ); }
bool ARG::pib() { return cursorToBoundOfIndentBlock( +1 ); }
