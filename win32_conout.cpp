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
#include "win32_pvt.h"

//--------------------------------------------------------------------------------------------

// see for good explanations:
// http://www.adrianxw.dk/SoftwareSite/Consoles/Consoles5.html

//  BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT
//  BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT
//  BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT
//  BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT

typedef Win32::CHAR_INFO ScreenCell; // this is copied from our cache by Win32::WriteConsoleOutputA

struct W32_ScreenSize_CursorLocn {
   YX_t size;
   YX_t cursor;
   };

class TConsoleOutputControl {
   class TConsoleOutputCacheLineInfo {
      COL         d_xMinDirty;
      COL         d_xMaxDirty;
      ScreenCell *d_paCHAR_INFO_BufStart;
   public:
      bool fLineDirty() const { return d_xMinDirty <= d_xMaxDirty; }
      void ColUpdated( int col ) {
         d_xMinDirty = std::min( d_xMinDirty, col );
         d_xMaxDirty = std::max( d_xMaxDirty, col );
         }
      void BoundValidColumns( COL *minColUpdated, COL *maxColUpdated ) const {
         *minColUpdated = std::min( *minColUpdated, d_xMinDirty );
         *maxColUpdated = std::max( *maxColUpdated, d_xMaxDirty );
         }
      ScreenCell *BufPtrOfCol( int col ) const {
         return d_paCHAR_INFO_BufStart + col;
         }
      void Undirty() {
         d_xMinDirty = INT_MAX;
         d_xMaxDirty = -1;
         }
      void Init( ScreenCell *pChIB, ScreenCell *pChIB_pastend ) {
         Undirty();
         d_paCHAR_INFO_BufStart = pChIB;
         for( ; pChIB < pChIB_pastend; ++pChIB ) {
            pChIB->Attributes     = 0;  // attribute and char of actual data WILL NOT BE 0 : 0,
            pChIB->Char.AsciiChar = '\0';  // ensuring initial actual data write will be "dirtying"
            }
         }
      };
   const Win32::HANDLE d_hConsoleScreenBuffer;
   std::vector<TConsoleOutputCacheLineInfo> d_vLineControl; // one per line
   bool              d_fCursorVisible;
   bool              d_fBigCursor;
   std::vector<ScreenCell> d_vOutputBufferCache;
   struct {
      int first;
      int last;
      }              d_LineToUpdt;
   std::mutex        d_mutex;
   W32_ScreenSize_CursorLocn d_xyState; // height of CSB-mapped window
public: //**************************************************
   TConsoleOutputControl( int yHeight, int xWidth );
   bool WriteToFileOk( FILE *ofh ); // debug/test facility
   Win32::HANDLE GetConsoleScreenBufferHandle() const { return d_hConsoleScreenBuffer; }
   void NullifyUpdtLineRange() {
      d_LineToUpdt.first = d_xyState.size.lin;
      d_LineToUpdt.last  = 0;
      }
   void  GetSizeCursorLocn( W32_ScreenSize_CursorLocn *cxy ); // not const cuz hits mutex!
   YX_t  GetMaxConsoleSize();                                 // not const cuz hits mutex!
   bool  GetCursorState( YX_t *pt, bool *pfVisible );        // not const cuz hits mutex!
   bool  SetConsoleSizeOk( YX_t &newSize );
   void  SetCursorLocn( LINE yLine, COL xCol );
   void  SetCursorSize( bool fBigCursor );
   bool  SetCursorVisibilityChanged( bool fVisible );
   COL   WriteLineSegToConsoleBuffer( PCChar pszStringToDisp, COL StringLen, LINE yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs );
   void  FlushConsoleBufferToScreen();
   void  ScrollConsole( LINE ulc_yLine, COL ulc_xCol, LINE lrc_yLine, COL lrc_xCol, LINE deltaLine );
   bool  SetConsolePalette( const unsigned palette[16] );
private://**************************************************
   void  SetNewScreenSize( const YX_t &newSize );
   int   WriteConsoleOutput_wrap( LINE yMin, LINE yMax, COL xMin, COL xMax );
   void  SetConsoleCursorInfo();
   };

STATIC_VAR TConsoleOutputControl *s_EditorScreen;

void VidInitApiError( PCChar errmsg ) {
   ConIO::DbgPopf(         "%s", errmsg );
   DBG(             "%s", errmsg );
   fprintf( stderr, "%s", errmsg );
   }

class ConsoleScreenBufferInfo {
   Win32::HANDLE                     d_hCSB;
   bool                              d_isValid;
   Win32::CONSOLE_SCREEN_BUFFER_INFO d_csbi;
   Win32::COORD                      d_maxSize;
public:
   ConsoleScreenBufferInfo( Win32::HANDLE hCSB, PCChar name=nullptr, bool fFailQuietly=false );
   bool isValid() const { return d_isValid; }
   const Win32::SMALL_RECT & srWindow() const { return d_csbi.srWindow; }
   YX_t  WindowSize()     const { return YX_t( d_csbi.srWindow.Bottom - d_csbi.srWindow.Top  + 1
                                             , d_csbi.srWindow.Right  - d_csbi.srWindow.Left + 1
                                             );
                                }
   YX_t  BufferSize()     const { return YX_t( d_csbi.dwSize          .Y  , d_csbi.dwSize          .X   ); }
// YX_t  MaxBufSize()     const { return YX_t( d_maxSize              .Y-1, d_maxSize              .X-1 ); } // -1 because the MAX is sometimes too big (goes off screen) because of window border thickness
   YX_t  MaxBufSize()     const { return YX_t( d_maxSize              .Y  , d_maxSize              .X   ); }
   YX_t  CursorPosition() const { return YX_t( d_csbi.dwCursorPosition.Y  , d_csbi.dwCursorPosition.X   ); }
   int   Attribute()      const { return d_csbi.wAttributes; }
   };

ConsoleScreenBufferInfo::ConsoleScreenBufferInfo( Win32::HANDLE hCSB, PCChar name, bool fFailQuietly )
   : d_hCSB( hCSB )
   { // NB: Win32::GetLargestConsoleWindowSize RESULT can CHANGE as console font is changed (via window-properties dialogs) as we run.
   d_maxSize = Win32::GetLargestConsoleWindowSize( d_hCSB );
   //  /----------------------------------\ <-- documented GetLargestConsoleWindowSize API failure indicator
   if( d_maxSize.X == 0 || d_maxSize.Y == 0 || !GetConsoleScreenBufferInfo( d_hCSB, &d_csbi ) ) {
      d_isValid = false;
      if( name && !fFailQuietly ) {
         linebuf oseb; VidInitApiError( FmtStr<120>( "Win32::GetConsoleScreenBufferInfo on %s FAILED: %s", name, OsErrStr( BSOB(oseb) ) ) );
         }
      }
   else {
      d_isValid = true;
      if( 0 ) {
         // NOTE that sadly we CANNOT create/expand console windows beyond GetLargestConsoleWindowSize
         // (we _sometimes_ desire to do this when running on a homogenous multi-monitor rig)
         // because SetConsoleWindowInfo fails.  This is apparently an OS limitation, and I get NO hits
         // on the web that indicate this is even considered a problem, much less offer a workaround.
         //
         // 20100324 kgoodwin
         const auto smcxvs = Win32::GetSystemMetrics(SM_CXVIRTUALSCREEN); const auto smcxs = Win32::GetSystemMetrics(SM_CXSCREEN);
         const auto smcyvs = Win32::GetSystemMetrics(SM_CYVIRTUALSCREEN); const auto smcys = Win32::GetSystemMetrics(SM_CYSCREEN);
         const auto ratX = static_cast<double>(smcxvs) / smcxs;
         const auto ratY = static_cast<double>(smcyvs) / smcys;
         d_maxSize.X *= ratX;  DBG( "smcxvs=%d, smcxs=%d, ratX=%5.3f, d_maxSize.X=%d", smcxvs, smcxs, ratX, d_maxSize.X );
         d_maxSize.Y *= ratY;  DBG( "smcyvs=%d, smcys=%d, ratY=%5.3f, d_maxSize.Y=%d", smcxvs, smcxs, ratX, d_maxSize.Y );
         }
      if( name ) {
         const auto winSize( WindowSize() );
         const auto bufSize( BufferSize() );
         0 && DBG( "Win32::ConsoleScreenBufferInfo %s Sizes: Win(%dx%d) Buf(%dx%d) Max(%dx%d)", name
            , winSize.col , winSize.lin
            , bufSize.col , bufSize.lin
            , d_maxSize.X , d_maxSize.Y
            );
         }
      }
   }

// Given a ConsoleScreenBuffer handle, create a COC and init it with/using the
// dimensions, dflt colors, etc. of that ConsoleScreenBuffer
//
STATIC_FXN Win32::HANDLE NewConsoleScreenBuffer( PCChar tag ) {
   0 && DBG( "%s+ from %s", __func__, tag );
   const auto hcsb( Win32::CreateConsoleScreenBuffer( (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ|FILE_SHARE_WRITE), nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr ) );
   if( hcsb == Win32::Invalid_Handle_Value() ) {
      linebuf oseb; VidInitApiError( FmtStr<120>( "Win32::CreateConsoleScreenBuffer FAILED: 0x%08lX=%s", Win32::GetLastError(), OsErrStr( BSOB(oseb) ) ) );
      exit( 1 );
      }
   0 && DBG( "%s- from %s", __func__, tag );
   return hcsb;
   }

