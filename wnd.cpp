//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

class Win::impl {
   Point     d_size_pct;
public:
   const Point &SizePct() const { return d_size_pct; }
   void  SizePct_set( const Point &src )  { d_size_pct = src; }
   };

Win::~Win() = default;

GLOBAL_VAR TGlobalStructs g__;

GLOBAL_VAR PFBUF s_curFBuf;       // not literally static (s_), but s/b treated as such!

enum { BORDER_WIDTH =  1 };
enum { MAX_WINDOWS  = 10 };

STIL bool CanCreateWin()  { return g_iWindowCount() < MAX_WINDOWS; }

#define  AssertPWin( pWin )   Assert( (pWin) >= g__.aWindow && (pWin) <= g__.aWindow[ g_iWindowCount()-1 ] )
#define  AssertWidx( widx )   Assert( widx >= 0 && widx < g_iWindowCount() );

bool Win::GetCursorForDisplay( Point *pt ) {
   const auto pcv( ViewHd.First() );
   pt->lin = d_UpLeft.lin + (pcv->Cursor().lin - pcv->Origin().lin) + MinDispLine();
   pt->col = d_UpLeft.col + (pcv->Cursor().col - pcv->Origin().col);
   return true;
   }

void View::Event_Win_Resized( const Point &newSize ) { 0 && DBG( "%s %s", __func__, d_pFBuf->Name() );
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
      if( pimpl->SizePct() != newSizePct ) { 0 && DBG( "%s[%d] pctg(%d%%,%d%%)->(%d%%,%d%%)", __func__, d_wnum,  pimpl->SizePct().lin, pimpl->SizePct().col, newSizePct.lin, newSizePct.col );
          pimpl->SizePct_set( newSizePct );
         }
      auto pv( ViewHd.First() );
      pv->EnsureWinContainsCursor();
      DLINKC_FIRST_TO_LAST( ViewHd, dlinkViewsOfWindow, pv ) {
         pv->Event_Win_Resized( newSize );
         }
      }
   }

void Win::Event_Win_Resized( const Point &newSize ) {
   Event_Win_Resized( newSize, pimpl->SizePct() );
   }

STATIC_FXN int NonWinDisplayLines() { return ScreenLines() - EditScreenLines(); }
STATIC_FXN int NonWinDisplayCols()  { return 0; }

bool Wins_CanResizeContent( const Point &newSize ) {
   const auto existingSplitVertical( g_iWindowCount() > 1 && g_Win(0)->d_UpLeft.lin == g_Win(1)->d_UpLeft.lin );
   auto min_size_x( NonWinDisplayCols() ); auto min_size_y( NonWinDisplayLines() );
   if( existingSplitVertical ) {
      min_size_x += (g_iWindowCount() * MIN_WIN_WIDTH)  + (BORDER_WIDTH * (g_iWindowCount() - 1));
      min_size_y += MIN_WIN_HEIGHT;
      }
   else {
      min_size_x += MIN_WIN_WIDTH;
      min_size_y += (g_iWindowCount() * MIN_WIN_HEIGHT) + (BORDER_WIDTH * (g_iWindowCount() - 1));
      }
   const auto rv(// g_iWindowCount() == 1 &&
                    newSize.col >= min_size_x
                 && newSize.lin >= min_size_y
                );
   0 && DBG( "can%s resize", rv?"":"not" );
   return rv;
   }

