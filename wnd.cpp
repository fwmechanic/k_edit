//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

GLOBAL_VAR TGlobalStructs g__;

GLOBAL_VAR PFBUF s_curFBuf;       // not literally static (s_), but s/b treated as such!

enum { BORDER_WIDTH = 1, };

STIL bool CanCreateWin()  { return g__.iWindowCount < ELEMENTS(g__.aWindow); }

#define  AssertPWin( pWin )   Assert( (pWin) >= g__.aWindow && (pWin) < g__.aWindow[ g__.iWindowCount ] )

bool Win::GetCursorForDisplay( Point *pt ) {
   const auto pcv( ViewHd.First() );
   pt->lin = d_UpLeft.lin + (pcv->Cursor().lin - pcv->Origin().lin) + MinDispLine();
   pt->col = d_UpLeft.col + (pcv->Cursor().col - pcv->Origin().col);
   return true;
   }

bool CanResizeContent( int newX, int newY ) {
   if(   1 == g_iWindowCount()
      && newX >= MIN_WIN_HEIGHT
      && newY >= MIN_WIN_WIDTH
     )
      return true;

   auto maxWinsOnAnyLine(0); // max # of wnds on any display line
   {
   const auto yTop(0), yBottom( EditScreenLines() );
   for( auto yLine(yTop) ; yLine < yBottom; ++yLine ) {
      auto winsOnLine(0);
      for( auto iw(0) ; iw < g_iWindowCount() ; ++iw ) {
         if( g_Win( iw )->VisibleOnDisplayLine( yLine ) ) ++winsOnLine;
         }
      maxWinsOnAnyLine = Max( maxWinsOnAnyLine, winsOnLine );
      }
   }
   auto maxWinsOnAnyCol(0); // max # of wnds on any display row
   {
   const auto xLeft(0), xRight( EditScreenCols() );
   for( auto xCol(xLeft) ; xCol < xRight; ++xCol ) {
      auto winsOnCol(0);
      for( auto iw(0) ; iw < g_iWindowCount() ; ++iw ) {
         if( g_Win( iw )->VisibleOnDisplayCol( xCol ) ) ++winsOnCol;
         }
      maxWinsOnAnyCol = Max( maxWinsOnAnyCol, winsOnCol );
      }
   }

   DBG( "%s maxWinsOnAnyLine=%d, maxWinsOnAnyCol=%d", __func__, maxWinsOnAnyLine, maxWinsOnAnyCol );

   if( 0&&newY > MIN_WIN_HEIGHT * maxWinsOnAnyLine
       && newX > MIN_WIN_WIDTH  * maxWinsOnAnyCol
     )
      return true;

/*

*/

   return false;
   }

bool ResizeScreen( int newX, int newY ) {
   // BUGBUG need to support resizing in a multiple window enviromnent:
   //   1: minimum-sizecheck
   //   2: resize

   const auto fVModeSwOk( VideoSwitchModeToXY( newX, newY ) );
   if( !fVModeSwOk )
      Msg( "Not supported by video display" );

   return fVModeSwOk;
   }

void View::Event_Win_Resized( LINE newHeight, COL newWidth ) { 0 && DBG( "%s %s", __func__, d_pFBuf->Name() );
   HiliteAddin_Event_WinResized();
   }

void Win::Event_Win_Resized( LINE newHeight, COL newWidth ) {
   d_Size.col = newWidth;
   d_Size.lin = newHeight;
   auto pv( ViewHd.First() );
   pv->EnsureWinContainsCursor();
   DLINKC_FIRST_TO_LAST( ViewHd, dlinkViewsOfWindow, pv ) {
      pv->Event_Win_Resized( newHeight, newWidth );
      }
   }

STATIC_FXN int PWinToWidx( PWin pWin ) {
   for( auto ix(0) ; ix < g_iWindowCount(); ++ix )
      if( g_Win( ix ) == pWin )
         return ix;

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
   d_UpLeft.col = 0;
   d_UpLeft.lin = 0;
   d_Size.col = EditScreenCols();
   d_Size.lin = EditScreenLines();
   }

Win::Win() {
   Maximize();
   }