TConsoleOutputControl::TConsoleOutputControl( int yHeight, int xWidth )
   : d_hConsoleScreenBuffer         ( NewConsoleScreenBuffer( "#1" ) )
   , d_fCursorVisible               ( true )
   {
   0 && DBG( "%s", __func__ );
   // associate d_hConsoleScreenBuffer w/"the window" FIRST so that csbi, which
   // is derived from d_hConsoleScreenBuffer, will return correct window dims
   if( Win32::SetConsoleActiveScreenBuffer( d_hConsoleScreenBuffer ) == 0 ) {
      linebuf oseb; VidInitApiError( FmtStr<120>( "Win32::SetConsoleActiveScreenBuffer FAILED: %s", OsErrStr( BSOB(oseb) ) ) );
      exit( 1 );
      }
   YX_t newSize; newSize.lin=yHeight; newSize.col=xWidth;
   SetConsoleSizeOk( newSize );
   const ConsoleScreenBufferInfo csbi( d_hConsoleScreenBuffer, "new editor console" );
   if( !csbi.isValid() ) {
      exit( 1 ); // error-msgs already displayed, just bail
      }
   d_xyState.cursor = csbi.CursorPosition();
   }

void TConsoleOutputControl::SetNewScreenSize( const YX_t &newSize ) {
   d_xyState.size.lin = newSize.lin; // the ONLY place d_xyState.size is written!
   d_xyState.size.col = newSize.col; // the ONLY place d_xyState.size is written!
   const size_t cells( d_xyState.size.lin * d_xyState.size.col );
   0 && DBG( "+%s (%dx%d) cells=%" PR_SIZET "->%" PR_SIZET " (x %" PR_SIZET " = %" PR_SIZET ")", __func__, d_xyState.size.col, d_xyState.size.lin, d_vOutputBufferCache.size(), cells
         , sizeof(d_vOutputBufferCache[0])
         , sizeof(d_vOutputBufferCache[0]) * cells
         );
   d_vOutputBufferCache.resize( cells              );
   d_vLineControl      .resize( d_xyState.size.lin );
   auto pChIB( &d_vOutputBufferCache[0] );
   for( auto &lc : d_vLineControl ) {
      lc.Init( pChIB, pChIB + d_xyState.size.col );
      pChIB += d_xyState.size.col;
      }
   NullifyUpdtLineRange();
   0 && DBG( "-%s (%dx%d)", __func__, d_xyState.size.col, d_xyState.size.lin );
   }

HANDLE_CTRL_CLOSE_EVENT( volatile bool g_fProcessExitRequested; )

#if defined(__GNUC__) && !defined(__x86_64)
  namespace Win32 {
  #ifdef __cplusplus
    extern "C" {
  #endif
      // NB: these are officially supported Win32 APIs, however last-released nuwen 32-bit mingw gcc does not provide their prototypes:
      BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsoleOutput,BOOL bMaximumWindow,PCONSOLE_FONT_INFO lpConsoleCurrentFont);
      COORD WINAPI GetConsoleFontSize(HANDLE hConsoleOutput,DWORD nFont);
  #ifdef __cplusplus
      }
  #endif
    }
#endif

//--------------------------------------------------------------------------------------------

void TConsoleOutputControl::SetConsoleCursorInfo() {
   Win32::CONSOLE_CURSOR_INFO ConsoleCursorInfo;
   ConsoleCursorInfo.bVisible = d_fCursorVisible ? TRUE : FALSE;
   ConsoleCursorInfo.dwSize = d_fBigCursor ? 100 : 25;
   if( 0 == Win32::SetConsoleCursorInfo( d_hConsoleScreenBuffer, &ConsoleCursorInfo ) ) {
      DBG( "**************** Win32::SetConsoleCursorInfo FAILED *********************" );
      }
   }

void ConOut::SetCursorSize( bool fBigCursor ) {
   if( s_EditorScreen ) {
       s_EditorScreen->SetCursorSize( fBigCursor );
       }
   }

void TConsoleOutputControl::SetCursorSize( bool fBigCursor ) {
   d_fBigCursor = fBigCursor;
   SetConsoleCursorInfo();
   }

bool TConsoleOutputControl::SetCursorVisibilityChanged( bool fVisible ) { enum { SD=0 };
   if( !fVisible &&  d_fCursorVisible ) { SD && DBG( "**************** Making CURSOR INVISIBLE *********************" ); }
   if(  fVisible && !d_fCursorVisible ) { SD && DBG( "**************** Making CURSOR   VISIBLE *********************" ); }
   const auto retVal( d_fCursorVisible != fVisible );
   d_fCursorVisible = fVisible;
   SetConsoleCursorInfo();
   return retVal;
   }

bool TConsoleOutputControl::GetCursorState( YX_t *pt, bool *pfVisible ) {
   std::scoped_lock<std::mutex> mtx( d_mutex );
   *pt        = d_xyState.cursor;
   *pfVisible = d_fCursorVisible;
   return true;
   }

bool ConOut::GetCursorState( YX_t *pt, bool *pfVisible ) {
   YX_t yx;
   return s_EditorScreen ? s_EditorScreen->GetCursorState( pt, pfVisible ) : 0;
   }

bool ConOut::SetCursorVisibilityChanged( bool fVisible ) {
   return s_EditorScreen ? s_EditorScreen->SetCursorVisibilityChanged( fVisible ) : 0;
   }

void ConOut::SetCursorLocn( LINE yLine, COL xCol ) {
   if( s_EditorScreen ) { s_EditorScreen->SetCursorLocn( yLine, xCol ); }
   }

void TConsoleOutputControl::SetCursorLocn( LINE yLine, COL xCol ) {
   const YX_t newPt( yLine, xCol );
   std::scoped_lock<std::mutex> mtx( d_mutex );
   if( d_xyState.cursor != newPt ) {
      Win32::COORD dwCursorPosition;
      dwCursorPosition.Y = yLine;
      dwCursorPosition.X = xCol;
      if( Win32::SetConsoleCursorPosition( d_hConsoleScreenBuffer, dwCursorPosition ) ) {
         0 && DBG( "%lX %s(y=%d,x=%d)", Win32::GetCurrentThreadId(), __func__, yLine, xCol );
         d_xyState.cursor = newPt;
         }
      }
   }

COL ConOut::BufferWriteString( PCChar pszStringToDisp, COL StringLen, LINE yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs ) {
   return s_EditorScreen ? s_EditorScreen->WriteLineSegToConsoleBuffer( pszStringToDisp, StringLen, yLineWithinConsoleWindow, xColWithinConsoleWindow, colorAttribute, fPadWSpcs ) : 0;
   }

COL TConsoleOutputControl::WriteLineSegToConsoleBuffer( PCChar src, COL srcChars, LINE yConsole, int xConsole, const int attr, bool fPadWSpcs ) {
   std::scoped_lock<std::mutex> mtx( d_mutex );
   if(   yConsole  >= d_xyState.size.lin
      || xConsole  >= d_xyState.size.col
      || (srcChars == 0 && !fPadWSpcs)
     ) {
      0 && DBG( "%s BAILS!", __func__ );
      return 0;
      }
   const auto maxConChars( d_xyState.size.col - xConsole );
   srcChars = std::min( srcChars, maxConChars );
   const auto padLen( fPadWSpcs ? maxConChars - srcChars : 0 );
   auto updtdCells(0);
   auto &lc( d_vLineControl[ yConsole ] );
   auto pCI( lc.BufPtrOfCol( xConsole ) );
   auto UpdtCell = [&]( char newCh ) {
      if(   pCI->Char.AsciiChar == newCh
         && pCI->Attributes     == attr
        ) { return; }
      pCI->Char.AsciiChar = newCh;
      pCI->Attributes     = attr;
      lc.ColUpdated( xConsole );
      ++updtdCells;
      };
   for( auto ix(0); ix < srcChars; ++ix, ++pCI, ++xConsole ) {
      UpdtCell( *src++ );
      }
   for( auto ix(0); ix < padLen; ++ix, ++pCI, ++xConsole ) {
      UpdtCell( ' ' ); // pad
      }
   if( updtdCells ) {
      NoMoreThan( &d_LineToUpdt.first, yConsole );
      NoLessThan( &d_LineToUpdt.last , yConsole );
      }
   return srcChars + padLen;
   }

