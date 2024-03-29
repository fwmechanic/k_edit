//
// Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

STIL bool CursorCannotBeInTabFill() { return g_fRealtabs && g_fTabAlign; }
STIL COL TabAlignCursorPolicy( PCFBUF pFBuf, LINE yPos, COL xPos ) { return CursorCannotBeInTabFill() ?  TabAlignedCol( pFBuf->TabWidth(), pFBuf->PeekRawLine( yPos ), xPos ) : xPos; }
STIL COL ConstrainCursorX_1  ( PCFBUF pFBuf, LINE yPos, COL xPos ) { return CursorCannotBeInTabFill() ?  ColOfNextChar( pFBuf->TabWidth(), pFBuf->PeekRawLine( yPos ), xPos ) : xPos + 1; }

STATIC_FXN bool CurView_MoveCursor_fMoved( LINE yLine, COL xColumn ) {
   const FBufLocnNow cp;
   g_CurView()->MoveCursor( yLine, xColumn );
   return cp.Moved();
   }

STIL COL CurLineCols() {
   return FBOP::LineCols( g_CurFBuf(), g_CursorLine() );
   }

bool ARG::right() { PCWrV;
   const auto xNewCol( d_fMeta
                     ? pcw->d_Size.col + pcv->Origin().col - 1
                     : ConstrainCursorX_1( pcv->FBuf(), g_CursorLine(), g_CursorCol() )
                     );
   const auto g_CursorCol_was( g_CursorCol() );
   pcv->MoveCursor( g_CursorLine(), xNewCol );  0 && DBG( "xNewCol=%d -> %d -> %d", g_CursorCol_was, xNewCol, g_CursorCol() );
   return g_CursorCol() == CurLineCols();
   }

STATIC_FXN COL ColOfFirstNonBlankChar( PCFBUF fb, LINE yLine ) {
   const auto rl( fb->PeekRawLine( yLine ) );
   if( IsStringBlank( rl ) ) { return -1; }
   const auto ix( FirstNonBlankOrEnd( rl ) );
   return ColOfFreeIdx( fb->TabWidth(), rl, ix );
   }

bool ARG::begline() {
   const auto xFirstNonBlank( ColOfFirstNonBlankChar( g_CurFBuf(), g_CursorLine() ) );
   const auto xTgt( d_fMeta ? 0 : (g_CursorCol() == xFirstNonBlank ? 0 : xFirstNonBlank) );
   return CurView_MoveCursor_fMoved( g_CursorLine(), xTgt );
   }