Win::Win( Win &parent, bool fSplitVertical, int ColumnOrLineToSplitAt ) { // ! parent is a reference since this is a COPY CTOR
   // CAREFUL HERE!  Order is important because parent.d_Size.lin/col IS MODIFIED _AND USED_ herein!
   //
   parent.DispNeedsRedrawAllLines(); // in the horizontal-split case this is somewhat overkill...

   if( fSplitVertical ) {
             d_Size.lin  = parent.d_Size.lin                                         ;
             d_Size.col  = parent.d_Size.col   - ColumnOrLineToSplitAt - 2           ;
      parent.d_Size.col -=        d_Size.col                           + BORDER_WIDTH; // <-- !!!!
           d_UpLeft.lin  = parent.d_UpLeft.lin                                       ;
           d_UpLeft.col  = parent.d_UpLeft.col + parent.d_Size.col     + BORDER_WIDTH;
      }
   else {
             d_Size.lin  = parent.d_Size.lin   - ColumnOrLineToSplitAt - 2           ;
             d_Size.col  = parent.d_Size.col                                         ;
      parent.d_Size.lin -=        d_Size.lin                           + BORDER_WIDTH; // <-- !!!!
           d_UpLeft.lin  = parent.d_UpLeft.lin + parent.d_Size.lin     + BORDER_WIDTH;
           d_UpLeft.col  = parent.d_UpLeft.col                                       ;
      }

   parent.Event_Win_Resized( parent.d_Size.lin, parent.d_Size.col );

   // clone all of original window's Views in a new list bound to the new window  !BUGBUG a different approach should be created!
   for( auto view( parent.CurView() ) ; view ; view = DLINK_NEXT( view, dlinkViewsOfWindow ) ) {
      auto dupdPF( new View( *view, this ) ); // DO NOT inline the 'new View( ... )' into DLINK_INSERT_LAST _macro_ !!!
      DLINK_INSERT_LAST( ViewHd, dupdPF, dlinkViewsOfWindow );
      }
   }

Win::Win( PCChar pC ) { // Used during ReadStateFile processing ONLY!
   0 && DBG( "RdSF: WIN-GEOM '%s'", pC );
   sscanf( pC, " %d %d %d %d "
      , &d_UpLeft.col
      , &d_UpLeft.lin
      , &d_Size.col
      , &d_Size.lin
      );
   0 && DBG( "RdSF: WIN-GEOM %d %d %d %d", d_UpLeft.col, d_UpLeft.lin, d_Size.col, d_Size.lin );
   }

STATIC_FXN PWin SaveNewWin( PWin newWin ) {
   g__.aWindow[ g__.iWindowCount++ ] = newWin;
   return newWin;
   }

STATIC_FXN void SetWindowIdx( int widx ) {
   Assert( widx >= 0 && widx < g_iWindowCount() );
   g__.ixCurrentWin = widx;
   }

void InitNewWin( PCChar pC ) { // Used during ReadStateFile processing only!
   if( CanCreateWin() ) {
      SaveNewWin( new Win(pC) );
      SetWindowIdx( g__.iWindowCount-1 );
      }
   }

void CreateWindow0() { // Used before ReadStateFile processing only!
   SaveNewWin( new Win );
   }

void SetWindow0() { // Used during ReadStateFile processing only!
   switch( g_iWindowCount() ) {
   // case 0: SaveNewWin( new Win )      ; break; // state file had NO windows?
      case 1: g__.aWindow[0]->Maximize() ; break; // state file had one window, but its size may not equal the size of the console
      }
   SetWindowIdx( 0 );
   }

int cmp_win( PCWin w1, PCWin w2 ) {
   if( w1->d_UpLeft.lin < w2->d_UpLeft.lin )   return -1; // w1 < w2
   if( w1->d_UpLeft.lin > w2->d_UpLeft.lin )   return  1; // w1 > w2
   if( w1->d_UpLeft.col < w2->d_UpLeft.col )   return -1; // w1 < w2
   if( w1->d_UpLeft.col > w2->d_UpLeft.col )   return  1; // w1 > w2
                                               return  0; // w1 = w2 ?
   }

STATIC_FXN int CDECL__ qsort_cmp_win( PCVoid p1, PCVoid p2 ) {
   return cmp_win( *PPCWin( p1 ), *PPCWin( p2 ) );
   }

STATIC_FXN void SortWinArray() {
   const auto tmpCurWin( g_CurWin() );  // needed to update g_CurWin()
   qsort( g__.aWindow, g_iWindowCount(), sizeof(g__.aWindow[0]), qsort_cmp_win );
   SetWindowIdx( PWinToWidx( tmpCurWin ) );  // update g_CurWin()
   }