STATIC_FXN bool SetConsoleWindowSizeOk( const Win32::HANDLE d_hConsoleScreenBuffer, LINE yheight, COL xWidth ) {
   enum { SHOWDBG = 0 };
   // *** Either or BOTH of desired width OR height are LESS THAN current values.
   // *** SHRINK Screen Buffer mapping to minimum to meet
   // *** the requirements of the SetConsoleScreenBufferSize API:
   //
   // "The specified width AND height cannot be less than the width and
   // height of the screen buffer's window.  The specified dimensions also
   // cannot be less than the minimum size allowed by the system."
   //
   //lint -e{734}
   Win32::SMALL_RECT newWinMapRect;
   newWinMapRect.Top    = 0;
   newWinMapRect.Left   = 0;
   newWinMapRect.Right  = xWidth  - 1;
   newWinMapRect.Bottom = yheight - 1;
   SHOWDBG && DBG( "%s (%dx%d)", __func__
      , newWinMapRect.Right  +1
      , newWinMapRect.Bottom +1
      );
   if( !Win32::SetConsoleWindowInfo( d_hConsoleScreenBuffer, 1, &newWinMapRect ) ) {
      linebuf oseb;
      DBG( "%s FAILED: %s", __func__, OsErrStr( BSOB(oseb) ) );
      return false;
      }
   return true;
   }

// Reposition the window to keep it entirely on-screen (insofar as possible).
// 20090613 kgoodwin based on win_pos in cs.c.txt from http://www.geocities.com/jadoxa/misc/cs.c.txt
// This isn't perfect since the GetLargestConsoleWindowSize values, which govern
// how big a window we'll try to create, do not include border size, while the
// GetWindowRect of the console window INCLUDES the window's borders, thus the
// max-sized window that ARG::resize can create is slightly larger than it
// should be, resulting in the right border (and part of the right console
// character column) being off the right edge of the screen.  Likewise at the
// bottom...
// 20170214 see AdjustWindowRectEx for an(other) attempt to migitate this.

void win_fully_on_desktop() { enum { SHOWDBG = 1 };
// SHOWDBG && DBG( "SM_CXSIZEFRAME =%4d,%4d", Win32::GetSystemMetrics( SM_CXSIZEFRAME  ), Win32::GetSystemMetrics( SM_CYSIZEFRAME  ) );
// SHOWDBG && DBG( "SM_CXEDGE      =%4d,%4d", Win32::GetSystemMetrics( SM_CXEDGE       ), Win32::GetSystemMetrics( SM_CYEDGE       ) );
// SHOWDBG && DBG( "SM_CXFIXEDFRAME=%4d,%4d", Win32::GetSystemMetrics( SM_CXFIXEDFRAME ), Win32::GetSystemMetrics( SM_CYFIXEDFRAME ) );
// SHOWDBG && DBG( "SM_CXBORDER    =%4d,%4d", Win32::GetSystemMetrics( SM_CXBORDER     ), Win32::GetSystemMetrics( SM_CYBORDER     ) );
   const auto hwnd( Win32::GetConsoleWindow() );
   Win32::UpdateWindow( hwnd ); // seems necessary to get correct rcWinNow values post-TConsoleOutputControl::SetConsoleSizeOk
// Win32::RECT rcDesktop; Win32::GetWindowRect( Win32::GetDesktopWindow(), &rcDesktop   ); /* rect includes taskbar */ SHOWDBG && DBG( "dsktop X=[%4ld,%4ld], Y=[%4ld,%4ld]", rcDesktop.left, rcDesktop.right, rcDesktop.top, rcDesktop.bottom  );
   Win32::RECT rcWinNow; Win32::GetWindowRect( hwnd, &rcWinNow ); SHOWDBG && DBG( "window X=[%4ld,%4ld], Y=[%4ld,%4ld]", rcWinNow.left, rcWinNow.right, rcWinNow.top, rcWinNow.bottom  );
   Win32::RECT bszs = {0};  // bordersizes
   if( false ) { // input (0-sized) rect to AdjustWindowRectEx; output rect contains border sizes/widths http://stackoverflow.com/a/13749190
      Win32::WINDOWINFO wi = { sizeof(wi) }; Win32::GetWindowInfo( hwnd, &wi );
      Win32::AdjustWindowRectEx( &bszs, wi.dwStyle, 0, wi.dwExStyle ); SHOWDBG && DBG( "adjwre X=[%4ld,%4ld], Y=[%4ld,%4ld]", bszs.left, bszs.right, bszs.top, bszs.bottom );
      }
   Win32::RECT workarea; Win32::SystemParametersInfo( SPI_GETWORKAREA, 0, &workarea, 0 ); /*workarea EXcludes taskbar*/ SHOWDBG && DBG( "wkarea X=[%4ld,%4ld], Y=[%4ld,%4ld]", workarea.left, workarea.right, workarea.top, workarea.bottom );
   auto moved(false); auto rcWinNew(rcWinNow);
   if( (rcWinNew.right -bszs.right ) > workarea.right  ) { moved = true; rcWinNew.left -= (rcWinNew.right -bszs.right ) - workarea.right ; }
   if( (rcWinNew.bottom-bszs.bottom) > workarea.bottom ) { moved = true; rcWinNew.top  -= (rcWinNew.bottom-bszs.bottom) - workarea.bottom; }
   // !!! next 2 lines must be AFTER prev 2 lines  NB: bszs.left and bszs.top are (from AdjustWindowRectEx) _negative_!
   if( (rcWinNew.left  -bszs.left  ) < workarea.left   ) { moved = true; rcWinNew.left  = workarea.left; }
   if( (rcWinNew.top   -bszs.top   ) < workarea.top    ) { moved = true; rcWinNew.top   = workarea.top ; }
   if( moved ) { // NB: only rcWinNew.left and rcWinNew.top are relevant (the new ulc of the window)
      SHOWDBG && DBG( "winold X=[%4ld,%4ld], Y=[%4ld,%4ld] width=%4ld, height=%4ld", rcWinNow.left , rcWinNow.right , rcWinNow.top , rcWinNow.bottom
                    , rcWinNow.right  - rcWinNow.left + 1
                    , rcWinNow.bottom - rcWinNow.top  + 1
                    );
      SHOWDBG && DBG( "winnew X=[%4ld,....], Y=[%4ld,....]", rcWinNew.left , rcWinNew.top );
      if( !Win32::SetWindowPos( hwnd, nullptr, rcWinNew.left, rcWinNew.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER ) ) {
         linebuf oseb;
         DBG( "%s: Win32::SetWindowPos FAILED: %s", __func__, OsErrStr( BSOB(oseb) ) );
         }
      }
   }

STATIC_FXN bool SetConsoleBufferSizeOk( const Win32::HANDLE d_hConsoleScreenBuffer, LINE yheight, COL xWidth ) {
   enum { SHOWDBG = 0 };
   Win32::COORD dwSize;
   dwSize.X = xWidth ;
   dwSize.Y = yheight;
   SHOWDBG && DBG( "%s (%dx%d)", __func__, dwSize.X, dwSize.Y );
   if( !Win32::SetConsoleScreenBufferSize( d_hConsoleScreenBuffer, dwSize ) ) {
      // *** ConsoleScreenBuffer resize failed (likely too small): restore window mapping and exit
      linebuf oseb;
      DBG( "%s FAILED: %s", __func__, OsErrStr( BSOB(oseb) ) );
      return false;
      }
   return true;
   }

bool ConOut::SetScreenSizeOk( YX_t &newSize ) {
   return s_EditorScreen ? s_EditorScreen->SetConsoleSizeOk( newSize ) : false;
   }

YX_t ConOut::GetMaxConsoleSize() {
   return s_EditorScreen ? s_EditorScreen->GetMaxConsoleSize() : YX_t(0,0);
   }

YX_t TConsoleOutputControl::GetMaxConsoleSize() {
   std::scoped_lock<std::mutex> mtx( d_mutex );  //##################################################
   const ConsoleScreenBufferInfo csbi( d_hConsoleScreenBuffer, "console resize" );
   if( !csbi.isValid() ) { DBG( "%s: Win32::GetConsoleScreenBufferInfo FAILED", __func__ );
      return YX_t(0,0);
      }
   auto rv( csbi.MaxBufSize() );
   NoMoreThan( &rv.col, static_cast<decltype(rv.col)>(sizeof(Linebuf)-1) );
   return rv;
   }

