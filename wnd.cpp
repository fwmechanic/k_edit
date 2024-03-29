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

#include "ed_main.h"

Win::~Win() = default;

GLOBAL_VAR TGlobalStructs g__;

GLOBAL_VAR PFBUF s_curFBuf;       // not literally static (s_), but s/b treated as such!

constexpr int BORDER_WIDTH =  1;
constexpr int MAX_WINDOWS  = 10;

STIL bool CanCreateWin()  { return g_WindowCount() < MAX_WINDOWS; }
STIL bool WidxInRange( int widx )  {  return widx >= 0 && widx < g_WindowCount(); }

#define  AssertWidx( widx )   Assert( WidxInRange( widx ) );

bool Win::GetCursorForDisplay( Point *pt ) const {
   const auto pcv( d_ViewHd.front() );
   pt->lin = d_UpLeft.lin + (pcv->Cursor().lin - pcv->Origin().lin) + MinDispLine();
   pt->col = d_UpLeft.col + (pcv->Cursor().col - pcv->Origin().col);
   return true; // used to chain (with CursorLocnOutsideView_Get)
   }

void View::Event_Win_Resized( const Point &newSize ) { 0 && DBG( "%s %s", __func__, CFBuf()->Name() );
   HiliteAddin_Event_WinResized();
   }

void Win::Event_Win_Reposition( const Point &newUlc ) {
   if( d_UpLeft != newUlc ) { 0 && DBG( "%s[%d] ulcYX(%d,%d)->(%d,%d)", __func__, d_wnum,  d_UpLeft.lin, d_UpLeft.col, newUlc.lin, newUlc.col );
       d_UpLeft  = newUlc;
      }
   }

void Win::Event_Win_Resized( const Point &newSize, const Point &newSizePct ) {
   if( d_Size != newSize ) { 0 && DBG( "%s[%d] size(%d,%d)->(%d,%d)", __func__, d_wnum,  d_Size.lin, d_Size.col, newSize.lin, newSize.col );
       d_Size  = newSize;
      if( SizePct() != newSizePct ) { 0 && DBG( "%s[%d] pctg(%d%%,%d%%)->(%d%%,%d%%)", __func__, d_wnum,  SizePct().lin, SizePct().col, newSizePct.lin, newSizePct.col );
          SizePct_set( newSizePct );
         }
      auto pv( d_ViewHd.front() );
      pv->EnsureWinContainsCursor();
      DLINKC_FIRST_TO_LAST( d_ViewHd, d_dlinkViewsOfWindow, pv ) {
         pv->Event_Win_Resized( newSize );
         }
      }
   }

void Win::Event_Win_Resized( const Point &newSize ) {
   Event_Win_Resized( newSize, SizePct() );
   }

STATIC_FXN int NonWinDisplayLines() { return ScreenLines() - EditScreenLines(); }
STATIC_FXN int NonWinDisplayCols()  { return 0; }

bool Wins_CanResizeContent( const Point &newSize ) {
   const auto existingSplitVertical( g_WindowCount() > 1 && g_Win(0)->d_UpLeft.lin == g_Win(1)->d_UpLeft.lin );
   auto min_size_x( NonWinDisplayCols() ); auto min_size_y( NonWinDisplayLines() );
   if( existingSplitVertical ) {
      min_size_x += (MIN_WIN_WIDTH  * g_WindowCount()) + (BORDER_WIDTH * (g_WindowCount() - 1));
      min_size_y +=  MIN_WIN_HEIGHT;
      }
   else {
      min_size_x +=  MIN_WIN_WIDTH;
      min_size_y += (MIN_WIN_HEIGHT * g_WindowCount()) + (BORDER_WIDTH * (g_WindowCount() - 1));
      }
   const auto rv(// g_WindowCount() == 1 &&
                    newSize.col >= min_size_x
                 && newSize.lin >= min_size_y
                );
   0 && DBG( "can%s resize", rv?"":"not" );
   return rv;
   }