void Wins_ScreenSizeChanged( const Point &newSize ) {
   const Point newWinRgnSize( newSize, -NonWinDisplayLines(), -NonWinDisplayCols() );
   if( g_iWindowCount() == 1 ) {
      g_CurWin()->Event_Win_Resized( newWinRgnSize );
      }
   else { // multiwindow resize
      if( 1 ) {
         #define MW_RESIZE( aaa, bbb )                                                                                      \
            Point newNonBorderSize( newWinRgnSize );                                                                        \
                  newNonBorderSize.aaa -= BORDER_WIDTH*(g_iWindowCount()-1);                                                \
            auto curNonBorderSize( 0 ); /* excluding borders */                                                             \
            for( const auto &pWin : g__.aWindow ) {                                                                         \
               curNonBorderSize += pWin->d_Size.aaa;                                                                        \
               }                                                                                                            \
            if( newNonBorderSize.aaa != curNonBorderSize ) { /* grow/shrink all windows proportional to their d_size_pct */ \
               0 && DBG( "%s %s:%d->%d", __func__, #aaa, curNonBorderSize, newNonBorderSize.aaa );                          \
               auto ulc( newNonBorderSize.aaa + (BORDER_WIDTH*(g_iWindowCount()-1)) );                                      \
               for( auto it( g__.aWindow.rbegin() ); it != g__.aWindow.rend(); ++it ) {                                     \
                  const auto pW( *it );                                                                                     \
                  const auto size_( pW->d_Size.aaa );                                                                       \
                  int newSize_( (newNonBorderSize.aaa * static_cast<double>(pW->pimpl->SizePct().aaa)) / 100 );             \
                  if( newNonBorderSize.aaa > curNonBorderSize ) {                                                           \
                     NoLessThan( &newSize_, size_ );                                                                        \
                     }                                                                                                      \
                  const auto delta( ulc - newSize_ );                                                                       \
                  const auto iw( std::distance( g__.aWindow.begin(), it.base()) -1 );                                       \
                  0 && DBG( "Win[%Id] size_.%s %d->%d delta=%d", iw, #aaa, size_, newSize_, delta );                        \
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
               }                                                                                                            \

         const auto existingSplitVertical( g_iWindowCount() > 1 && g_Win(0)->d_UpLeft.lin == g_Win(1)->d_UpLeft.lin );
         if( existingSplitVertical ) { MW_RESIZE( col, lin ) } // splitVert
         else                        { MW_RESIZE( lin, col ) } // splitHoriz

         #undef MW_RESIZE

/*
         auto min_size_x( NonWinDisplayCols() ); auto min_size_y( NonWinDisplayLines() );
         if( 0 ) {
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
            if( 0&&newSize.lin > MIN_WIN_HEIGHT * maxWinsOnAnyLine
                && newSize.col > MIN_WIN_WIDTH  * maxWinsOnAnyCol
              )
               return true;
            }
  */
         }
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
   d_wnum = 0;
   d_UpLeft.col = 0;
   d_UpLeft.lin = 0;
   d_Size.col = EditScreenCols();
   d_Size.lin = EditScreenLines();
   { Point tmp{ .lin=100, .col=100 }; pimpl->SizePct_set( tmp ); }
   DBG( "%s", __func__ );
   }

Win::Win()
   : pimpl{ new impl{} }
   {
   Maximize();
   }

Win::Win( Win &parent_, bool fSplitVertical, int ColumnOrLineToSplitAt )
   : pimpl{ new impl{} }
   { // ! parent_ is a reference since this is a COPY CTOR
   parent_.DispNeedsRedrawAllLines(); // in the horizontal-split case this is somewhat overkill...

   // CAREFUL HERE!  Order is important because parent.d_Size.lin/col IS MODIFIED _AND USED_ herein!
   const auto &parent( parent_ ); // parent_ SHALL NOT be modified until this dims have been set
   Point newParentSize, newParentSizePct;
   #define SPLIT_IT( aaa, bbb )                                                       \
        this->d_Size.aaa = parent.d_Size.aaa                                        ; \
        this->d_Size.bbb = parent.d_Size.bbb - ColumnOrLineToSplitAt - 2            ; \
       newParentSize.aaa = parent.d_Size.aaa                                        ; \
       newParentSize.bbb = parent.d_Size.bbb -   (this->d_Size.bbb  + BORDER_WIDTH) ; \
      this->d_UpLeft.aaa = parent.d_UpLeft.aaa                                      ; \
      this->d_UpLeft.bbb = parent.d_UpLeft.bbb + (newParentSize.bbb + BORDER_WIDTH) ; \
      { Point tmp                                                                   ; \
        tmp.aaa = 100 /* _ASSUMING_ uniform split-type */                           ; \
        tmp.bbb = (parent.pimpl->SizePct().bbb * this->d_Size.bbb)                    \
                / (parent.d_Size.bbb - BORDER_WIDTH)                                ; \
        this->pimpl->SizePct_set( tmp );                                              \
      }                                                                               \
      newParentSizePct.aaa = parent.pimpl->SizePct().aaa                            ; \
      newParentSizePct.bbb = parent.pimpl->SizePct().bbb                              \
                           -  this->pimpl->SizePct().bbb                            ; \
      1 && DBG( "%s: src=%d=%d%%->%d=%d%%, new=%d=%d%%", __func__,                    \
                parent.d_Size.bbb, parent.pimpl->SizePct().bbb                        \
              , newParentSize.bbb,        newParentSizePct.bbb                        \
              ,  this->d_Size.bbb,  this->pimpl->SizePct().bbb                        \
              );                                                                      \

   if( fSplitVertical ) {  SPLIT_IT( lin, col )  }
   else                 {  SPLIT_IT( col, lin )  }

   #undef SPLIT_IT

   parent_.Event_Win_Resized( newParentSize, newParentSizePct ); // feed newParentSize back into parent

   // clone all of original window's Views in a new list bound to the new window  !BUGBUG a different approach should be created!
   for( auto view( parent_.CurView() ) ; view ; view = DLINK_NEXT( view, dlinkViewsOfWindow ) ) {
      auto dupdPF( new View( *view, this ) ); // DO NOT inline the 'new View( ... )' into DLINK_INSERT_LAST _macro_ !!!
      DLINK_INSERT_LAST( ViewHd, dupdPF, dlinkViewsOfWindow );
      }
   }

Win::Win( PCChar pC ) // Used during ReadStateFile processing ONLY!
   : pimpl{ new impl{} }
   {
   d_wnum = 0;
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
      SetWindowIdx( g_iWindowCount()-1 );
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

int cmp_win( PCWin w1, PCWin w2 ) { // used by Lua: l_register_Win_object
   if( w1->d_UpLeft.lin < w2->d_UpLeft.lin )   return -1; // w1 < w2
   if( w1->d_UpLeft.lin > w2->d_UpLeft.lin )   return  1; // w1 > w2
   if( w1->d_UpLeft.col < w2->d_UpLeft.col )   return -1; // w1 < w2
   if( w1->d_UpLeft.col > w2->d_UpLeft.col )   return  1; // w1 > w2
                                               return  0; // w1 = w2 ?
   }

STATIC_FXN void SortWinArray() {
   const auto tmpCurWin( g_CurWin() );  // needed to update g_CurWin()
   std::sort( g__.aWindow.begin(), g__.aWindow.end(), []( PCWin w1, PCWin w2 ) { return w1->d_UpLeft < w2->d_UpLeft; } );
   { int iw(0); for( const auto &win : g__.aWindow ) { win->d_wnum = iw++; } }
   SetWindowIdx( PWinToWidx( tmpCurWin ) );  // update g_CurWin()
   }

PWin SplitCurWnd( bool fSplitVertical, int ColumnOrLineToSplitAt ) {
   if( !CanCreateWin() ) {
      Msg( "Too many windows" );
      return nullptr;
      }

   if( g_iWindowCount() > 1 ) {
      const auto existingSplitVertical( g_Win(0)->d_UpLeft.lin == g_Win(1)->d_UpLeft.lin );
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

STATIC_FXN void CloseWindow_( int winToClose, int wixToMergeTo ) { 1 && DBG( "%s merge %d to %d of %d", __func__, winToClose, wixToMergeTo, g_iWindowCount() );
   const auto pWinToMergeTo( g_Win( wixToMergeTo ) );
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

   Point newSizePct( pWinToMergeTo->pimpl->SizePct() );
   Point newSize(    pWinToMergeTo->d_Size     );
   Point newUlc (    pWinToMergeTo->d_UpLeft   );
   #define  WIN_SIZE_MERGE( aaa )                                   \
         newSize   .aaa += pWinToClose->d_Size.aaa + BORDER_WIDTH ; \
         newSizePct.aaa += pWinToClose->pimpl->SizePct()  .aaa    ; \
         NoGreaterThan( &newUlc.aaa, pWinToClose->d_UpLeft.aaa )  ; \

   const auto fSplitVertical( pWinToMergeTo->d_UpLeft.lin == pWinToClose->d_UpLeft.lin );
   if( fSplitVertical ) { WIN_SIZE_MERGE( col ) }
   else                 { WIN_SIZE_MERGE( lin ) }

   #undef  WIN_SIZE_MERGE

   Delete0( pWinToClose ); //----------------------------------------------------------------------

   pWinToMergeTo->Event_Win_Reposition( newUlc ); // before sorting
   pWinToMergeTo->Event_Win_Resized( newSize, newSizePct );

   g__.aWindow.erase( g__.aWindow.begin() + winToClose );
   if( winToClose < wixToMergeTo )
      --wixToMergeTo;

   SetWindowIdx( wixToMergeTo );
   SortWinArray();
   SetWindowSetValidView( -1 );
   }

STATIC_FXN bool CloseWnd( int winToClose ) { 0 && DBG( "%s+ %d of %d", __func__, winToClose, g_iWindowCount() );
   for( auto ix(0) ; ix < g_iWindowCount() ; ++ix )
      if( winToClose != ix && WindowsCanBeMerged( winToClose, ix ) ) {
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

   const auto  pWin( g_CurWin() );
   const auto  iw( g_CurWindowIdx() );
         auto &vh( g_CurViewHd() );
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
      if( !pf )
         break;
      0 && DBG( "REFRESH-CHK '%s'", pf->Name() );
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
   PRbNode pNd;
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

   enum { DV=0 };
   class {
      PCView  d_pVw;
   public:
      PCView View() const     { return d_pVw; }
      void Init( PCView pVw ) { d_pVw = pVw; }
      STATIC_FXN bool skip_( PCView pv ) {
         if( !pv ) return false;   // counterintuitive: nullptr is "valid" in the sense that we don't want to skip from it to the entry following it
         const auto &fbuf( *pv->FBuf() );
         const auto alreadySaved( fbuf.IsSavedToStateFile() );  const auto chAlreadySaved  ( alreadySaved   ? 'S' : '-' );
         const auto notDiskNm( !fbuf.FnmIsDiskWritable() );     const auto chNotOnDisk     ( notDiskNm      ? 'D' : '-' );
         const auto explicitForget( fbuf.ToForgetOnExit() );    const auto chExplicitForget( explicitForget ? 'F' : '-' );
         const auto skip( alreadySaved || notDiskNm || explicitForget );
         DV && DBG( "%c=%c%c%c %s", (skip?'1':'0'), chAlreadySaved, chNotOnDisk, chExplicitForget, fbuf.Name() );
         return skip;
         }
      void Next()             { d_pVw = DLINK_NEXT( d_pVw, dlinkViewsOfWindow ); }
      void ToSaveCand()       { while( skip_( d_pVw ) ) { DV && DBG("skipping %s", d_pVw->FBuf()->Name() );
                                   Next();
                                   }
                              }
      } hds[ MAX_WINDOWS ];
   auto hdsMax( 0 );
   for( auto pWin : g__.aWindow ) {
      if( !pWin->ViewHd.IsEmpty() ) {
         hds[hdsMax++].Init( pWin->ViewHd.First() );
         }
      }

   auto iFilesSaved(0);
   while( 1 ) {
      class not_anon {
         int    d_vw4s_ix    ;
         time_t d_tmFocusedOn;
      public:
         not_anon()
           : d_vw4s_ix     (-1)
           , d_tmFocusedOn (-1)
           {}
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
         if( hds[ix].View() ) { DV && DBG("hds[%d] %8Id %s", ix, hds[ix].View()->TmFocusedOn(), hds[ix].View()->FBuf()->Name() );
            best.cmp( ix, hds[ix].View()->TmFocusedOn() );
            }
         }
      if( best.is_empty() ) { break; /*################################################*/ }

      // we have an entry to write to the statefile
      auto &hd( hds[ best.get_idx() ] );
      const auto pv( hd.View() );
      DV && DBG("hds[%d] %8Id %s  *** SAVING ***", best.get_idx(), pv->TmFocusedOn(), pv->FBuf()->Name() );
      pv->Write( ofh );  ++iFilesSaved;
      pv->FBuf()->SetSavedToStateFile(); // prevent saving state for this file again
      hd.Next();
      }

   if( iFilesSaved == 0 ) {
      fprintf( ofh, " %s|0 0 0 0\n", szNoFile );
      }
   fprintf( ofh, ".\n" ); // EoF marker (just so if you look at the file you can confirm that this process completed)
   }