bool TConsoleOutputControl::SetConsoleSizeOk( YX_t &newSize ) {
   enum { DODBG = 0 };
   std::scoped_lock<std::mutex> mtx( d_mutex );  //##################################################
   FmtStr<80> trying( "%s+ Ask(%dx%d)", __func__, newSize.col, newSize.lin );
   DODBG && DBG( "%s", trying.c_str() );
   // NB: GetLargestConsoleWindowSize()'s retVal will change if the console
   //    font size is changed, so WE CAN'T JUST alloc the dynamic buffers for
   //    the biggest size and forget about them forevermore.
   //
   // Also NB: the font of a newly created console seems to be inherited from any parent console
   //
   const ConsoleScreenBufferInfo csbi( d_hConsoleScreenBuffer, "console resize" );
   if( !csbi.isValid() ) {
      DBG( "%s", trying.c_str() );
      DBG( "%s: Win32::GetConsoleScreenBufferInfo FAILED", __func__ );
      return false;
      }
   const auto winSize( csbi.WindowSize() );
   const auto bufSize( csbi.BufferSize() );
   const auto maxSize( csbi.MaxBufSize() );
   DODBG && DBG( "%s + Win=(%4d,%4d), Buf=(%4d,%4d) Max=(%4d,%4d)", __func__, winSize.col, winSize.lin, bufSize.col, bufSize.lin, maxSize.col, maxSize.lin );
   if( newSize.lin > maxSize.lin )       { newSize.lin = maxSize.lin      ; }
   if( newSize.col > maxSize.col )       { newSize.col = maxSize.col      ; }
   if( newSize.col > sizeof(Linebuf)-1 ) { newSize.col = sizeof(Linebuf)-1; }
   auto retVal(false);
   if(   winSize.col == bufSize.col && bufSize.col == newSize.col
      && winSize.lin == bufSize.lin && bufSize.lin == newSize.lin
     ) {
      DODBG && DBG( "%s - Nothing to do", __func__ );
      goto SIZE_OK;
      }
   do { // NOT a loop; just so I can use break (instead of goto) in this block
      if( newSize.lin < winSize.lin || newSize.col < winSize.col ) {
         // *** Either or BOTH of desired width OR height are LESS THAN current values.
         // *** SHRINK Screen Buffer mapping to minimum to meet
         // *** the requirements of the SetConsoleScreenBufferSize API:
         //
         // "The screen buffer's width AND height cannot be less than the width and
         // height of the (screen buffer's) WINDOW.  The specified dimensions also
         // cannot be less than the minimum size allowed by the system."
         //
         if( !SetConsoleWindowSizeOk( d_hConsoleScreenBuffer, std::min( int(bufSize.lin), newSize.lin ), std::min( int(bufSize.col), newSize.col ) ) ) {
            DBG( "%s shrinking SetConsoleWindowInfo FAILED!", trying.c_str() );
            break;
            }
         }
      if( !SetConsoleBufferSizeOk( d_hConsoleScreenBuffer, newSize.lin, newSize.col ) ) {
         // *** ConsoleScreenBuffer resize failed (likely too small): restore window mapping and exit
         Win32::SetConsoleWindowInfo( d_hConsoleScreenBuffer, 1, &csbi.srWindow() );
         DBG( "%s SetConsoleScreenBufferSize FAILED!", trying.c_str() );
         break;
         }
      //lint -e{734}
      if( !SetConsoleWindowSizeOk( d_hConsoleScreenBuffer, newSize.lin, newSize.col ) ) {
         DBG( "%s growing SetConsoleWindowSizeOk FAILED!", trying.c_str() );
         break;
         }
      {
      // Win32::COORD maxSize = Win32::GetLargestConsoleWindowSize( d_hConsoleScreenBuffer );
      0 && DBG( "%s Max=(%4d,%4d)", __func__, maxSize.col, maxSize.lin );
      // Win32::CONSOLE_SCREEN_BUFFER_INFO csbi;
      // Win32::GetConsoleScreenBufferInfo( d_hConsoleScreenBuffer, &csbi );
      0 && DBG( "%s - Win=(%4d,%4d), Buf=(%4d,%4d) Max=(%4d,%4d)", __func__, winSize.col, winSize.lin, bufSize.col, bufSize.lin, maxSize.col, maxSize.lin );
      }
SIZE_OK:
      win_fully_on_desktop();
      SetNewScreenSize( newSize );
      Event_ScreenSizeChanged( newSize );
      retVal = true;
      } while(0); // NOT a loop; just so I can use break (not goto) above
   0 && DBG( "%s- rv=%d", __func__, retVal );
   return retVal;
   }

// 20191203 thinking about updating Win32ConsoleFontChanger:
//    there are console font APIs that have been officially supported for Vista/S08+:
//
//       https://docs.microsoft.com/en-us/windows/console/console-font-infoex
//       https://docs.microsoft.com/en-us/windows/console/getcurrentconsolefontex
//       https://docs.microsoft.com/en-us/windows/console/setcurrentconsolefontex
//       https://docs.microsoft.com/en-us/windows/console/getconsolefontsize       <-- using this now
//
//    which the current Win32ConsoleFontChanger impl is not using (IIRC) because of a need to support
//    pre-Vista (IOW: XP) OS versions, a need which has now passed into history.
//
//    Unfortunately, while the need to support pre-Vista has passed, it appears the newer/unused APIs
//    do not fully duplicate the functionality of the old, undocumented APIs which the current
//    Win32ConsoleFontChanger impl uses.
//
//    In particular, today GetNumberOfConsoleFonts has no officially supported replacement API.
//

// note that the above APIs are from Vista/S08+, whereas IIRC in recent (past 2-3) years, there as been
// a major effort within MS to revamp the Console subsystem.  I suspect the fruits of this effort have
// NOT been documented in the same place, but in another MS silo (this is one of the major practical
// hassles of programming on MS platform: almost wiki-like siloing of information, with ostensible
// "single-point of reference" docs being superseded elsewhere with no way of knowing that fact (or where
// "elsewhere" is)).

Win32ConsoleFontChanger::~Win32ConsoleFontChanger() {
   Free0( d_pFonts     );
   Free0( d_pFontSizes );
   }

Win32ConsoleFontChanger::Win32ConsoleFontChanger()
   : d_pFonts(nullptr)
   , d_pFontSizes(nullptr)
   { enum { SHOWDBG=1 };
   auto hmod( Win32::GetModuleHandleA("KERNEL32.DLL") );
   if(  !hmod
     || !LoadFuncOk( d_SetConsoleFont         , hmod, "SetConsoleFont"          )
     || !LoadFuncOk( d_GetConsoleFontInfo     , hmod, "GetConsoleFontInfo"      )
     || !LoadFuncOk( d_GetNumberOfConsoleFonts, hmod, "GetNumberOfConsoleFonts" )
     ) {
      return;
      }
   d_hwnd    = Win32::GetConsoleWindow();
   d_hConout = s_EditorScreen->GetConsoleScreenBufferHandle();
   Win32::CONSOLE_FONT_INFO ConsoleCurrentFont;
   d_setFont = d_origFont = GetCurrentConsoleFont( d_hConout, 0, &ConsoleCurrentFont ) ? ConsoleCurrentFont.nFont : 0xFFFF;
   d_num_fonts = d_GetNumberOfConsoleFonts();
   SHOWDBG && DBG( "d_num_fonts = %lu", d_num_fonts );
   if( d_num_fonts > 0 ) {
      AllocArrayNZ( d_pFonts, d_num_fonts );
      GetFontInfo();
      AllocArrayNZ( d_pFontSizes, d_num_fonts );
      for( auto idx(0); idx < d_num_fonts; ++idx ) {
         d_pFontSizes[idx] = Win32::GetConsoleFontSize(d_hConout, d_pFonts[idx].nFont);
         SHOWDBG && DBG( "%cfont[%2d]: XxY = %3dx%3d"
            , idx == ConsoleCurrentFont.nFont ? '*' : ' '
            , idx
            ,                d_pFonts[idx].dwFontSize.X, d_pFonts[idx].dwFontSize.Y
            );
         }
      }
   }

int Win32ConsoleFontChanger::GetFontSizeX() const {
   return d_pFonts ? d_pFontSizes[d_setFont].X : 0;
   }

int Win32ConsoleFontChanger::GetFontSizeY() const {
   return d_pFonts ? d_pFontSizes[d_setFont].Y : 0;
   }

double Win32ConsoleFontChanger::GetFontAspectRatio( int idx ) const {
   if( !validFontIdx( idx ) ) { return 2.0; }
   return double(d_pFontSizes[idx].X) / d_pFontSizes[idx].Y;
   }

void Win32ConsoleFontChanger::GetFontInfo() {
   if( d_pFonts ) {
      const Win32::BOOL fMaxWin = TRUE; // 20090614 kgoodwin this param seems to be ignored
      d_GetConsoleFontInfo( d_hConout, fMaxWin, d_num_fonts, d_pFonts );
      }
   }

// 20090724 kgoodwin for my work and home PC's, MAX_CON_WR_BYTES = 128K
constexpr int MAX_CON_WR_BYTES = 128 * 1024;

void Win32ConsoleFontChanger::SetFont( Win32::DWORD idx ) {
   if( validFontIdx( idx ) && idx != d_setFont ) {
      d_setFont = idx;
      // d_GetConsoleFontInfo NOW (not @ ctor time) because dwFontSize needs to reflect the CURRENT
      // console screen size, which may have changed since ctor time
      d_SetConsoleFont(d_hConout, d_pFonts[idx].nFont);
      Win32::InvalidateRect(d_hwnd, nullptr, FALSE);
      Win32::UpdateWindow(d_hwnd);
      const auto yHeight( d_pFonts[idx].dwFontSize.Y );
            auto xWidth ( d_pFonts[idx].dwFontSize.X );
      const size_t write_bytes( xWidth * (yHeight-2) * sizeof(ScreenCell) );
      if( write_bytes > MAX_CON_WR_BYTES ) {
         xWidth = MAX_CON_WR_BYTES / ((yHeight-2) * sizeof(ScreenCell));
         }
      YX_t newSize; newSize.lin=yHeight; newSize.col=xWidth;
      ConOut::SetScreenSizeOk( newSize );
      }
   }