bool ARG::endline() {
   const auto xEoLn(  CurLineCols() );
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
      pcv->MoveCursor( pcv->Origin() );
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

void View::MoveCursor_( LINE yCursor, COL xCursor, COL visibleCharsAtCursor, bool fUpdtWUC ) {
   0 && DBG( "%s(%d,%d L %d) fUpdtWUC=%c", __func__, yCursor, xCursor, visibleCharsAtCursor, fUpdtWUC?'t':'f' );
   xCursor = std::max( COL (0), xCursor );
   yCursor = std::max( LINE(0), yCursor );
   const auto &winSize( Win()->d_Size );
   Constrain( COL(1), &visibleCharsAtCursor, winSize.col ); // ensure visibleCharsAtCursor request is satisfiable
   xCursor = TabAlignCursorPolicy( CFBuf(), yCursor, xCursor );
   // HORIZONTAL WINDOW SCROLL HANDLING
   //
   // hscroll: The number of columns that the editor scrolls the text left or
   // right when you move the cursor out of the window.  When the window
   // does not occupy the full screen, the amount scrolled is in proportion to
   // the window size.
   //
   // Text is never scrolled in increments greater than the size of the window.
   const auto hscroll( std::max( COL(1), (g_iHscroll * winSize.col) / EditScreenCols() ) );
         auto xOrigin( Origin().col );
   const auto xWinCursor( xCursor - xOrigin );
   if     ( xWinCursor <  COL(0)      ) { xOrigin -= hscroll;  if( xWinCursor < -hscroll               ) { xOrigin += xWinCursor + 1          ; } } // hscroll left?
   else if( xWinCursor >= winSize.col ) { xOrigin += hscroll;  if( xWinCursor >= hscroll + winSize.col ) { xOrigin += xWinCursor - winSize.col; } } // hscroll right?
   { // if necessary, scroll to ensure [xCursor..xRight] visible (only possible if visibleCharsAtCursor > 1)
   const auto xRight( xCursor + visibleCharsAtCursor - 1 ); visibleCharsAtCursor > 1 && 0 && DBG( "visibleCharsAtCursor=%d", visibleCharsAtCursor );
   const auto xMax  ( xOrigin + winSize.col - 1 );
   if( xRight > xMax ) { xOrigin += xRight - xMax; }
   }
   // VERTICAL WINDOW SCROLL HANDLING
   //
   // vscroll: The number of lines scrolled when you move the cursor out of the
   // window.  When the window is smaller than the full screen, the amount
   // scrolled is in proportion to the window size.
   //
   // Text is never scrolled in increments greater than the size of the window.
   const auto vscroll( std::max( COL(1), (g_iVscroll * winSize.lin) / EditScreenLines() ) );
         auto yOrigin( Origin().lin );
   const auto yWinCursor( yCursor - yOrigin );
   if( (yWinCursor >= -vscroll) && (yWinCursor < winSize.lin + vscroll) ) { // within one vscroll of the window boundaries?
      if     ( yWinCursor <  LINE(0)     ) { yOrigin -= vscroll; }
      else if( yWinCursor >= winSize.lin ) { yOrigin += vscroll; }
      }
   else {
      //  hike: the distance from the cursor to the top/bottom of the window
      //  if you move the cursor out of the window by more than the number of
      //  lines specified by vscroll, specified in percent of window size
      //
      Constrain( LINE(1), &g_iHike, 99 );
      const auto yHikeLines( std::max( LINE(1), (winSize.lin * g_iHike) / 100 ) );
      yOrigin = yCursor - yHikeLines;
      }
   ScrollOriginAndCursor_( yOrigin, xOrigin, yCursor, xCursor, fUpdtWUC );
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
   const auto saved( d_saved.fValid );
   if( saved ) {
      ScrollOriginAndCursor( d_saved );
      }
   return saved;
   }

bool ARG::savecur() { PCV;  // left here for macro programming
   pcv->SaveCur();
   return true;
   }

bool ARG::restcur() { PCV;
   if( Get_g_ArgCount() > 0 ) {
      if( !pcv->RestCur() ) {
         return fnMsg( "no cursor location saved" );
         }
      }
   else {
      pcv->SaveCur();
      fnMsg( "saved cursor location" );
      }
   return true;
   }

bool ARG::newline() {
   g_CurView()->MoveCursor( g_CursorLine() + 1, d_fMeta ? 0 : FBOP::GetSoftcrIndent( g_CurFBuf() ) );
   return true;
   }

bool ARG::setwindow() { PCV;
   switch( d_argType ) {
    break;default:      DispNeedsRedrawTotal();  // bug-masker: if color is changed interactively or in a startup macro the change did not affect all lines w/o this change
    break;case NULLARG: pcv->ScrollOriginYX( g_CursorLine(), d_cArg == 1 ? g_CursorCol() : pcv->Origin().col );
    }
   return true;
   }

bool ARG::pmlines( int direction ) { PCWrV;
   switch( d_argType ) {
    default:      return BadArg();
    case NULLARG: pcv->ScrollOrigin_Y_Abs( direction > 0 ? g_CursorLine() : g_CursorLine() - pcw->d_Size.lin + 1 );
                  return true;
    case NOARG:   {
                  const LINE scroll( std::max( LINE(1), (pcw->d_Size.lin * g_iVscroll) / EditScreenLines() ) );
                  pcv->ScrollOrigin_Y_Rel( direction * scroll );
                  return true;
                  }
    case TEXTARG: if( !StrSpnSignedInt( d_textarg.pText ) ) {
                     return BadArg();
                     }
                  pcv->ScrollOrigin_Y_Rel( direction * atoi( d_textarg.pText ) );
                  return true;
    }
   }

bool ARG::plines() { return pmlines( +1 ); }
bool ARG::mlines() { return pmlines( -1 ); }

void View::CapturePrevLineCount() { d_prevLineCount = CFBuf()->LineCount(); }

void CapturePrevLineCountAllWindows( PFBUF pFBuf, bool fIncludeCurWindow ) {
   for( size_t ix(0u), max=g_WindowCount() ; ix < max ; ++ix ) {
      const auto pWin( g_Win(ix) );
      if( (fIncludeCurWindow || pWin != g_CurWin()) && pWin->CurView()->FBuf() == pFBuf ) {
         pWin->CurViewWr()->CapturePrevLineCount();
         }
      }
   }

// sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi&&sleep 2&&echo hi

bool MoveCursorToEofAllWindows( PFBUF pFBuf, bool fIncludeCurWindow ) {
   enum { CURSOR_ON_LINE_AFTER_LAST = 1 }; // = 0 to have "tailing cursor" sit atop last line
   auto scrolledAny( false );
   for( size_t ix(0u), max=g_WindowCount() ; ix < max ; ++ix ) {
      const auto pWin( g_Win(ix) );
      if( (fIncludeCurWindow || pWin != g_CurWin()) && pWin->CurView()->FBuf() == pFBuf ) {
         const auto pView( pWin->CurViewWr() );
         if( pView->Cursor().lin >= pView->PrevLineCount() ) { // "smart tailing": if the user has moved the cursor above the tail line, don't molest the cursor
            const auto yCursor( pFBuf->LastLine() + CURSOR_ON_LINE_AFTER_LAST );
            const auto yOrigin( std::max( yCursor - pWin->d_Size.lin + 1, 0 ) );
            if(   0 != pView->Cursor().col || yCursor != pView->Cursor().lin
               || 0 != pView->Origin().col || yOrigin != pView->Origin().lin
              ) {
               pView->ScrollOriginAndCursor( yOrigin, 0, yCursor, 0 );        0 && DBG( "CurTRUE %d", pFBuf->LastLine() );
               scrolledAny = true;
               }
            }
         }
      }
   if( scrolledAny ) {
      FBOP::PrimeRedrawLineRangeAllWin( pFBuf, 0, pFBuf->LineCount() );
      }
   return scrolledAny;
   }

void FBUF::MoveCursorAllViews( LINE yLine, COL xCol ) { // NB: this affects _all Views_ referencing this!
   DLINKC_FIRST_TO_LASTA( d_dhdViewsOfFBUF, d_dlinkViewsOfFBUF, pv ) {
      pv->MoveCursor( yLine, xCol );
      }
   }

STIL bool LineFollowsPara( PFBUF fb, LINE yLine )  { return !FBOP::IsLineBlank( fb, yLine - 1 ) &&  FBOP::IsLineBlank( fb, yLine ); }
STIL bool FirstLineOfPara( PFBUF fb, LINE yLine )  { return  FBOP::IsLineBlank( fb, yLine - 1 ) && !FBOP::IsLineBlank( fb, yLine ); }
STIL bool LastLineOfPara ( PFBUF fb, LINE yLine )  { return  FBOP::IsLineBlank( fb, yLine + 1 ) && !FBOP::IsLineBlank( fb, yLine ); }

bool ARG::ppara() {
   const auto origLine( g_CursorLine() );
   const auto pFBuf( g_CurFBuf() );
   if( pFBuf->LineCount() <= g_CursorLine() ) {
      return false;
      }
   if( Get_g_ArgCount() > 0 ) { d_fMeta = !d_fMeta; } // makes this fn more useful for selection purposes
   auto yLine( g_CursorLine() + 1 );
   for( ; yLine < pFBuf->LineCount(); ++yLine ) {
      if( d_fMeta ? LastLineOfPara( pFBuf, yLine ) : FirstLineOfPara( pFBuf, yLine ) ) {
         break;
         }
      }
   g_CurView()->MoveCursor_NoUpdtWUC( yLine, g_CursorCol() );
   return g_CursorLine() != origLine;
   }

bool ARG::mpara() {
   const auto origLine( g_CursorLine() );
   if( g_CursorLine() == 0 ) {
      return false;
      }
   // if( Get_g_ArgCount() > 0 ) { d_fMeta = !d_fMeta; } // makes this fn more useful for selection purposes
   auto yLine( g_CursorLine() - 1 );
   const auto pFBuf( g_CurFBuf() );
   for( ; yLine; --yLine ) {
      if( d_fMeta ? LineFollowsPara( pFBuf, yLine ) : FirstLineOfPara( pFBuf, yLine ) ) {
         break;
         }
      }
   g_CurView()->MoveCursor_NoUpdtWUC( yLine, g_CursorCol() );
   return g_CursorLine() != origLine;
   }

STATIC_FXN bool SameIndentRange( PCFBUF fb, LINE *yMin, LINE *yMax, LINE start ) { // skip language-specific lines
   *yMin = *yMax = -1;
   auto goal(-1);
   auto yLine( start );
   for( ; yLine; --yLine ) {
      goal = ColOfFirstNonBlankChar( fb, yLine );
      if( goal >= 0 ) {
         break;
         }
      }
   if( goal < 0 ) { return false; }
   for( ; yLine ; --yLine ) {
      const COL dent( ColOfFirstNonBlankChar( fb, yLine ) );
      if( dent >= 0 ) {
         if     ( dent == goal ) { *yMin = yLine; }
         else if( dent <  goal ) { break;         }
         }
      }
   if( *yMin < 0 ) { *yMin = 0; }
   for( yLine = start ; yLine < fb->LineCount(); ++yLine ) {
      const auto dent( ColOfFirstNonBlankChar( fb, yLine ) );
      if( dent >= 0 ) {
         if     ( dent == goal ) { *yMax = yLine; }
         else if( dent <  goal ) { break;         }
         }
      }
   if( *yMax < 0 ) { *yMax = fb->LineCount() - 1; }
   return true;
   }

STATIC_FXN bool cursorToBoundOfIndentBlock( int bottom ) {
   LINE yMin, yMax;
   const auto rv( SameIndentRange( g_CurFBuf(), &yMin, &yMax, g_CursorLine() ) );
   if( rv ) { g_CurView()->MoveCursor_NoUpdtWUC( (bottom > 0) ? yMax : yMin, g_CursorCol() ); }
   return rv;
   }

bool ARG::mib() { return cursorToBoundOfIndentBlock( -1 ); }
bool ARG::pib() { return cursorToBoundOfIndentBlock( +1 ); }