void Wins_ScreenSizeChanged( const Point &newSize ) {
   const Point newWinRgnSize( newSize, -NonWinDisplayLines(), -NonWinDisplayCols() );
   if( g_WindowCount() == 1 ) {
      g_CurWinWr()->Event_Win_Resized( newWinRgnSize );
      }
   else { // multiwindow resize
     #define MW_RESIZE( aaa, bbb )                                                                                       \
         Point newNonBorderSize( newWinRgnSize );                                                                        \
               newNonBorderSize.aaa -= BORDER_WIDTH*(g_WindowCount()-1);                                                 \
         auto curNonBorderSize( 0 ); /* excluding borders */                                                             \
         for( const auto &pWin : g__.aWindow ) {                                                                         \
            curNonBorderSize += pWin->d_Size.aaa;                                                                        \
            }                                                                                                            \
         if( newNonBorderSize.aaa != curNonBorderSize ) { /* grow/shrink all windows proportional to their d_size_pct */ \
            0 && DBG( "%s %s:%d->%d", __func__, #aaa, curNonBorderSize, newNonBorderSize.aaa );                          \
            auto ulc( newNonBorderSize.aaa + (BORDER_WIDTH*(g_WindowCount()-1)) );                                       \
            for( auto it( g__.aWindow.rbegin() ); it != g__.aWindow.rend(); ++it ) {                                     \
               const auto pW( *it );                                                                                     \
               const auto size_( pW->d_Size.aaa );                                                                       \
               int newSize_( (newNonBorderSize.aaa * static_cast<double>(pW->SizePct().aaa)) / 100 );                    \
               if( newNonBorderSize.aaa > curNonBorderSize ) {                                                           \
                  NoLessThan( &newSize_, size_ );                                                                        \
                  }                                                                                                      \
               const auto delta( ulc - newSize_ );                                                                       \
               const auto iw( std::distance( g__.aWindow.begin(), it.base()) -1 );                                       \
               0 && DBG( "Win[%" PR_PTRDIFFT "] size_.%s %d->%d delta=%" PR_SIZET, iw, #aaa, size_, newSize_, delta );   \
               if( 0==iw && delta > 0 ) { newSize_ += delta; }  /* 0th element and space left?  consume it! */           \
               { Point tmp; tmp.aaa=ulc - newSize_, tmp.bbb=pW->d_UpLeft    .bbb, pW->Event_Win_Reposition( tmp ); }     \
               { Point tmp; tmp.aaa=      newSize_, tmp.bbb=newNonBorderSize.bbb, pW->Event_Win_Resized   ( tmp ); }     \
               ulc -= newSize_ + BORDER_WIDTH;                                                                           \
               }                                                                                                         \
            }                                                                                                            \
         else if( newNonBorderSize.bbb != g_Win(0)->d_Size.bbb ) { /* the easy dimension */                              \
            0 && DBG( "%s %s:%d->%d", __func__, #bbb, g_Win(0)->d_Size.bbb, newNonBorderSize.bbb );                      \
            for( auto it( g__.aWindow.rbegin() ); it != g__.aWindow.rend(); ++it ) {                                     \
               const auto pW( *it );                                                                                     \
               { Point tmp; tmp.aaa=pW->d_Size.aaa, tmp.bbb=newNonBorderSize.bbb, pW->Event_Win_Resized( tmp ); }        \
               }                                                                                                         \
            }
      const auto existingSplitVertical( g_WindowCount() > 1 && g_Win(0)->d_UpLeft.lin == g_Win(1)->d_UpLeft.lin );
      if( existingSplitVertical ) { MW_RESIZE( col, lin ) } // splitVert
      else                        { MW_RESIZE( lin, col ) } // splitHoriz
     #undef MW_RESIZE
      }
   }

STATIC_FXN int PWinToWidx( PCWin tgt ) {
   for( auto it( g__.aWindow.cbegin() ) ; it != g__.aWindow.cend() ; ++it ) {
      if( *it == tgt ) {
         return std::distance( g__.aWindow.cbegin(), it );
         }
      }
   Assert( !"couldn't find PWin!" );
   return -1;
   }

void Win::Write( FILE *fout ) const {
   fprintf( fout, "> %d %d %d %d\n"
      , d_UpLeft.col
      , d_UpLeft.lin
      , d_Size.col
      , d_Size.lin
      );
   }

void Win::Maximize() {
   d_wnum = 0;
   d_UpLeft.col = 0;
   d_UpLeft.lin = 0;
   d_Size.col = EditScreenCols();
   d_Size.lin = EditScreenLines();
   SizePct_set( Point( 100, 100 ) );
   // DBG( "%s", __func__ );
   }

Win::Win()
   {
   Maximize();
   }

Win::Win( Win &parent_, bool fSplitVertical, int columnOrLineToSplitAt )
   { // ! parent_ is a reference since this is a COPY CTOR
   parent_.DispNeedsRedrawAllLines(); // in the horizontal-split case this is somewhat overkill...
   // CAREFUL HERE!  Order is important because parent.d_Size.lin/col IS MODIFIED _AND USED_ herein!
   const auto &parent( parent_ ); // parent_ SHALL NOT be modified until this dims have been set
   Point newParentSize, newParentSizePct;
  #define SPLIT_IT( aaa, bbb )                                                        \
        this->d_Size.aaa = parent.d_Size.aaa                                        ; \
        this->d_Size.bbb = parent.d_Size.bbb - columnOrLineToSplitAt - 2            ; \
       newParentSize.aaa = parent.d_Size.aaa                                        ; \
       newParentSize.bbb = parent.d_Size.bbb -   (this->d_Size.bbb  + BORDER_WIDTH) ; \
      this->d_UpLeft.aaa = parent.d_UpLeft.aaa                                      ; \
      this->d_UpLeft.bbb = parent.d_UpLeft.bbb + (newParentSize.bbb + BORDER_WIDTH) ; \
      { Point tmp                                                                   ; \
        tmp.aaa = 100 /* _ASSUMING_ uniform split-type */                           ; \
        tmp.bbb = (parent.SizePct().bbb * this->d_Size.bbb)                           \
                / (parent.d_Size.bbb - BORDER_WIDTH)                                ; \
        this->SizePct_set( tmp );                                                     \
      }                                                                               \
      newParentSizePct.aaa = parent.SizePct().aaa                                   ; \
      newParentSizePct.bbb = parent.SizePct().bbb                                     \
                           -  this->SizePct().bbb                                   ; \
      0 && DBG( "%s: src=%d=%d%%->%d=%d%%, new=%d=%d%%", __func__,                    \
                parent.d_Size.bbb, parent.  SizePct().bbb                             \
              , newParentSize.bbb, newParentSizePct  .bbb                             \
              ,  this->d_Size.bbb,    this->SizePct().bbb                             \
              );

   if( fSplitVertical ) {  SPLIT_IT( lin, col )  }
   else                 {  SPLIT_IT( col, lin )  }
  #undef SPLIT_IT

   parent_.Event_Win_Resized( newParentSize, newParentSizePct ); // feed newParentSize back into parent
   // clone all of original window's Views in a new list bound to the new window  !BUGBUG a different approach should be created!
   for( auto view( parent_.CurView() ) ; view ; view = DLINK_NEXT( view, d_dlinkViewsOfWindow ) ) {
      auto dupdPF( new View( *view, this ) ); // DO NOT inline the 'new View( ... )' into DLINK_INSERT_LAST _macro_ !!!
      DLINK_INSERT_LAST( d_ViewHd, dupdPF, d_dlinkViewsOfWindow );
      }
   }

Win::Win( PCChar pC ) { 0 && DBG( "RdSF: WIN-GEOM '%s'", pC );
   sscanf( pC, " %d %d %d %d "  // Used during ReadStateFile processing ONLY!
      , &d_UpLeft.col
      , &d_UpLeft.lin
      , &d_Size.col
      , &d_Size.lin
      );                0 && DBG( "RdSF: WIN-GEOM %d %d %d %d", d_UpLeft.col, d_UpLeft.lin, d_Size.col, d_Size.lin );
   }

STATIC_FXN PWin SaveNewWin( PWin newWin ) {
   g__.aWindow.push_back( newWin );
   return newWin;
   }

STATIC_FXN void SetWindowIdx( int widx ) {
   AssertWidx( widx );
   g__.ixCurrentWin = widx;
   }

void InitNewWin( PCChar pC ) { // Used during ReadStateFile processing only!
   if( CanCreateWin() ) {
      SaveNewWin( new Win(pC) );
      SetWindowIdx( g_WindowCount()-1 );
      }
   }

void CreateWindow0() { // Used before ReadStateFile processing only!
   SaveNewWin( new Win );
   }

void SetWindow0() { // Used during ReadStateFile processing only!
   switch( g_WindowCount() ) {
   // break;case 0: SaveNewWin( new Win )      ;  // state file had NO windows?
      break;case 1: g__.aWindow[0]->Maximize() ;  // state file had one window, but its size may not equal the size of the console
      }
   SetWindowIdx( 0 );
   }

int cmp_win( PCWin w1, PCWin w2 ) { // used by Lua: l_register_Win_object
   if( w1->d_UpLeft.lin < w2->d_UpLeft.lin ) { return -1; } // w1 < w2
   if( w1->d_UpLeft.lin > w2->d_UpLeft.lin ) { return  1; } // w1 > w2
   if( w1->d_UpLeft.col < w2->d_UpLeft.col ) { return -1; } // w1 < w2
   if( w1->d_UpLeft.col > w2->d_UpLeft.col ) { return  1; } // w1 > w2
                                               return  0;   // w1 = w2 ?
   }

STATIC_FXN void SortWinArray() {
   const auto tmpCurWin( g_CurWin() );  // needed to update g_CurWin()
   std::sort( g__.aWindow.begin(), g__.aWindow.end(), []( PCWin w1, PCWin w2 ) { return w1->d_UpLeft < w2->d_UpLeft; } );
   for( auto it( g__.aWindow.begin() ) ; it != g__.aWindow.end() ; ++it ) {
      (*it)->d_wnum = std::distance( g__.aWindow.begin(), it );
      }
   SetWindowIdx( tmpCurWin->d_wnum );  // update g_CurWin()
   }

PWin SplitCurWnd( bool fSplitVertical, int columnOrLineToSplitAt ) {
   if( !CanCreateWin() ) {
      Msg( "Too many windows" );
      return nullptr;
      }
   if( g_WindowCount() > 1 ) {
      const auto existingSplitVertical( g_Win(0)->d_UpLeft.lin == g_Win(1)->d_UpLeft.lin );
      if( existingSplitVertical != fSplitVertical ) {
         Msg( "cannot split %s when previous splits %s", fSplitVertical?"Vertical":"Horizontal", existingSplitVertical?"Vertical":"Horizontal" );
         return nullptr;
         }
      }
   const auto pWin( g_CurWinWr() );
   if(   ( fSplitVertical && (columnOrLineToSplitAt < MIN_WIN_WIDTH  || pWin->d_Size.col - columnOrLineToSplitAt < MIN_WIN_WIDTH ))
      || (!fSplitVertical && (columnOrLineToSplitAt < MIN_WIN_HEIGHT || pWin->d_Size.lin - columnOrLineToSplitAt < MIN_WIN_HEIGHT))
     ) {
      Msg( "Window too small to split" );
      return nullptr;
      }
// const Point svCursLocn( g_CurView()->Cursor()     ); // unnecessary?  No, this does something useful ...
   const auto  svUlcLine ( g_CurView()->Origin().lin ); // ... (ulc of orig window tends to scroll without it)
   // top of new window is at cursor's line
   if( !fSplitVertical ) {
      g_CurView()->PokeOriginLine_HACK( g_CursorLine() ? g_CursorLine() - 1 : g_CursorLine() );
      }                                  0 && DBG( "%s+ from w%" PR_SIZET " of %" PR_SIZET, __func__, g_CurWindowIdx(), g_WindowCount() );
   auto newWin( SaveNewWin( new Win( *pWin, fSplitVertical, columnOrLineToSplitAt ) ) );
   SortWinArray();
   if( !fSplitVertical ) {
      g_CurView()->PokeOriginLine_HACK( svUlcLine );
      }
// g_CurView()->MoveCursor( svCursLocn );
   newWin->CurViewWr()->HiliteAddins_Init();  // here since not called by SetWindowSetValidView
   return newWin;
   }

STATIC_FXN bool WindowsCanBeMerged( PCWin pw1, PCWin pw2 ) {
  #define  MERGEABLE( aaa, bbb ) \
      ( \
         pw1->d_Size  .aaa == pw2->d_Size  .aaa \
      && pw1->d_UpLeft.aaa == pw2->d_UpLeft.aaa \
      && (  (pw1->d_UpLeft.bbb + pw1->d_Size.bbb + BORDER_WIDTH) == pw2->d_UpLeft.bbb \
         || (pw2->d_UpLeft.bbb + pw2->d_Size.bbb + BORDER_WIDTH) == pw1->d_UpLeft.bbb \
         ) \
      )
   return    MERGEABLE( lin, col )
          || MERGEABLE( col, lin );
  #undef MERGEABLE
   }

STATIC_FXN void CloseWindow_( PWin pWinToClose, PWin pWinToMergeTo ) {
         auto wixToMergeTo( pWinToMergeTo->d_wnum );
   const auto wixToClose(   pWinToClose->d_wnum );    1 && DBG( "%s merge %d to %d of %" PR_SIZET, __func__, wixToClose, wixToMergeTo, g_WindowCount() );
   {
   DLINKC_FIRST_TO_LASTA( pWinToClose->d_ViewHd, d_dlinkViewsOfWindow, pv ) {
      const auto pFBuf( pv->FBuf() );
      if( pFBuf->IsDirty() && pFBuf->FnmIsDiskWritable() && pFBuf->ViewCount() == 1 ) {
         switch( chGetCmdPromptResponse( "yn", -1, -1, "%s has changed!  Save changes (Y/N)? ", pFBuf->Name() ) ) {
            break;default:  Assert( 0 );           // chGetCmdPromptResponse bug or params out of sync
            break;case 'y': pFBuf->WriteToDisk();  // SAVE
            break;case 'n':                        // NO SAVE
            break;case -1:  ;                      // NO SAVE
            }
         }
      }
   }
   DestroyViewList( &pWinToClose->d_ViewHd );

   Point newSizePct( pWinToMergeTo->SizePct()  );
   Point newSize(    pWinToMergeTo->d_Size     );
   Point newUlc (    pWinToMergeTo->d_UpLeft   );
   #define  WIN_SIZE_MERGE( aaa )                                   \
         newSize   .aaa += pWinToClose->d_Size.aaa + BORDER_WIDTH ; \
         newSizePct.aaa += pWinToClose->SizePct().aaa ;             \
         NoMoreThan( &newUlc.aaa, pWinToClose->d_UpLeft.aaa )  ;
   const auto fSplitVertical( pWinToMergeTo->d_UpLeft.lin == pWinToClose->d_UpLeft.lin );
   if( fSplitVertical ) { WIN_SIZE_MERGE( col ) }
   else                 { WIN_SIZE_MERGE( lin ) }
   #undef  WIN_SIZE_MERGE

   Delete0( pWinToClose ); //----------------------------------------------------------------------
   pWinToMergeTo->Event_Win_Reposition( newUlc ); // before sorting
   pWinToMergeTo->Event_Win_Resized( newSize, newSizePct );
   g__.aWindow.erase( g__.aWindow.begin() + wixToClose );
   if( wixToClose < wixToMergeTo ) {
      --wixToMergeTo;
      }
   SetWindowIdx( wixToMergeTo );
   SortWinArray();
   SetWindowSetValidView( -1 );
   }

STATIC_FXN bool CloseWnd( PWin pWinToClose ) { 0 && DBG( "%s+ %d of %" PR_SIZET, __func__, pWinToClose->d_wnum, g_WindowCount() );
   for( auto pWin : g__.aWindow ) {
      if( pWinToClose != pWin && WindowsCanBeMerged( pWinToClose, pWin ) ) {
         CloseWindow_( pWinToClose, pWin );
         return true;
         }
      }
   return false;  // cannot close
   }

// SetWindowSetValidView is used to prep (ensure validity, i.e. displayability of) the window's current View+FBUF
// note that it is used at editor startup to discard any leading filenames (whether from cmdline or history), as
// well as when switching between windows post-startup
//
void SetWindowSetValidView( int widx ) { enum { SD=0 };
   if( WidxInRange( widx ) ) {
      SetWindowIdx( widx );
      }
   const auto  iw( g_CurWindowIdx() );
   const auto  pWin( g_CurWin() );
         auto &vh( g_CurViewHd() );            SD && DBG( "%s Win[%" PR_SIZET "]", __func__, iw );
   for( auto try_(0); !vh.empty(); ++try_ ) {
      const auto fb( vh.front()->FBuf() );     SD && DBG( "%s try %d=%s", __func__, try_, fb->Name() );
      if( fChangeFile( fb->Name() ) ) {        SD && DBG( "%s try %d successful!", __func__, try_ );  // fb->PutFocusOn() also works
         Assert( g_CurView() != nullptr );
         return;
         }
      }                                        SD && DBG( "%s Win[%" PR_SIZET "] giving up, adding %s", __func__, iw, kszNoFile );
   fChangeFile( kszNoFile );
   Assert( g_CurView() != nullptr );
   }

void SetWindowSetValidView_( PWin pWin ) {
   SetWindowSetValidView( PWinToWidx( pWin ) );
   }

#if MOUSE_SUPPORT

bool SwitchToWinContainingPointOk( const Point &pt ) {
   for( auto it( g__.aWindow.cbegin() ) ; it != g__.aWindow.cend() ; ++it ) {
      const auto pWin( *it );
      if(  pt.lin >= pWin->d_UpLeft.lin && pt.lin < pWin->d_UpLeft.lin + pWin->d_Size.lin
        && pt.col >= pWin->d_UpLeft.col && pt.col < pWin->d_UpLeft.col + pWin->d_Size.col
        ) {
         SetWindowSetValidView( std::distance( g__.aWindow.cbegin(), it ) );
         return true;
         }
      }
   return false;
   }

#endif

//
// Window
//
//     Moves the cursor to the next window. The next window is to the right
//     of or below the active window.
//
//     Returns
//
//     The Window function without an argument always returns True.
//
// Arg Window
//
//     Splits the active window horizontally at the cursor. All windows
//     must be at least five lines high.
//
//     Returns
//
//     True:  Successfully split the window
//     False: Could not split the window
//
// Arg Arg Window
//
//     Splits the active window vertically at the cursor. All windows must
//     be at least 10 columns wide.
//
//     Returns
//
//     True:  Successfully split the window
//     False: Could not split the window
//
// Meta Window
//
//     Closes the active window.
//
//     Returns
//
//     True:  Window closed
//     False: Window not closed; only one window is open
//

bool ARG::window() {
   auto rv( true );
   switch( d_argType ) {
      break;default:      return false;
      break;case NOARG:   if( g_WindowCount() == 1 ) {
                             rv = false;
                             break;
                             }
                          if( d_fMeta ) {
                             if( !CloseWnd( g_CurWinWr() ) ) {
                                return Msg( "Cannot close this window" );
                                }
                             }
                          else {
                             SetWindowSetValidView( (g_CurWindowIdx()+1) % g_WindowCount() );
                             }
      break;case NULLARG: const auto fSplitVertical( d_cArg != 1 );
                          const auto xyParam( fSplitVertical
                             ? d_nullarg.cursor.col - g_CurView()->Origin().col
                             : d_nullarg.cursor.lin - g_CurView()->Origin().lin
                             );
                          rv = nullptr != SplitCurWnd( fSplitVertical, xyParam );
      }
   DispRefreshWholeScreenNow();
   return rv;
   }

void RefreshCheckAllWindowsFBufs() {
   // first collect affected PFBUF's, avoiding dups
   PFBUF pfbufs[ 1+MAX_WINDOWS ] = { nullptr }; // 1+ = room for trailing nullptr end-marker
   for( auto &pWin : g__.aWindow ) {
      if( pWin->CurView() && pWin->CurView()->FBuf() ) { // look ONLY at each window's (one) visible FBUF
         const auto pFBuf( pWin->CurView()->FBuf() );
         // insert, avoiding duplication, into pfbufs[]
         for( auto &pf : pfbufs ) {
            if( pf == nullptr ) {   // not already recorded?
               pf = pFBuf;          // record pFBuf
               break;
               }
            if( pf == pFBuf ) {     // pFBuf already recorded?
               break;               // don't record pFBuf _again_
               }
            }
         }
      }
   // now that all are known, update them all
   auto updates(0);
   for( auto &pf : pfbufs ) {
      if( !pf ) {
         break;
         }                                    0 && DBG( "REFRESH-CHK '%s'", pf->Name() );
      updates += pf->SyncNoWrite();
      }
   DispDoPendingRefreshes(); // BUGBUG leaving this work to the IdleThread can cause a CRASH
   if( updates ) { Msg( "%d file%s updated when you switched back", updates, Add_s( updates ) ); }
// else          { Msg( "no files changed" ); }
   }

void Wins_WriteStateFile( FILE *ofh ) {
   // 20140330 new
   //
   // Since we never recover multiple windows at startup time, but we want to
   // preserve user file-visited state to the max extent possible, we save a
   // single list of View state merged from all windows views, prioritizing View
   // info pertaining to the more recent visits over that of earlier visits.
   //
   // how: walk all Window's View lists (which are de facto sorted in descending
   // d_tmFocusedOn order), in parallel (collectively) from most- to
   // least-recently visited, saving most recently visited View (pertaining to an
   // unsaved FBUF) to state file (and marking the ref'd FBUF as saved).
   //
   // outcome: each historically-visited file is referenced by a single entry in
   // the state file (containing info for the most recent visit)
   //
   // pro: choosing info to save based upon recentness of visit is much better
   //      than the old hacky algo.
   //
   // con: previous-visit (even if from current session) info is discarded.
   //
   // for now, this is an acceptable tradeoff

   // DO!  NOT!  DESTROY/ALTER  THE  VIEW  LIST  IN  THIS  FUNCTION  !!!
   // DO!  NOT!  DESTROY/ALTER  THE  FBUF  LIST  IN  THIS  FUNCTION  !!!
   // just because we are writing the state file does NOT mean we are exiting the editor!

   // merge all active Views of all windows into a single list ordered by d_tmFocusedOn

   { // call NotSavedToStateFile() on all FBUFs (this is our (Statefile write code) private data in FBUF, so OK to modify)
#if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead,dlinkAllFBufs,pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      pFBuf->NotSavedToStateFile();
      }
   }

   enum { SD=0 };
   class {
      PCView  d_pVw;
   public:
      PCView View() const     { return d_pVw; }
      void Init( PCView pVw ) { d_pVw = pVw; }
      STATIC_FXN bool skip_( PCView pv ) {
         if( !pv ) { return false; }  // counterintuitive: nullptr is "valid" in the sense that we don't want to skip from it to the entry following it
         const auto &fbuf( *pv->FBuf() );
         const auto alreadySaved( fbuf.IsSavedToStateFile() );  const auto chAlreadySaved  ( alreadySaved   ? 'S' : '-' );
         const auto notDiskNm( !fbuf.FnmIsDiskWritable() );     const auto chNotOnDisk     ( notDiskNm      ? 'D' : '-' );
         const auto explicitForget( fbuf.ToForgetOnExit() );    const auto chExplicitForget( explicitForget ? 'F' : '-' );
         const auto skip( alreadySaved || notDiskNm || explicitForget );
         SD && DBG( "%c=%c%c%c %s", (skip?'1':'0'), chAlreadySaved, chNotOnDisk, chExplicitForget, fbuf.Name() );
         return skip;
         }
      void Next()             { d_pVw = DLINK_NEXT( d_pVw, d_dlinkViewsOfWindow ); }
      void ToSaveCand()       { while( skip_( d_pVw ) ) { SD && DBG("skipping %s", d_pVw->FBuf()->Name() );
                                   Next();
                                   }
                              }
      } hds[ MAX_WINDOWS ];
   auto hdsMax( 0 );
   for( auto pWin : g__.aWindow ) {
      if( !pWin->d_ViewHd.empty() ) {
         hds[hdsMax++].Init( pWin->d_ViewHd.front() );
         }
      }
   auto iFilesSaved(0);
   while( 1 ) {
      class not_anon {
         int    d_vw4s_ix     = -1;
         time_t d_tmFocusedOn = -1;
      public:
         int  get_idx()  const { return d_vw4s_ix; }
         bool is_empty() const { return d_vw4s_ix == -1; }
         void cmp( int ix, time_t tNew ) {
            if( is_empty() || tNew > d_tmFocusedOn ) {
               d_vw4s_ix     = ix;
               d_tmFocusedOn = tNew;
               }
            }
         };
      not_anon best;
      for( auto ix(0) ; ix < hdsMax; ++ix ) {
         hds[ix].ToSaveCand();
         if( hds[ix].View() ) {          SD && DBG("hds[%d] %8" PR_TIMET " %s", ix, hds[ix].View()->TmFocusedOn(), hds[ix].View()->FBuf()->Name() );
            best.cmp( ix, hds[ix].View()->TmFocusedOn() );
            }
         }
      if( best.is_empty() ) { break; /*################################################*/ }
      // we have an entry to write to the statefile
      auto &hd( hds[ best.get_idx() ] );
      const auto pv( hd.View() );        SD && DBG("hds[%d] %8" PR_TIMET " %s  *** SAVING ***", best.get_idx(), pv->TmFocusedOn(), pv->FBuf()->Name() );
      pv->Write( ofh );  ++iFilesSaved;
      pv->FBuf()->SetSavedToStateFile(); // prevent saving state for this file again
      hd.Next();
      }
   if( iFilesSaved == 0 ) {
      fprintf( ofh, " %s|0 0 0 0\n", kszNoFile );
      }
   fprintf( ofh, ".\n" ); // EoF marker (just so if you look at the file you can confirm that this process completed)
   }