void ConOut::BufferFlushToScreen() { if( s_EditorScreen ) { s_EditorScreen->FlushConsoleBufferToScreen(); } }

GLOBAL_VAR int g_WriteConsoleOutputCalls;
GLOBAL_VAR int g_WriteConsoleOutputLines;

#define  LOG_CONSOLE_WRITES  0

int  TConsoleOutputControl::WriteConsoleOutput_wrap( LINE yMin, LINE yMax, COL xMin, COL xMax ) {
   // Win32::WriteConsoleOutput will write a minimum rectangle (WriteRegion)
   // based on the min/max dirty column taken across the line range being
   // written.  Win32::WriteConsoleOutput will read
   // d_vOutputBufferCache/d_hConsoleScreenBuffer directly, using bufferSize
   // to determine its geometry.
   // convert everything to Win32-isms
   auto              srcBuffer     ( &d_vOutputBufferCache[0] );
   Win32::COORD      srcBufferDims ; srcBufferDims.X = d_xyState.size.col; srcBufferDims.Y = d_xyState.size.lin;
   Win32::COORD      srcOrigin     ; srcOrigin.X = xMin; srcOrigin.Y = yMin;
   Win32::SMALL_RECT destRect      ; destRect.Top = yMin; destRect.Bottom = yMax; destRect.Left = xMin; destRect.Right = xMax;
   if( 0 ) {
      Assert( destRect.Left    >= 0 );
      Assert( destRect.Right   >= 0 );
      Assert( destRect.Left    <= destRect.Right  );
      Assert( destRect.Top     <= destRect.Bottom );
      Assert( destRect.Top     >= 0 );
      Assert( destRect.Top     <  srcBufferDims.Y );
      Assert( destRect.Bottom  >= 0 );
      Assert( destRect.Bottom  <  srcBufferDims.Y );
      }
   if( LOG_CONSOLE_WRITES ) {
      const size_t write_bytes( (xMax-xMin+1) * (yMax-yMin+1) * sizeof(ScreenCell) );
      if( write_bytes > MAX_CON_WR_BYTES ) { DBG( "*** WriteConsoleOutput w/too-large buffer: %" PR_SIZET, write_bytes ); }
      for( auto iy(yMin); iy <= yMax ; ++iy ) {
         for( auto ix(xMin); ix <= xMax ; ++ix ) {
            auto pCell( &d_vOutputBufferCache[ (iy*d_xyState.size.col)+ix ] );
            const auto ch( pCell->Char.AsciiChar );
            if( ch < ' ' || ch > 0x7E ) { DBG( "*** writing junk (0x%02X) to WriteConsoleOutput[%3d][%3d]", ch, iy, ix ); }
         // else           { DBG( "*** first char is '%c'", ch ); }
            }
         }
      DBG( "*** junk-check done" );
      }
   const auto before( destRect );
   if( !Win32::WriteConsoleOutputA(
             d_hConsoleScreenBuffer // dest buffer handle
           , srcBuffer      // The data to be written to the console screen buffer; treated as the origin of a two-dimensional array of CHAR_INFO structures whose size is specified by srcBufferDims. The total size of the array must be less than 64K.
           , srcBufferDims  // The X,Y size of the buffer pointed to by the lpBuffer parameter, in character cells.
           , srcOrigin      // The coordinates of the upper-left cell in the buffer pointed to by the lpBuffer parameter.
           , &destRect      // ON INPUT , the structure members specify the upper-left and lower-right coordinates of the console screen buffer rectangle to write to.
                            // ON OUTPUT, the structure members specify the actual rectangle that was used.
           )
     ) { linebuf oseb; DBG( "%s FAILED: %s", "WriteConsoleOutput", OsErrStr( BSOB(oseb) ) ); }
   if( LOG_CONSOLE_WRITES &&
       (  before.Top    != destRect.Top
       || before.Bottom != destRect.Bottom
       || before.Left   != destRect.Left
       || before.Right  != destRect.Right
       )
     ) { DBG( "*** WriteConsoleOutput before != destRect" ); }
   if( LOG_CONSOLE_WRITES ) {
      g_WriteConsoleOutputCalls++;
      g_WriteConsoleOutputLines += destRect.Bottom - destRect.Top + 1;
      DBG( "%s --- Win32::WriteConsoleOutput [%3d..%3d], [%3d..%3d]", __func__, destRect.Top, destRect.Bottom, destRect.Left, destRect.Right );
      return 1;
      }
   else {
      return 0;
      }
   }

void TConsoleOutputControl::FlushConsoleBufferToScreen() {
   auto ConWriteCount(0);
   std::scoped_lock<std::mutex> mtx( d_mutex );  //##################################################
#define  CONSOLE_VIDEO_FLUSH_MODE   1
   // CONSOLE_VIDEO_FLUSH_MODE == 0
   // minimizes # of calls to Win32::WriteConsoleOutputA;
   // may unnecessarily rewrite many many bytes/lines
   // CONSOLE_VIDEO_FLUSH_MODE == 1
   // minimizes # of bytes written to video device; may call
   // Win32::WriteConsoleOutputA many many times
   if( d_LineToUpdt.first <= d_LineToUpdt.last ) {
      LOG_CONSOLE_WRITES && DBG( "%s *** line range %d..%d", __func__, d_LineToUpdt.first, d_LineToUpdt.last );
#if  CONSOLE_VIDEO_FLUSH_MODE
      for( auto yLine( d_LineToUpdt.first ) ; yLine <= d_LineToUpdt.last ; ) {
         // find next sequence of dirty lines
         for( ; yLine <= d_LineToUpdt.last; ++yLine ) {
            if( d_vLineControl[yLine].fLineDirty() ) { //*** skip leading not-dirty lines
               break;
               }
            }
         if( !(yLine <= d_LineToUpdt.last) ) {
            break;
            }
         const auto firstDirtyLine( yLine ); // drop anchor
         for( ; yLine < d_LineToUpdt.last; ++yLine ) {
            if( !d_vLineControl[yLine+1].fLineDirty() ) {
               break;
               }
            }
         // next sequence of dirty lines = [firstDirtyLine..yLine];
         // find minimum dirty column-set [xMin..xMax] across this range of lines
         auto xMin(COL_MAX);  auto xMax(-1);
         for( auto iy(firstDirtyLine); iy <= yLine; ++iy ) {
            auto & lc( d_vLineControl[iy] );
            // Assert( lc.fLineDirty() );
            // if( lc.fLineDirty() )
               {
               lc.BoundValidColumns( &xMin, &xMax );
               lc.Undirty();
               }
            }
         // Assert( xMin != COL_MAX );
#else
         const auto xMin(0);  const auto xMax(d_xyState.size.col-1);
         const auto firstDirtyLine( d_LineToUpdt.first );
         const auto yLine         ( d_LineToUpdt.last  );
#endif
         ConWriteCount += WriteConsoleOutput_wrap( firstDirtyLine, yLine, xMin, xMax );
#if  CONSOLE_VIDEO_FLUSH_MODE
         }
#endif
      NullifyUpdtLineRange();
      LOG_CONSOLE_WRITES && DBG( "%s *** %d calls", __func__, ConWriteCount );
      }
   }

void ConOut::GetScreenSize( YX_t *rv ) { // returning 8 byte struct msvc
   W32_ScreenSize_CursorLocn cxy;
   s_EditorScreen->GetSizeCursorLocn( &cxy );
   *rv = cxy.size;
   }

void TConsoleOutputControl::GetSizeCursorLocn( W32_ScreenSize_CursorLocn *cxy ) {
   std::scoped_lock<std::mutex> mtx( d_mutex );  //##################################################
   *cxy = d_xyState;
   }

bool TConsoleOutputControl::WriteToFileOk( FILE *ofh ) { // would be const but use of d_mutex prevents...
   std::scoped_lock<std::mutex> mtx( d_mutex );  //##################################################
   const struct {
      uint32_t magic          ;
      int ySize               ;
      int xSize               ;
      int yCursor             ;
      int xCursor             ;
      uint32_t fCursorVisible ;
      uint32_t fBigCursor     ;
      } fhdr = {
        0x08240418           ,
        d_xyState.size.lin   ,
        d_xyState.size.col   ,
        d_xyState.cursor.lin ,
        d_xyState.cursor.col ,
        d_fCursorVisible     ,
        d_fBigCursor         ,
      };
   if( sizeof fhdr != fwrite( &fhdr, 1, sizeof fhdr, ofh ) ) { DBG( "%s fwrite of fhdr FAILED", __func__ ); return false; }
   auto pPastEnd( &d_vOutputBufferCache[ d_xyState.size.lin * d_xyState.size.col ] );
   for( auto pCI(&d_vOutputBufferCache[0]); pCI < pPastEnd; ++pCI ) {
      uint8_t entry[2] = { pCI->Char.AsciiChar, uint8_t(pCI->Attributes) };
      if( sizeof entry != fwrite( entry, 1, sizeof entry, ofh ) ) { DBG( "%s fwrite of entry FAILED", __func__ ); return false; }
      }
   return true;
   }