PWin SplitCurWnd( bool fSplitVertical, int ColumnOrLineToSplitAt ) {
   if( !CanCreateWin() ) {
      Msg( "Too many windows" );
      return nullptr;
      }

   if( g_iWindowCount() > 1 ) {
      const auto existingSplitVertical( g__.aWindow[ 0 ]->d_UpLeft.lin == g__.aWindow[ 1 ]->d_UpLeft.lin );
      if( existingSplitVertical != fSplitVertical ) {
         Msg( "cannot split %s when previous splits %s", fSplitVertical?"Vertical":"Horizontal", existingSplitVertical?"Vertical":"Horizontal" );
         return nullptr;
         }
      }

   const auto pWin( g_CurWin() );
   if(   ( fSplitVertical && (ColumnOrLineToSplitAt < MIN_WIN_WIDTH  || pWin->d_Size.col - ColumnOrLineToSplitAt < MIN_WIN_WIDTH ))
      || (!fSplitVertical && (ColumnOrLineToSplitAt < MIN_WIN_HEIGHT || pWin->d_Size.lin - ColumnOrLineToSplitAt < MIN_WIN_HEIGHT))
     ) {
      Msg( "Window too small to split" );
      return nullptr;
      }

// const Point svCursLocn( g_CurView()->Cursor()     ); // unnecessary?  No, this does something useful ...
   const auto  svUlcLine ( g_CurView()->Origin().lin ); // ... (ulc of orig window tends to scroll without it)

   // top of new window is at cursor's line
   if( !fSplitVertical ) {
      g_CurView()->PokeOriginLine_HACK( g_CursorLine() ? g_CursorLine() - 1 : g_CursorLine() );
      }
   0 && DBG( "%s+ from w%d of %d", __func__, g_CurWindowIdx(), g_iWindowCount() );

   auto newWin( SaveNewWin( new Win( *pWin, fSplitVertical, ColumnOrLineToSplitAt ) ) );
   SortWinArray();

   if( !fSplitVertical )
      g_CurView()->PokeOriginLine_HACK( svUlcLine );
// g_CurView()->MoveCursor( svCursLocn );

   newWin->CurView()->HiliteAddins_Init();  // here since not called by SetWindowSetValidView
   return newWin;
   }