bool ConOut::WriteToFileOk( FILE *ofh ) {
   return s_EditorScreen ? s_EditorScreen->WriteToFileOk( ofh ) : false;
   }

STATIC_FXN bool savescreen( CPCChar ofnm ) {
   enum { SHOWDBG=0 };
   const auto ofh( fopen( ofnm, "wb" ) );
   if( !ofh ) { return Msg("open of file \"%s\" FAILED", ofnm ); }   SHOWDBG && DBG( "%s: opened ofh = '%s'", __func__, ofnm );
   const auto rv( ConOut::WriteToFileOk( ofh ) );
   fclose( ofh );                                                    SHOWDBG && DBG( "%s: closed ofh = '%s'", __func__, ofnm );
   Msg( "wrote \"%s\"", ofnm );
   return rv;
   }

#ifdef fn_savescreen
bool ARG::savescreen() { // 20111113 kgoodwin for regression testing purposes
   switch( d_argType ) {
      default     : return BadArg();
      case NOARG  : return ::savescreen( "savescreen"    );
      case TEXTARG: return ::savescreen( d_textarg.pText );
      }
   }
#endif

// 20120821_140511 a currently incomplete attempt to support arbitrary color mapping
// arg "WM_SETCONSOLEINFO" google
//
// When we are started from a CMD.exe shell, our console _IS_ that of the shell
// (and the WM_SETCONSOLEINFO-based code below aborts), but if editor is started
// from a GUI (IOW, has no parent console), the code below works because there is
// no inherited console.

#define WM_SETCONSOLEINFO                       (WM_USER+201)

#pragma pack(push, 1)

// Structure associated with WM_SETCONSOLEINFO
//
typedef struct
{
   Win32::ULONG      Length;
   Win32::COORD      ScreenBufferSize;
   Win32::COORD      WindowSize;
   Win32::ULONG      WindowPosX;
   Win32::ULONG      WindowPosY;

   Win32::COORD      FontSize;
   Win32::ULONG      FontFamily;
   Win32::ULONG      FontWeight;
   Win32::WCHAR      FaceName[32];

   Win32::ULONG      CursorSize;
   Win32::ULONG      FullScreen;
   Win32::ULONG      QuickEdit;
   Win32::ULONG      AutoPosition;
   Win32::ULONG      InsertMode;

   Win32::USHORT     ScreenColors;
   Win32::USHORT     PopupColors;
   Win32::ULONG      HistoryNoDup;
   Win32::ULONG      HistoryBufferSize;
   Win32::ULONG      NumberOfHistoryBuffers;

   Win32::COLORREF   ColorTable[16];

   Win32::ULONG      CodePage;
   Win32::HWND       Hwnd;

   Win32::WCHAR      ConsoleTitle[0x100];

} CONSOLE_INFO;

#pragma pack(pop)

//
//  Wrapper around WM_SETCONSOLEINFO. We need to create the
//  necessary section (file-mapping) object in the context of the
//  process which owns the console, before posting the message
//

// version from http://conemu-maximus5.googlecode.com/svn-history/r12/trunk/ConEmu/src/common/Common.cpp
// with various C++11 mods and cleanup

Win32::HANDLE ghConsoleSection = nullptr;

constexpr int gnConsoleSectionSize = sizeof(CONSOLE_INFO) + 1024;

bool SetConsoleInfo( Win32::HWND hwndConsole, CONSOLE_INFO *pci ) {
   Win32::DWORD dwConsoleOwnerPid; // Retrieve the process which "owns" the console
   const auto dwConsoleThreadId( Win32::GetWindowThreadProcessId( hwndConsole, &dwConsoleOwnerPid ) );
   if( 0 && dwConsoleOwnerPid != Win32::GetCurrentProcessId() ) {
      DBG( "console was created by another process!" );
      return Msg( "console was created by another process!" );
      }
   // Create a SECTION object backed by page-file, then map a view of
   // this section into the owner process so we can write *pci into it
   //
   if( !ghConsoleSection ) {
      ghConsoleSection = Win32::CreateFileMapping( Win32::Invalid_Handle_Value(), nullptr, PAGE_READWRITE, 0, gnConsoleSectionSize, nullptr );
      if( !ghConsoleSection ) {
         linebuf oseb; DBG( "'%s' -> Can't CreateFileMapping(ghConsoleSection): %s", __func__, OsErrStr( BSOB(oseb) ) );
         return false;
         }
      }
   // Copy *pci into the section-object: map, copy, unmap[, SendMessage(hwndConsole, WM_SETCONSOLEINFO...)]
   //
   const auto ptrView( Win32::MapViewOfFile( ghConsoleSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, gnConsoleSectionSize ) );
   if( !ptrView ) {
      linebuf oseb;  DBG( "'%s' -> Can't MapViewOfFile: %s", __func__, OsErrStr( BSOB(oseb) ) );
      return false;
      }
#define  RESTORE_WIN_VISIBILITY  0
#if      RESTORE_WIN_VISIBILITY
   const auto lbWasVisible( Win32::IsWindowVisible( hwndConsole ) );
   Win32::RECT rcOldPos = {0}, rcAllMonRect = {0};
   if( !lbWasVisible ) {
      Win32::GetWindowRect( hwndConsole, &rcOldPos );
      pci->AutoPosition = FALSE;
      pci->WindowPosX = rcAllMonRect.left - 1280;
      pci->WindowPosY = rcAllMonRect.top - 1024;
      }
#endif
   memcpy( ptrView, pci, pci->Length );
   Win32::UnmapViewOfFile( ptrView );
   // Send console window the "update" message
   const auto dwConInfoRc ( Win32::SendMessage( hwndConsole, WM_SETCONSOLEINFO, (Win32::WPARAM)ghConsoleSection, 0 ) );
   const auto dwConInfoErr( Win32::GetLastError() );
#if      RESTORE_WIN_VISIBILITY
   if( !lbWasVisible && Win32::IsWindowVisible( hwndConsole ) ) {
      Win32::ShowWindow( hwndConsole, SW_HIDE );
      // SetWindowPos(hwndConsole, nullptr, rcOldPos.left, rcOldPos.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
      Win32::SetWindowPos( hwndConsole, nullptr, 0, 0, 0,0, SWP_NOSIZE|SWP_NOZORDER );
      }
#endif
#undef  RESTORE_WIN_VISIBILITY
   return true;
   }

bool TConsoleOutputControl::SetConsolePalette( const unsigned palette[16] ) { enum { SD=0 };
   auto hwndConsole( Win32::GetConsoleWindow() );
   CONSOLE_INFO ci = { sizeof(ci) };
   // stuff that seems to be working OK:
   ci.Hwnd                     = hwndConsole;
   ci.ConsoleTitle[0]          = L'\0';
   ci.CodePage                 = 0;//0x352;
   ci.CursorSize               = d_fBigCursor ? 100 : 25;
   ci.FullScreen               = FALSE;
   ci.QuickEdit                = TRUE;
   ci.AutoPosition             = 0x10000;
   ci.InsertMode               = TRUE;
#define  MAKE_WORD(low, high)  ((uint16_t)((((uint16_t)(high)) << 8) | ((uint8_t)(low))))
   ci.ScreenColors             = MAKE_WORD(0x7, 0x0);
   ci.PopupColors              = MAKE_WORD(0x5, 0xf);
   ci.HistoryNoDup             = TRUE;
   ci.HistoryBufferSize        = 1;
   ci.NumberOfHistoryBuffers   = 1;
#define  H(xx)  (((xx)>>16)&BITS(8))|(((xx)&BITS(8))<<16)|((xx)&0xFF00)
   for( auto ix(0); ix < ELEMENTS(ci.ColorTable); ++ix ) {
      ci.ColorTable[ix] = H( palette[ix] );
      }
#undef H
   { // this works OK
   Win32::CONSOLE_SCREEN_BUFFER_INFO csbi;
   Win32::GetConsoleScreenBufferInfo( d_hConsoleScreenBuffer, &csbi );
   ci.ScreenBufferSize = csbi.dwSize;
   ci.WindowSize.X     = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
   ci.WindowSize.Y     = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;  SD && DBG( "%s: %ux%u", __func__, ci.WindowSize.X, ci.WindowSize.Y );
   ci.WindowPosX       = csbi.srWindow.Left;
   ci.WindowPosY       = csbi.srWindow.Top;
   }
   { // this DOES NOT work OK: only in Vista+ is GetCurrentConsoleFontEx available to obtain FontFamily & FontWeight
   Win32::CONSOLE_FONT_INFO cfi;
   auto curFont( Win32::GetCurrentConsoleFont( d_hConsoleScreenBuffer, FALSE, &cfi ) ? cfi.nFont : 0xFFFF );  SD && DBG( "CurFont=%lu", curFont );
   auto fsize( Win32::GetConsoleFontSize( d_hConsoleScreenBuffer, curFont ) );                                SD && DBG( "FontSiz=%u x %u", fsize.X,fsize.Y );
#if 1
   ci.FontSize = fsize;
#else
   ci.FontSize.X = fsize.X;
   ci.FontSize.Y = fsize.Y;
#endif
   ci.FontFamily = 0x30                 ; // 0x30 works (preserves font)
                // FF_MODERN|FIXED_PITCH; // DOESN'T WORK (resets font to ?startup font?)
   // it seems 1000 is the max FontWeight allowed, values above this mean "don't change"?
   // updt: not exactly; the font IS changed when this is executed; the "to what" means
   // the change isn't always NOTICED...
   // arg "LOGFONT structure lfWeight" google
   constexpr int FontWeight_UNCHANGED = 1000+1;  // 0 does NOT have the same effect as 1000+1
   ci.FontWeight = FontWeight_UNCHANGED;
   ci.FaceName[0] = L'\0';
   }
   return SetConsoleInfo( hwndConsole, &ci );
   }

bool ConOut::SetConsolePalette( const unsigned palette[16] ) {
   return s_EditorScreen ? s_EditorScreen->SetConsolePalette( palette ) : false;
   }

#ifdef fn_ctwk

bool ARG::ctwk() {
   STATIC_VAR bool fTweaked;
   // note that these colors are encoded as HTML: RGB (whereas the stupid MS COLORSET is encoded BGR)
   // http://html-color-codes.info/
   STATIC_CONST unsigned TweakedColors[16] =
   {
#if 0
        // this is a goofed-up (rotated) version of the default, so we'll know when it works
        0x0B3B39, 0x1D0A95, 0x008000, 0x008080,
        0x800000, 0x800080, 0x808000, 0xc0c0c0,
        0x808080, 0x0000ff, 0x00ff00, 0x00ffff,
        0xff0000, 0xff00ff, 0xffff00, 0xffffff,
#else
        // what kicked all of this off http://news.ycombinator.com/item?id=4412109
        // http://news.ycombinator.com/item?id=4412471  Solarized color scheme
        0x002b36, 0x073642, 0x586e75, 0x657b83,
        0x839496, 0x93a1a1, 0xeee8d5, 0xfdf6e3,
        0xb58900, 0xcb4b16, 0xdc322f, 0xd33682,
        0x6c71c4, 0x268bd2, 0x2aa198, 0x859900,
#endif
   };
   STATIC_CONST unsigned DefaultColors[16] =
   {
        0x000000, 0x000080, 0x008000, 0x008080,
        0x800000, 0x800080, 0x808000, 0xc0c0c0,
        0x808080, 0x0000ff, 0x00ff00, 0x00ffff,
        0xff0000, 0xff00ff, 0xffff00, 0xffffff,
   };
   const auto rv( ConOut::SetConsolePalette( fTweaked ? DefaultColors : TweakedColors ) );
   fTweaked = !fTweaked;
   DBG( "%s done(%u)", __func__, rv );
   Msg( "%s done(%u)", __func__, rv );
   return rv;
   }
#endif

//
//   END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT
//   END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT
//   END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT
//   END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT  END CONSOLE_OUTPUT
//
//#############################################################################################################################
//#############################################################################################################################

//#############################################################################################################################
//#############################################################################################################################
//
//   BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC
//   BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC
//   BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC
//   BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC  BEGIN INIT_N_MISC
//

STATIC_FXN void Copy_CSBI_content_to_g_pFBufConsole( Win32::HANDLE hConout, const ConsoleScreenBufferInfo &parentCsbi ) { enum {SD=0};
   // scrollback buffer may be vast, and is absolutely empty below parentCsbi.srWindow().Bottom,
   // so to save time and RAM, don't read below parentCsbi.srWindow().Bottom
   const Point src_size( parentCsbi.srWindow().Bottom, parentCsbi.BufferSize().col );
   SD && DBG( "parentCsbi seems to be valid, buf = (%dx%d) (x %" PR_SIZET " bytes) = %" PR_SIZET " bytes", src_size.col, src_size.lin, sizeof(ScreenCell), src_size.col * src_size.lin * sizeof(ScreenCell) );
   if( g_pFBufConsole->LineCount() == 0 ) {
      constexpr int maxReadConsoleBufsize = 48*1024;
      // Documented maxReadConsoleBufsize is 64KB, however experiments and
      // http://www.tech-archive.net/Archive/Development/microsoft.public.win32.programmer.kernel/2005-12/msg00292.html
      // indicate that the limit is "somewhat smaller", so I said "fine, I'll
      // assume the limit is much smaller than the documented limit and be
      // done with it."  20090818 kgoodwin
      Win32::COORD dest_buf_size;  // WARNING WARNING WARNING dest_buf_size members (X, Y) are of type Win32::SHORT (sint16_t) !!!
      dest_buf_size.X  = src_size.col;
      dest_buf_size.Y  = ( maxReadConsoleBufsize / (src_size.col * sizeof(ScreenCell)) );
      SD && DBG( "dest_buf_size.Y = %u, %" PR_SIZET " bytes", dest_buf_size.Y, dest_buf_size.Y * src_size.col * sizeof(ScreenCell) );
      ScreenCell *dest_buf;
      AllocArrayNZ( dest_buf, dest_buf_size.Y * src_size.col );
      std::string chbuf;
      auto lcnt( dest_buf_size.Y );
      for( auto iy(0) ; iy < src_size.lin ; iy += lcnt ) {
         if( lcnt > src_size.lin - iy ) {
             lcnt = src_size.lin - iy;
             }
         Win32::COORD bufPos = {0,0};    // const for all iterations, but since Win32 API does not take as const, best to rewrite each iter
         Win32::SMALL_RECT srcRgn;
         srcRgn.Left   = 0;              // const for all iterations, but Win32 API documents this as "[in, out]", we must rewrite each iter
         srcRgn.Right  = src_size.col-1; // const for all iterations, but Win32 API documents this as "[in, out]", we must rewrite each iter
         srcRgn.Top    = iy;
         srcRgn.Bottom = iy + lcnt - 1;
         SD && DBG( "ReadConsoleOutputA srcRgn: Y=(%4d,%4d), X=(%d,%d)", srcRgn.Top, srcRgn.Bottom, srcRgn.Left, srcRgn.Right );
         if( Win32::ReadConsoleOutputA( hConout, dest_buf, dest_buf_size, bufPos, &srcRgn ) == 0 ) {
            linebuf oseb;
            g_pFBufConsole->FmtLastLine( "***/\n*** Win32::ReadConsoleOutputA FAILED: %s ***\n***\\", OsErrStr( BSOB(oseb) ) );
            }
         else {
            auto pc( dest_buf );
            for( auto jy(0); jy < lcnt; ++jy ) {
               chbuf.clear();
               for( auto jx(0) ; jx < src_size.col ; ++jx ) {
                  chbuf.push_back( (pc++)->Char.AsciiChar );
                  }
               g_pFBufConsole->PutLastLineRaw( chbuf );
               }
            }
         }
      Free0( dest_buf );
      g_pFBufConsole->Undo_Reinit();
      g_pFBufConsole->UnDirty();
      }
   }

STATIC_FXN PCChar ftNm( const Win32::DWORD ft ) {
   switch( ft ) {
      default                : return "unsupported GetFileType() rv";
      case FILE_TYPE_CHAR    : return "character file, typically an LPT or console";
      case FILE_TYPE_DISK    : return "disk file";
      case FILE_TYPE_PIPE    : return "socket, named or anonymous pipe";
      case FILE_TYPE_REMOTE  : return "Remote: unused";
      case FILE_TYPE_UNKNOWN : return "either unknown, or GetFileType() failed";
      }
   }

STATIC_VAR Win32::HANDLE s_hParentActiveConsoleScreenBuffer = nullptr;
STATIC_CONST Win32::UINT s_invalid_CP{ 0xFFFFFFFFu };   CompileTimeAssert( sizeof(Win32::UINT) == 4 );
STATIC_VAR   auto s_atStartupConsoleCP      ( s_invalid_CP );
STATIC_VAR   auto s_atStartupConsoleOutputCP( s_invalid_CP );