STATIC_FXN bool WindowsCanBeMerged( int winDex1, int winDex2 ) {
   CPCWin pw1( g_Win(winDex1) ), pw2( g_Win(winDex2) );

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

STATIC_FXN void CloseWindow_( int winToClose, int wixToMergeTo ) { 0 && DBG( "%s merge %d to %d of %d", __func__, winToClose, wixToMergeTo, g_iWindowCount() );
   const auto pWinToMergeTo( g_Win( wixToMergeTo ) );
   {
   auto pWinToClose( g_Win( winToClose ) );

   {
   DLINKC_FIRST_TO_LASTA( pWinToClose->ViewHd, dlinkViewsOfWindow, pv ) {
      const auto pFBuf( pv->FBuf() );
      if( pFBuf->IsDirty() && pFBuf->FnmIsDiskWritable() && pFBuf->ViewCount() == 1 ) {
         switch( chGetCmdPromptResponse( "yn", -1, -1, "%s has changed!  Save changes (Y/N)? ", pFBuf->Name() ) ) {
            default:   Assert( 0 );            // chGetCmdPromptResponse bug or params out of sync
            case 'y':  pFBuf->WriteToDisk();   break; // SAVE
            case 'n':                          break; // NO SAVE
            case -1:                           break; // NO SAVE
            }
         }
      }
   }

   DestroyViewList( &pWinToClose->ViewHd );

   if( pWinToMergeTo->d_UpLeft.lin == pWinToClose->d_UpLeft.lin )  pWinToMergeTo->d_Size.col += pWinToClose->d_Size.col + BORDER_WIDTH;
   else                                                            pWinToMergeTo->d_Size.lin += pWinToClose->d_Size.lin + BORDER_WIDTH;

   NoGreaterThan( &pWinToMergeTo->d_UpLeft.col, pWinToClose->d_UpLeft.col );
   NoGreaterThan( &pWinToMergeTo->d_UpLeft.lin, pWinToClose->d_UpLeft.lin );
   Delete0( pWinToClose );
   }

   for( auto ix( winToClose+1 ); ix < g_iWindowCount(); ++ix )
      g__.aWindow[ ix-1 ] = g__.aWindow[ ix ];  // move "the bubble" to the end

   g__.aWindow[ --g__.iWindowCount ] = nullptr; // drop "the bubble"

   pWinToMergeTo->Event_Win_Resized( pWinToMergeTo->d_Size.lin, pWinToMergeTo->d_Size.col );

   if( winToClose < wixToMergeTo )
      --wixToMergeTo;

   SetWindowIdx( wixToMergeTo );
   SortWinArray();
   SetWindowSetValidView( -1 );
   }

STATIC_FXN bool CloseWnd( int winToClose ) { 0 && DBG( "%s+ %d of %d", __func__, winToClose, g_iWindowCount() );
   for( auto ix(0) ; ix < g_iWindowCount() ; ++ix )
      if( WindowsCanBeMerged( winToClose, ix ) ) {
         CloseWindow_( winToClose, ix );
         return true;
         }
   return false;  // cannot close
   }


// SetWindowSetValidView is used to prep (ensure validity, i.e. displayability) of the window's current View+FBUF
// note that it is used at editor startup to discard any leading filenames (whether from cmdline or history), as
// well as when switching between windows post-startup
//
void SetWindowSetValidView( int widx ) { enum { DD=0 };
   if( widx >= 0 ) {
      SetWindowIdx( widx );
      }

   const auto pWin( g_CurWin() );
   const auto iw( g_CurWindowIdx() );
   ViewHead &vh( g_CurViewHd() );
   DD && DBG( "%s Win[%d]", __func__, iw );
   for( auto try_(0); !vh.IsEmpty(); ++try_ ) {
      const auto fb( vh.First()->FBuf() );     DD && DBG( "%s try %d=%s", __func__, try_, fb->Name() );
      if( fChangeFile( fb->Name() ) ) {  // fb->PutFocusOn() also works
         DD && DBG( "%s try %d successful!", __func__, try_ );
         Assert( g_CurView() != nullptr );
         return;
         }
      }

   DD && DBG( "%s Win[%d] giving up, adding %s", __func__, iw, szNoFile );
   fChangeFile( szNoFile );
   Assert( g_CurView() != nullptr );
   }

void SetWindowSetValidView_( PWin pWin ) {
   SetWindowSetValidView( PWinToWidx( pWin ) );
   }

#if MOUSE_SUPPORT

bool SwitchToWinContainingPointOk( const Point &pt ) {
   for( auto ix(0) ; ix < g_iWindowCount(); ++ix ) {
      const auto pWin( g_Win( ix ) );
      if(  pt.lin >= pWin->d_UpLeft.lin && pt.lin < pWin->d_UpLeft.lin + pWin->d_Size.lin
        && pt.col >= pWin->d_UpLeft.col && pt.col < pWin->d_UpLeft.col + pWin->d_Size.col
        ) {
         SetWindowSetValidView( ix );
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
      default:       return false;
      case NOARG:    if( g_iWindowCount() == 1 ) {
                        rv = false;
                        break;
                        }
                     if( d_fMeta ) {
                        if( !CloseWnd( g_CurWindowIdx() ) )
                           return Msg( "Cannot close this window" );
                        }
                     else
                        SetWindowSetValidView( (g_CurWindowIdx()+1) % g_iWindowCount() );
                     break;

      case NULLARG:  const auto fSplitVertical( d_cArg != 1 );
                     const auto xyParam( fSplitVertical
                        ? d_nullarg.cursor.col - g_CurView()->Origin().col
                        : d_nullarg.cursor.lin - g_CurView()->Origin().lin
                        );
                     rv = nullptr != SplitCurWnd( fSplitVertical, xyParam );
                     break;
      }

   DispRefreshWholeScreenNow();
   return rv;
   }

void RefreshCheckAllWindowsFBufs() {
   // first collect affected PFBUF's, avoiding dups
   PFBUF pfbufs[ 1+ELEMENTS(g__.aWindow) ] = { nullptr }; // 1+ = room for trailing nullptr end-marker
   for( auto ix(0) ; ix < g_iWindowCount() ; ++ix ) {
      const auto pWin( g_Win( ix ) );
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
      if( !pf )
         break;
      0 && DBG( "REFRESH-CHK '%s'", pf->Name() );
      updates += pf->SyncNoWrite();
      }

   DispDoPendingRefreshes(); // BUGBUG leaving this work to the IdleThread can cause a CRASH

   if( updates ) { Msg( "%d file%s updated when you switched back", updates, Add_s( updates ) ); }
// else          { Msg( "no files changed" ); }
   }