bool ConIO::StartupOk( bool fForceNewConsole ) { enum { CON_DBG = 0 }; CON_DBG&&DBG( "%s+", __PRETTY_FUNCTION__ );
   {
   Win32::STARTUPINFO startupInfo = { sizeof startupInfo };
   GetStartupInfo( &startupInfo );
   //    PRTF_IT  printf
#define  PRTF_IT  DBG
   CON_DBG && PRTF_IT( "STARTUPINFO.dwFlags       = 0x%lX", startupInfo.dwFlags       );
   if( CON_DBG && (startupInfo.dwFlags & STARTF_USESIZE      ) ) { PRTF_IT( "   STARTF_USESIZE|"       ); }
   if( CON_DBG && (startupInfo.dwFlags & STARTF_USESTDHANDLES) ) { PRTF_IT( "   STARTF_USESTDHANDLES|" ); }
   if( CON_DBG && (startupInfo.dwFlags & STARTF_USESHOWWINDOW) ) { PRTF_IT( "   STARTF_USESHOWWINDOW|" ); }
   if( CON_DBG && (startupInfo.dwFlags & STARTF_USEPOSITION  ) ) { PRTF_IT( "   STARTF_USEPOSITION|"   ); }
   if( CON_DBG && (startupInfo.dwFlags & STARTF_RUNFULLSCREEN) ) { PRTF_IT( "   STARTF_RUNFULLSCREEN|" ); }
   if( CON_DBG ) {
      PRTF_IT( "\n" );
      PRTF_IT( "STARTUPINFO.wShowWindow   = %d\n"  , startupInfo.wShowWindow   );
      PRTF_IT( "STARTUPINFO.dwXSize       = %ld\n" , startupInfo.dwXSize       );
      PRTF_IT( "STARTUPINFO.dwYSize       = %ld\n" , startupInfo.dwYSize       );
      PRTF_IT( "STARTUPINFO.dwXCountChars = %ld\n" , startupInfo.dwXCountChars );
      PRTF_IT( "STARTUPINFO.dwYCountChars = %ld\n" , startupInfo.dwYCountChars );
      PRTF_IT( "STARTUPINFO.hStdInput     = %p\n"  , startupInfo.hStdInput     );
      PRTF_IT( "STARTUPINFO.hStdOutput    = %p\n"  , startupInfo.hStdOutput    );
      PRTF_IT( "STARTUPINFO.hStdError     = %p\n"  , startupInfo.hStdError     );
      }
   }
   if( CON_DBG ) {
      char pbuf[81];
      PRTF_IT( "Console INPUT  Code Page = Win32::GetConsoleCP()       = %s" , GetCPName( BSOB(pbuf), Win32::GetConsoleCP() ) );
      PRTF_IT( "Console OUTPUT Code Page = Win32::GetConsoleOutputCP() = %s" , GetCPName( BSOB(pbuf), Win32::GetConsoleOutputCP() ) );
      PRTF_IT( "Misc Code Pages:" );
      PRTF_IT( "  Win32::GetOEMCP()           = %s" , GetCPName( BSOB(pbuf), Win32::GetOEMCP() ) );
      PRTF_IT( "  Win32::GetACP() (ANSI)      = %s" , GetCPName( BSOB(pbuf), Win32::GetACP() )   );
      }
#undef   PRTF_IT
   CON_DBG&&DBG( "%s 1", __PRETTY_FUNCTION__ );
   {
   const auto OemCP( Win32::GetOEMCP() );
   if( (s_atStartupConsoleCP=Win32::GetConsoleCP())             != OemCP && !Win32::SetConsoleCP(OemCP) ) {
      linebuf oseb; VidInitApiError( FmtStr<120>( "Win32::SetConsoleCP(OemCP) FAILED: %s", OsErrStr( BSOB(oseb) ) ) );
      }
   if( (s_atStartupConsoleOutputCP=Win32::GetConsoleOutputCP()) != OemCP && !Win32::SetConsoleOutputCP(OemCP) ) {
      linebuf oseb; VidInitApiError( FmtStr<120>( "Win32::SetConsoleOutputCP(OemCP) FAILED: %s", OsErrStr( BSOB(oseb) ) ) );
      }
   }
   // following is VERY sequence-dependent code
   // PHASE 1: close all STDHANDLES, in the process capturing any needed info associated
   // PHASE 1: stdin
   { // close any pipes that idiot spawning processes (like Accurev) may have bound to our STDHANDLES
   const auto hStdin( Win32::GetStdHandle( Win32::Std_Input_Handle() ) );
   if( Win32::Invalid_Handle_Value() == hStdin ) {
      linebuf oseb; VidInitApiError( FmtStr<120>( "Win32::GetStdHandle( STD_INPUT_HANDLE ) FAILED: returned INVALID_HANDLE_VALUE %s", OsErrStr( BSOB(oseb) ) ) );
      exit( 1 );
      }
   if( nullptr != hStdin ) {                                                              // VidInitApiError( "Win32::GetStdHandle( STD_INPUT_HANDLE ) FAILED: returned NULL" );
      const auto fti( Win32::GetFileType( hStdin ) );                                        CON_DBG && DBG( "%s: shi=%p, fti = 0x%lX (%s)\n", __func__, hStdin, fti, ftNm( fti ) );
      // if( fti == FILE_TYPE_PIPE ) {
            Win32::CloseHandle( hStdin ); // close their fucking pipe
      //    }
      }
   }
   CON_DBG&&DBG( "%s 20", __PRETTY_FUNCTION__ );
   Point initialWinSize( 43, 120 );
   auto fNewConsole( false );
   // PHASE 1: stdout
   {
   const auto hConout( Win32::GetStdHandle( Win32::Std_Output_Handle() ) );
   if( Win32::Invalid_Handle_Value() == hConout ) {
      linebuf oseb; VidInitApiError( FmtStr<120>( "Win32::GetStdHandle( STD_OUTPUT_HANDLE ) FAILED: returned INVALID_HANDLE_VALUE %s", OsErrStr( BSOB(oseb) ) ) );
      exit( 1 );
      }
   if( nullptr != hConout ) {                                                             // VidInitApiError( "Win32::GetStdHandle( STD_INPUT_HANDLE ) FAILED: returned NULL" );
      const auto fto( Win32::GetFileType( hConout ) );                                       CON_DBG && DBG( "%s: sho=%p, fto = 0x%lX (%s)\n", __func__, hConout, fto, ftNm( fto ) );
      if( FILE_TYPE_CHAR == fto ) {
         const ConsoleScreenBufferInfo parentCsbi( hConout, "parentConsole", true );
         fNewConsole = !parentCsbi.isValid() || fForceNewConsole;                            CON_DBG && DBG( "%s: fNewConsole=%d=%d|%d\n", __func__, fNewConsole, !parentCsbi.isValid(), fForceNewConsole );
         if( parentCsbi.isValid() ) { // this is almost certainly true if (FILE_TYPE_CHAR == fto), but...
            if( fNewConsole ) printf( "K editor PID %lu%s", Win32::GetCurrentProcessId(), (CON_DBG?"\n":"") );
            Copy_CSBI_content_to_g_pFBufConsole( hConout, parentCsbi );
            s_hParentActiveConsoleScreenBuffer = hConout;
            initialWinSize = parentCsbi.WindowSize();                                        CON_DBG && DBG( "%s: initialWinSize from parent = %dx%d\n", __func__, initialWinSize.col, initialWinSize.lin );
            }
         }
      if( nullptr == s_hParentActiveConsoleScreenBuffer ) { // s_hParentActiveConsoleScreenBuffer later used to FIX Win7 exit-leaves-CMD.EXE-hung issue, so DON'T CLOSE IT!!!
         Win32::CloseHandle( hConout ); // side effect of not closing parent console (CMD.exe) handle is that subsequent writes or ours to stdout (e.g. from Lua io.write()) go to the parent console
         }
      }
   }                                                                                       CON_DBG&&DBG( "%s 30", __PRETTY_FUNCTION__ );
   // PHASE 1: stderr
   Win32::CloseHandle( Win32::GetStdHandle( Win32::Std_Error_Handle() ) ); // we have no use for this handle
   if( fNewConsole ) {
      const auto fFreeConOK( Win32::FreeConsole() != 0 ); // detach from any associated console
                                                                                           CON_DBG&&DBG( "Win32::FreeConsole() %s" , fFreeConOK  ? "succeeded" : "FAILED" );
      const auto fAllocConOK( Win32::AllocConsole() != 0 ); // attach to a new console
      if( fAllocConOK ) {  CON_DBG && DBG( "Win32::AllocConsole() succeeded" ); }
      else { linebuf oseb; CON_DBG && DBG( "Win32::AllocConsole() FAILED: %s", OsErrStr( BSOB(oseb) ) ); }
      }                                                                                    CON_DBG&&DBG( "%s 40", __PRETTY_FUNCTION__ );
   s_EditorScreen = new TConsoleOutputControl( initialWinSize.lin, initialWinSize.col );   CON_DBG&&DBG( "%s 50", __PRETTY_FUNCTION__ );
   Conin_Init();                                                                           CON_DBG&&DBG( "%s 60", __PRETTY_FUNCTION__ );
   UpdateConsoleTitle_Init();
   UpdateConsoleTitle();
   DBG( "%s-", __PRETTY_FUNCTION__ );
   return true;
   }

void ConIO::Shutdown() {
   if( s_hParentActiveConsoleScreenBuffer ) {
      Win32::SetConsoleActiveScreenBuffer( s_hParentActiveConsoleScreenBuffer );
      }
   if( s_atStartupConsoleCP       != s_invalid_CP && s_atStartupConsoleCP       != Win32::GetConsoleCP()       ) { Win32::SetConsoleCP      ( s_atStartupConsoleCP       ); }
   if( s_atStartupConsoleOutputCP != s_invalid_CP && s_atStartupConsoleOutputCP != Win32::GetConsoleOutputCP() ) { Win32::SetConsoleOutputCP( s_atStartupConsoleOutputCP ); }
   ConinRelease();
   }
//
//#############################################################################################################################
//#############################################################################################################################
