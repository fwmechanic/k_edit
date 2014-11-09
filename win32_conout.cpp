// index CONSOLE_OUTPUT KEY_DECODING INIT_N_MISC

//
// Copyright 1991 - 2014 by Kevin L. Goodwin; All rights reserved
//

#include "ed_main.h"
#include "win32_pvt.h"

//--------------------------------------------------------------------------------------------

// see for good explanations:
// http://www.adrianxw.dk/SoftwareSite/Consoles/Consoles5.html


//
//  BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT
//  BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT
//  BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT
//  BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT BEGIN CONSOLE_OUTPUT
//

typedef Win32::CHAR_INFO ScreenCell; // this is copied from our cache by Win32::WriteConsoleOutputA

class TConsoleOutputCacheLineInfo {
   COL         d_xMinDirty;
   COL         d_xMaxDirty;
   ScreenCell *d_paCHAR_INFO_BufStart;

public:

   bool fLineDirty() const { return d_xMinDirty <= d_xMaxDirty; }

   void ColUpdated( int col ) {
      Min( &d_xMinDirty, col );
      Max( &d_xMaxDirty, col );
      }

   void BoundValidColumns( COL *minColUpdated, COL *maxColUpdated ) const {
      Min( minColUpdated, d_xMinDirty );
      Max( maxColUpdated, d_xMaxDirty );
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

typedef TConsoleOutputCacheLineInfo *PLineControl;

struct W32_ScreenSize_CursorLocn {
   Point size;
   Point cursor;
   };

class TConsoleOutputControl {
   const Win32::HANDLE d_hConsoleScreenBuffer;

   int               d_AllocdLineControlLines; // # of elements allocated in d_paLineControl
   PLineControl      d_paLineControl;          // one per line

   bool              d_fCursorVisible;
   bool              d_fBigCursor;

   ScreenCell       *d_pascOutputBufferCache;
   int               d_OutputBufferCacheAllocdCells;

   struct {
      int first;
      int last;
      }              d_LineToUpdt;

   Mutex             d_mutex;

   W32_ScreenSize_CursorLocn d_xyState; // height of CSB-mapped window

public: //**************************************************

   TConsoleOutputControl( int yHeight, int xWidth );

   bool WriteToFileOk( FILE *ofh );

   Win32::HANDLE GetConsoleScreenBufferHandle() const { return d_hConsoleScreenBuffer; }

   void NullifyUpdtLineRange() {
      d_LineToUpdt.first = d_xyState.size.lin;
      d_LineToUpdt.last  = 0;
      }

   void  GetSizeCursorLocn( W32_ScreenSize_CursorLocn *cxy ); // not const cuz hits mutex!
   Point GetMaxConsoleSize();                                 // not const cuz hits mutex!
   bool  GetCursorState( Point *pt, bool *pfVisible );        // not const cuz hits mutex!
   bool  ConsoleSizeSupported( const int newY, const int newX ) const;
   bool  SetConsoleSizeOk( Point &newSize );
   bool  SetCursorLocnOk( LINE yLine, COL xCol );
   bool  SetCursorSizeOk( bool fBigCursor );
   bool  SetCursorVisibilityChanged( bool fVisible );
   COL   WriteLineSegToConsoleBuffer( PCChar pszStringToDisp, COL StringLen, LINE yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs );
   void  FlushConsoleBufferToScreen();
   void  ScrollConsole( LINE ulc_yLine, COL ulc_xCol, LINE lrc_yLine, COL lrc_xCol, LINE deltaLine );
   bool  SetConsolePalette( const unsigned palette[16] );

private://**************************************************

   void SetNewScreenSize( int yHeight, int xWidth );

   int  FlushConsoleBufferLineRangeToWin32( LINE yMin, LINE yMax, COL xMin, COL xMax );
   bool SetConsoleCursorInfoOk();
   };

STATIC_VAR TConsoleOutputControl *s_EditorScreen;

void VidInitApiError( PCChar errmsg ) {
   DbgPopf(         "%s", errmsg );
   DBG(             "%s", errmsg );
   fprintf( stderr, "%s", errmsg );
   }


namespace Win32 {
   class ConsoleScreenBufferInfo {
      HANDLE                     d_hCSB;
      bool                       d_isValid;
      CONSOLE_SCREEN_BUFFER_INFO d_csbi;
      COORD                      d_maxSize;

   public:

      ConsoleScreenBufferInfo( HANDLE hCSB, PCChar name=nullptr, bool fFailQuietly=false );
      bool isValid() const { return d_isValid; }

      const SMALL_RECT & srWindow() const { return d_csbi.srWindow; }

      Point WindowSize()     const { return Point( d_csbi.srWindow.Bottom - d_csbi.srWindow.Top  + 1
                                                 , d_csbi.srWindow.Right  - d_csbi.srWindow.Left + 1
                                                 );
                                   }
      Point BufferSize()     const { return Point( d_csbi.dwSize          .Y  , d_csbi.dwSize          .X   ); }
      Point MaxBufSize()     const { return Point( d_maxSize              .Y-1, d_maxSize              .X-1 ); } // -1 because the MAX is sometimes too big (goes off screen) because of window border thickness
      Point CursorPosition() const { return Point( d_csbi.dwCursorPosition.Y  , d_csbi.dwCursorPosition.X   ); }

      int Attribute() const { return d_csbi.wAttributes; }
      };

   ConsoleScreenBufferInfo::ConsoleScreenBufferInfo( HANDLE hCSB, PCChar name, bool fFailQuietly )
      : d_hCSB( hCSB )
      {
      // NB: Win32::GetLargestConsoleWindowSize RESULT will CHANGE as font of
      //     console is changed (via window-properties dialogs) as we run.
      d_maxSize = Win32::GetLargestConsoleWindowSize( d_hCSB );
      //  /----------------------------------\ <-- documented GetLargestConsoleWindowSize API failure indicator
      if( d_maxSize.X == 0 || d_maxSize.Y == 0 || !GetConsoleScreenBufferInfo( d_hCSB, &d_csbi ) ) {
         d_isValid = false;
         if( name && !fFailQuietly ) {
            linebuf oseb;
            VidInitApiError( FmtStr<120>( "Win32::GetConsoleScreenBufferInfo on %s FAILED: %s", name, OsErrStr( BSOB(oseb) ) ) );
            }
         }
      else {
         d_isValid = true;

#if 0
         // NOTE that sadly we CANNOT create/expand console windows beyond GetLargestConsoleWindowSize
         // (we _sometimes_ desire to do this when running on a homogenous multi-monitor rig)
         // because SetConsoleWindowInfo fails.  This is apparently an OS limitation, and I get NO hits
         // on the web that indicate this is even considered a problem, much less offer a workaround.
         //
         // 20100324 kgoodwin

         const auto smcxvs = Win32::GetSystemMetrics(SM_CXVIRTUALSCREEN), smcxs = Win32::GetSystemMetrics(SM_CXSCREEN);
         const auto smcyvs = Win32::GetSystemMetrics(SM_CYVIRTUALSCREEN), smcys = Win32::GetSystemMetrics(SM_CYSCREEN);

         const auto ratX = (double)smcxvs / smcxs;
         const auto ratY = (double)smcyvs / smcys;

         d_maxSize.X *= ratX;
         d_maxSize.Y *= ratY;

         DBG( "smcxvs=%d, smcxs=%d, ratX=%5.3f, d_maxSize.X=%d", smcxvs, smcxs, ratX, d_maxSize.X );
         DBG( "smcyvs=%d, smcys=%d, ratY=%5.3f, d_maxSize.Y=%d", smcxvs, smcxs, ratX, d_maxSize.Y );
#endif

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
   } // namespace Win32


//
// Given a ConsoleScreenBuffer handle, create a COC and init it with/using the
// dimensions, dflt colors, etc. of that ConsoleScreenBuffer
//

STATIC_FXN Win32::HANDLE NewConsoleScreenBuffer( PCChar tag ) {
   0 && DBG( "%s+ from %s", __func__, tag );
   const auto hcsb( Win32::CreateConsoleScreenBuffer( (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ|FILE_SHARE_WRITE), nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr ) );
   if( hcsb == Win32::Invalid_Handle_Value() ) {
      linebuf oseb;
      VidInitApiError( FmtStr<120>( "Win32::CreateConsoleScreenBuffer FAILED: 0x%08lX=%s", Win32::GetLastError(), OsErrStr( BSOB(oseb) ) ) );
      exit( 1 );
      }
   0 && DBG( "Win32::CreateConsoleScreenBuffer" );
   0 && DBG( "%s- from %s", __func__, tag );
   return hcsb;
   }

TConsoleOutputControl::TConsoleOutputControl( int yHeight, int xWidth )
   : d_hConsoleScreenBuffer         ( NewConsoleScreenBuffer( "#1" ) )
   , d_AllocdLineControlLines       ( 0 )
   , d_paLineControl                ( nullptr )
   , d_fCursorVisible               ( true )
   , d_pascOutputBufferCache        ( nullptr )
   , d_OutputBufferCacheAllocdCells ( 0 )
   {
   0 && DBG( "%s", __func__ );

   // associate d_hConsoleScreenBuffer w/"the window" FIRST so that csbi, which
   // is derived from d_hConsoleScreenBuffer, will return correct window dims

   if( Win32::SetConsoleActiveScreenBuffer( d_hConsoleScreenBuffer ) == 0 ) {
      linebuf oseb;
      VidInitApiError( FmtStr<120>( "Win32::SetConsoleActiveScreenBuffer FAILED: %s", OsErrStr( BSOB(oseb) ) ) );
      exit( 1 );
      }

   Point newSize{ .lin=yHeight, .col=xWidth };
   SetConsoleSizeOk( newSize );

   const Win32::ConsoleScreenBufferInfo csbi( d_hConsoleScreenBuffer, "new editor console" );
   if( !csbi.isValid() )
      exit( 1 ); // error-msgs already displayed, just bail

   d_xyState.cursor = csbi.CursorPosition();
   }

void TConsoleOutputControl::SetNewScreenSize( int yHeight, int xWidth ) {
   d_xyState.size.lin = yHeight; // the ONLY place d_xyState.size is written!
   d_xyState.size.col = xWidth;  // the ONLY place d_xyState.size is written!

   const auto cells( yHeight * xWidth );
   DBG( "+%s (%dx%d) cells=%d->%d (x %Iu = %Iu)", __func__, d_xyState.size.col, d_xyState.size.lin, d_OutputBufferCacheAllocdCells, cells
      , sizeof(*d_pascOutputBufferCache)
      , sizeof(*d_pascOutputBufferCache) * cells
      );
   if( d_OutputBufferCacheAllocdCells < cells ) {
       d_OutputBufferCacheAllocdCells = cells;
       ReallocArray( d_pascOutputBufferCache, d_OutputBufferCacheAllocdCells, "d_pascOutputBufferCache" );
       }
   if( d_AllocdLineControlLines < d_xyState.size.lin ) {
       d_AllocdLineControlLines = d_xyState.size.lin;
       ReallocArray( d_paLineControl, d_AllocdLineControlLines, "d_paLineControl" );
       }
   {
   auto pChIB( d_pascOutputBufferCache );
   const auto pPastEnd( d_paLineControl + d_xyState.size.lin);
   for(  auto pLC     ( d_paLineControl)
      ; pLC < pPastEnd
      ; ++pLC, pChIB += d_xyState.size.col
      ) {
      pLC->Init( pChIB, pChIB + d_xyState.size.col );
      }
   }

   NullifyUpdtLineRange();

   EditorScreenSizeChanged( d_xyState.size );
   DBG( "-%s (%dx%d)", __func__, d_xyState.size.col, d_xyState.size.lin );
   }



GLOBAL_VAR volatile bool g_fSystemShutdownOrLogoffRequested;

HANDLE_CTRL_CLOSE_EVENT( volatile bool g_fProcessExitRequested; )

#if defined(__GNUC__) && !defined(__x86_64)

namespace Win32 {
#ifdef __cplusplus
extern "C" {
#endif
BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsoleOutput,BOOL bMaximumWindow,PCONSOLE_FONT_INFO lpConsoleCurrentFont);
COORD WINAPI GetConsoleFontSize(HANDLE hConsoleOutput,DWORD nFont);

#ifdef __cplusplus
}
#endif
}

#endif


//--------------------------------------------------------------------------------------------


bool TConsoleOutputControl::SetConsoleCursorInfoOk() {
   Win32::CONSOLE_CURSOR_INFO ConsoleCursorInfo;
   ConsoleCursorInfo.bVisible = d_fCursorVisible ? TRUE : FALSE;
   ConsoleCursorInfo.dwSize = d_fBigCursor ? 100 : 25;
   return ToBOOL(Win32::SetConsoleCursorInfo( d_hConsoleScreenBuffer, &ConsoleCursorInfo )); // NZ if OK
   }


bool Video::SetCursorSizeOk( bool fBigCursor ) {
   if(        s_EditorScreen )
       return s_EditorScreen->SetCursorSizeOk( fBigCursor );

   return 0;
   }

bool TConsoleOutputControl::SetCursorSizeOk( bool fBigCursor ) {
   d_fBigCursor = fBigCursor;
   return SetConsoleCursorInfoOk();
   }


bool TConsoleOutputControl::SetCursorVisibilityChanged( bool fVisible ) {
   if( !fVisible &&  d_fCursorVisible ) DBG( "**************** Making CURSOR INVISIBLE *********************" );
   if(  fVisible && !d_fCursorVisible ) DBG( "**************** Making CURSOR   VISIBLE *********************" );
   const auto retVal( d_fCursorVisible != fVisible );
   d_fCursorVisible = fVisible;

   SetConsoleCursorInfoOk();
   return retVal;
   }


bool TConsoleOutputControl::GetCursorState( Point *pt, bool *pfVisible ) {
   AutoMutex mtx( d_mutex );
   *pt        = d_xyState.cursor;
   *pfVisible = d_fCursorVisible;
   return true;
   }

bool Video::GetCursorState( Point *pt, bool *pfVisible ) {
   return s_EditorScreen ? s_EditorScreen->GetCursorState( pt, pfVisible ) : 0;
   }

bool Video::SetCursorVisibilityChanged( bool fVisible ) {
   return s_EditorScreen ? s_EditorScreen->SetCursorVisibilityChanged( fVisible ) : 0;
   }

bool Video::SetCursorLocnOk( LINE yLine, COL xCol ) {
   return s_EditorScreen ? s_EditorScreen->SetCursorLocnOk( yLine, xCol ) : 0;
   }

bool TConsoleOutputControl::SetCursorLocnOk( LINE yLine, COL xCol ) {
   auto rv(false);
   const Point newPt( yLine, xCol );
   AutoMutex mtx( d_mutex );
   if( d_xyState.cursor != newPt ) {
      Win32::COORD dwCursorPosition;
      dwCursorPosition.Y = yLine;
      dwCursorPosition.X = xCol;
      if( Win32::SetConsoleCursorPosition( d_hConsoleScreenBuffer, dwCursorPosition ) ) {
         0 && DBG( "%lX %s(y=%d,x=%d)", Win32::GetCurrentThreadId(), __func__, yLine, xCol );
         d_xyState.cursor = newPt;
         rv = true;
         }
      }

   return rv;
   }

COL Video::BufferWriteString( PCChar pszStringToDisp, COL StringLen, LINE yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs ) {
   return s_EditorScreen ? s_EditorScreen->WriteLineSegToConsoleBuffer( pszStringToDisp, StringLen, yLineWithinConsoleWindow, xColWithinConsoleWindow, colorAttribute, fPadWSpcs ) : 0;
   }

STIL int UpdtCell( const PLineControl lc, int xColWithinConsoleWindow, ScreenCell *pCI, const ScreenCell &ch_info ) {
   if(   ch_info.Attributes     != pCI->Attributes
      || ch_info.Char.AsciiChar != pCI->Char.AsciiChar
     ) {
      lc->ColUpdated( xColWithinConsoleWindow );
      *pCI = ch_info;
      return 1;
      }
   return 0;
   }

COL TConsoleOutputControl::WriteLineSegToConsoleBuffer( PCChar pszStringToDisp, COL StringLen, LINE yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs ) {
   AutoMutex mtx( d_mutex );

   if(   yLineWithinConsoleWindow >= d_xyState.size.lin
      || xColWithinConsoleWindow  >= d_xyState.size.col
      || (StringLen == 0 && !fPadWSpcs)
     ) {
      0 && DBG( "%s BAILS!", __func__ );
      return 0;
      }

   const auto maxConChars( d_xyState.size.col - xColWithinConsoleWindow );
   Min( &StringLen, maxConChars );
   // if( 78==yLineWithinConsoleWindow ) DBG( "%d: '%*s'", yLineWithinConsoleWindow, StringLen, pszStringToDisp );
   const auto padLen( fPadWSpcs ? maxConChars - StringLen : 0 );

   auto updtdCells(0);

   ScreenCell ch_info;
   ch_info.Attributes = colorAttribute;

   const auto lc( d_paLineControl + yLineWithinConsoleWindow );
   auto pCI( lc->BufPtrOfCol( xColWithinConsoleWindow ) );
   for( auto ix(0); ix < StringLen; ++ix, ++pCI, ++xColWithinConsoleWindow ) {
      ch_info.Char.AsciiChar = *pszStringToDisp++;
      updtdCells += UpdtCell( lc, xColWithinConsoleWindow, pCI, ch_info );
      }

   ch_info.Char.AsciiChar = ' ';

   for( auto ix(0); ix < padLen; ++ix, ++pCI, ++xColWithinConsoleWindow ) {
      updtdCells += UpdtCell( lc, xColWithinConsoleWindow, pCI, ch_info );
      }

   if( updtdCells ) {
      NoMoreThan( &d_LineToUpdt.first, yLineWithinConsoleWindow );
      NoLessThan( &d_LineToUpdt.last , yLineWithinConsoleWindow );
      }

   return StringLen + padLen;
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

// Reposition the window to keep it on-screen, or at a specific location.
// 20090613 kgoodwin based on win_pos in cs.c.txt from http://www.geocities.com/jadoxa/misc/cs.c.txt
// This isn't perfect since the GetLargestConsoleWindowSize values, which govern
// how big a window we'll try to create, do not include border size, while the
// GetWindowRect of the console window INCLUDES the window's borders, thus the
// max-sized window that ARG::resize can create is slightly larger than it
// should be, resulting in the right border (and part of the right console
// character column) being off the right edge of the screen.  Likewise at the
// bottom...


void win_fully_on_desktop() {
   enum { SHOWDBG = 0 };
   const auto hwnd( Win32::GetConsoleWindow() );
   Win32::UpdateWindow( hwnd ); // seems necessary to get correct win_now values post-TConsoleOutputControl::SetConsoleSizeOk
   Win32::RECT desktop, workarea, win_now;
   Win32::GetWindowRect( hwnd                     , &win_now     );
   Win32::GetWindowRect( Win32::GetDesktopWindow(), &desktop     ); // rect includes taskbar
   Win32::SystemParametersInfo( SPI_GETWORKAREA, 0, &workarea, 0 ); // rect EXcludes taskbar

   const auto padx(0); // Win32::GetSystemMetrics( SM_CXBORDER );
   const auto pady(0); // Win32::GetSystemMetrics( SM_CYBORDER );

   auto moved(false);
   auto win_new(win_now);
   if( win_new.right +padx > workarea.right  ) { moved = true; win_new.left -= win_new.right  - (workarea.right +(2*padx)); }
   if( win_new.bottom+pady > workarea.bottom ) { moved = true; win_new.top  -= win_new.bottom - (workarea.bottom+(2*pady)); }
   // !!! next 2 lines must be AFTER prev 2 lines
   if( win_new.left  -padx < workarea.left   ) { moved = true; win_new.left  = workarea.left+padx; }
   if( win_new.top   -pady < workarea.top    ) { moved = true; win_new.top   = workarea.top +pady; }

   if( moved ) {
   // SHOWDBG && DBG( "SM_CXSIZEFRAME =%4d,%4d", Win32::GetSystemMetrics( SM_CXSIZEFRAME  ), Win32::GetSystemMetrics( SM_CXSIZEFRAME  ) );
   // SHOWDBG && DBG( "SM_CXEDGE      =%4d,%4d", Win32::GetSystemMetrics( SM_CXEDGE       ), Win32::GetSystemMetrics( SM_CYEDGE       ) );
   // SHOWDBG && DBG( "SM_CXFIXEDFRAME=%4d,%4d", Win32::GetSystemMetrics( SM_CXFIXEDFRAME ), Win32::GetSystemMetrics( SM_CYFIXEDFRAME ) );
   // SHOWDBG && DBG( "SM_CXBORDER    =%4d,%4d", Win32::GetSystemMetrics( SM_CXBORDER     ), Win32::GetSystemMetrics( SM_CYBORDER     ) );
   // SHOWDBG && DBG( "padx=%4d  pady=%4d", padx, pady );
      SHOWDBG && DBG( "desktop  X=[%4ld,%4ld], Y=[%4ld,%4ld]", desktop.left , desktop.right , desktop.top , desktop.bottom  );
      SHOWDBG && DBG( "workarea X=[%4ld,%4ld], Y=[%4ld,%4ld]", workarea.left, workarea.right, workarea.top, workarea.bottom );
      SHOWDBG && DBG( "win      X=[%4ld,%4ld], Y=[%4ld,%4ld] width=%4ld, height=%4ld", win_now.left , win_now.right , win_now.top , win_now.bottom
                    , win_now.right  - win_now.left + 1
                    , win_now.bottom - win_now.top  + 1
                    );
      SHOWDBG && DBG( "win'     X=[%4ld,....], Y=[%4ld,....]", win_new.left , win_new.top );
      if( !Win32::SetWindowPos( hwnd, nullptr, win_new.left, win_new.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER ) ) {
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

bool Video::SetScreenSizeOk( Point &newSize ) {
   return s_EditorScreen ? s_EditorScreen->SetConsoleSizeOk( newSize ) : false;
   }

Point Video::GetMaxConsoleSize() {
   return s_EditorScreen ? s_EditorScreen->GetMaxConsoleSize() : Point(0,0);
   }

Point TConsoleOutputControl::GetMaxConsoleSize() {
   AutoMutex mtx( d_mutex );  //##################################################
   const Win32::ConsoleScreenBufferInfo csbi( d_hConsoleScreenBuffer, "console resize" );
   if( !csbi.isValid() ) {
      DBG( "%s: Win32::GetConsoleScreenBufferInfo FAILED", __func__ );
      return Point(0,0);
      }
   auto rv( csbi.MaxBufSize() );
   NoMoreThan( &rv.col, static_cast<int>(sizeof(Linebuf)-1) );
   return rv;
   }

bool TConsoleOutputControl::ConsoleSizeSupported( const int newY, const int newX ) const {
   const Win32::ConsoleScreenBufferInfo csbi( d_hConsoleScreenBuffer, "console resize" );
   FmtStr<80> trying( "%s+ Ask(%dx%d)", __func__, newX, newY );
   if( !csbi.isValid() ) {
      DBG( "%s", trying.k_str() );
      DBG( "%s: Win32::GetConsoleScreenBufferInfo FAILED", __func__ );
      return false;
      }
   const auto maxSize( csbi.MaxBufSize() );

   if( newY > maxSize.lin )       { return false; }
   if( newX > maxSize.col )       { return false; }
   if( newX > sizeof(Linebuf)-1 ) { return false; }
   return true;
   }

bool TConsoleOutputControl::SetConsoleSizeOk( Point &newSize ) {
   enum { DODBG = 0 };
   AutoMutex mtx( d_mutex );  //##################################################

   FmtStr<80> trying( "%s+ Ask(%dx%d)", __func__, newSize.col, newSize.lin );
   DODBG && DBG( "%s", trying.k_str() );

   // NB: GetLargestConsoleWindowSize()'s retVal will change if the console
   //    font size is changed, so WE CAN'T JUST alloc the dynamic buffers for
   //    the biggest size and forget about them forevermore.
   //
   // Also NB: the font of a newly created console seems to be inherited from any parent console
   //
   const Win32::ConsoleScreenBufferInfo csbi( d_hConsoleScreenBuffer, "console resize" );
   if( !csbi.isValid() ) {
      DBG( "%s", trying.k_str() );
      DBG( "%s: Win32::GetConsoleScreenBufferInfo FAILED", __func__ );
      return false;
      }

   const auto winSize( csbi.WindowSize() );
   const auto bufSize( csbi.BufferSize() );
   const auto maxSize( csbi.MaxBufSize() );
   DODBG && DBG( "%s + Win=(%4d,%4d), Buf=(%4d,%4d) Max=(%4d,%4d)", __func__, winSize.col, winSize.lin, bufSize.col, bufSize.lin, maxSize.col, maxSize.lin );

   if( newSize.lin > maxSize.lin )       { return false; }
   if( newSize.col > maxSize.col )       { return false; }
   if( newSize.col > sizeof(Linebuf)-1 ) { return false; }

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
         if( !SetConsoleWindowSizeOk( d_hConsoleScreenBuffer, Min( int(bufSize.lin), newSize.lin ), Min( int(bufSize.col), newSize.col ) ) ) {
            DBG( "%s shrinking SetConsoleWindowInfo FAILED!", trying.k_str() );
            break;
            }
         }

      if( !SetConsoleBufferSizeOk( d_hConsoleScreenBuffer, newSize.lin, newSize.col ) ) {
         // *** ConsoleScreenBuffer resize failed (likely too small): restore window mapping and exit
         Win32::SetConsoleWindowInfo( d_hConsoleScreenBuffer, 1, &csbi.srWindow() );
         DBG( "%s SetConsoleScreenBufferSize FAILED!", trying.k_str() );
         break;
         }

      //lint -e{734}
      if( !SetConsoleWindowSizeOk( d_hConsoleScreenBuffer, newSize.lin, newSize.col ) ) {
         DBG( "%s growing SetConsoleWindowSizeOk FAILED!", trying.k_str() );
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
      SetNewScreenSize( newSize.lin, newSize.col );

      retVal = true;
      } while(0); // NOT a loop; just so I can use break (not goto) above

   0 && DBG( "%s- rv=%d", __func__, retVal );

   return retVal;
   }


Win32ConsoleFontChanger::~Win32ConsoleFontChanger() {
   Free0( d_pFonts     );
   Free0( d_pFontSizes );
   }

Win32ConsoleFontChanger::Win32ConsoleFontChanger()
   : d_pFonts(nullptr)
   , d_pFontSizes(nullptr)
   {
   auto hmod( Win32::GetModuleHandleA("KERNEL32.DLL") );
   if(  !hmod
     || !LoadFuncOk( d_SetConsoleFont         , hmod, "SetConsoleFont"          )
     || !LoadFuncOk( d_GetConsoleFontInfo     , hmod, "GetConsoleFontInfo"      )
     || !LoadFuncOk( d_GetNumberOfConsoleFonts, hmod, "GetNumberOfConsoleFonts" )
     )
      return;

   d_hwnd    = Win32::GetConsoleWindow();
   d_hConout = s_EditorScreen->GetConsoleScreenBufferHandle();

   Win32::CONSOLE_FONT_INFO ConsoleCurrentFont;
   d_setFont = d_origFont = GetCurrentConsoleFont( d_hConout, 0, &ConsoleCurrentFont ) ? ConsoleCurrentFont.nFont : 0xFFFF;

   d_num_fonts = d_GetNumberOfConsoleFonts();
   AllocArrayNZ( d_pFonts, d_num_fonts, "CONSOLE_FONT_INFO" );
   GetFontInfo();
   AllocArrayNZ( d_pFontSizes, d_num_fonts, "CONSOLE_FONT_SIZE.COORD" );
   for( auto idx(0); idx < d_num_fonts; ++idx ) {
      d_pFontSizes[idx] = Win32::GetConsoleFontSize(d_hConout, d_pFonts[idx].nFont);
      0 && DBG( "%cfont[%2d]: XxY = %3dx%3d"
              , idx == ConsoleCurrentFont.nFont ? '*' : ' '
              , idx
              ,                d_pFonts[idx].dwFontSize.X, d_pFonts[idx].dwFontSize.Y
              );
      }
   }

int Win32ConsoleFontChanger::GetFontSizeX() const {
   return d_pFonts ? d_pFontSizes[d_setFont].X : 0;
   }

int Win32ConsoleFontChanger::GetFontSizeY() const {
   return d_pFonts ? d_pFontSizes[d_setFont].Y : 0;
   }

double Win32ConsoleFontChanger::GetFontAspectRatio( int idx ) const {
   if( !validFontIdx( idx ) ) return 2.0;
   return double(d_pFontSizes[idx].X) / d_pFontSizes[idx].Y;
   }

void Win32ConsoleFontChanger::GetFontInfo() {
   if( d_pFonts ) {
      const Win32::BOOL fMaxWin = TRUE; // 20090614 kgoodwin this param seems to be ignored
      d_GetConsoleFontInfo( d_hConout, fMaxWin, d_num_fonts, d_pFonts );
      }
   }

// 20090724 kgoodwin for my work and home PC's, MAX_CON_WR_BYTES = 128K
enum { MAX_CON_WR_BYTES = 128 * 1024 };

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
      Point newSize{ .lin=yHeight, .col=xWidth };
      Video::SetScreenSizeOk( newSize );
      }
   }

void Video::BufferFlushToScreen() {
   if( s_EditorScreen )
       s_EditorScreen->FlushConsoleBufferToScreen();
   }

GLOBAL_VAR int g_WriteConsoleOutputCalls;
GLOBAL_VAR int g_WriteConsoleOutputLines;

#define  LOG_CONSOLE_WRITES  0

int  TConsoleOutputControl::FlushConsoleBufferLineRangeToWin32( LINE yMin, LINE yMax, COL xMin, COL xMax ) {
   // Win32::WriteConsoleOutput will write a minimum rectangle (WriteRegion)
   // based on the min/max dirty column taken across the line range being
   // written.  Win32::WriteConsoleOutput will read
   // d_pascOutputBufferCache/d_hConsoleScreenBuffer directly, using bufferSize
   // to determine its geometry.

   // convert everything to Win32-isms

   Win32::COORD       bufferSize;
                      bufferSize.X       = d_xyState.size.col;
                      bufferSize.Y       = d_xyState.size.lin;

   Win32::COORD       bufferCoords;
                      bufferCoords.X     = xMin;
                      bufferCoords.Y     = yMin;

   Win32::SMALL_RECT  WriteRegion;
                      WriteRegion.Top    = yMin;
                      WriteRegion.Bottom = yMax;
                      WriteRegion.Left   = xMin;
                      WriteRegion.Right  = xMax;

   Assert( WriteRegion.Left    >= 0 );
   Assert( WriteRegion.Right   >= 0 );
   Assert( WriteRegion.Left    <= WriteRegion.Right  );
   Assert( WriteRegion.Top     <= WriteRegion.Bottom );
   Assert( WriteRegion.Top     >= 0 );
   Assert( WriteRegion.Top     <  bufferSize.Y );
   Assert( WriteRegion.Bottom  >= 0 );
   Assert( WriteRegion.Bottom  <  bufferSize.Y );

   auto               WrittenRegion      ( WriteRegion );

   Assert( WrittenRegion.Left    >= 0 );
   Assert( WrittenRegion.Right   >= 0 );
   Assert( WrittenRegion.Left    <= WrittenRegion.Right  );
   Assert( WrittenRegion.Top     <= WrittenRegion.Bottom );
   Assert( WrittenRegion.Top     >= 0 );
   Assert( WrittenRegion.Top     <  bufferSize.Y );
   Assert( WrittenRegion.Bottom  >= 0 );
   Assert( WrittenRegion.Bottom  <  bufferSize.Y );

#if LOG_CONSOLE_WRITES
   {
   const size_t write_bytes( (xMax-xMin+1) * (yMax-yMin+1) * sizeof(ScreenCell) );
   if( write_bytes > MAX_CON_WR_BYTES ) { DBG( "*** WriteConsoleOutput w/too-large buffer: %Iu", write_bytes ); }
   }

   for( auto iy(yMin); iy <= yMax ; ++iy ) {
      for( auto ix(xMin); ix <= xMax ; ++ix ) {
         auto pCell( d_pascOutputBufferCache+(iy*d_xyState.size.col)+ix );
         const auto ch( pCell->Char.AsciiChar );
         if( ch < ' ' || ch > 0x7E ) { DBG( "*** writing junk (0x%02X) to WriteConsoleOutput[%3d][%3d]", ch, iy, ix ); }
      // else           { DBG( "*** first char is '%c'", ch ); }
         }
      }
   DBG( "*** junk-check done" );

#endif

   if( !
         Win32::WriteConsoleOutputA(  // <---------------------------------------------------
              d_hConsoleScreenBuffer  // <---------------------------------------------------
            , d_pascOutputBufferCache // The data to be written to the console screen buffer; treated as the origin of a two-dimensional array of CHAR_INFO structures whose size is specified by the dwBufferSize parameter. The total size of the array must be less than 64K.
            , bufferSize              // The size of the buffer pointed to by the lpBuffer parameter, in character cells.
            , bufferCoords            // The coordinates of the upper-left cell in the buffer pointed to by the lpBuffer parameter.
            , &WrittenRegion          // ON INPUT , the structure members specify the upper-left and lower-right coordinates of the console screen buffer rectangle to write to.
                                      // ON OUTPUT, the structure members specify the actual rectangle that was used.
            )
     ) {
      linebuf oseb;
      DBG( "%s FAILED: %s", "WriteConsoleOutput", OsErrStr( BSOB(oseb) ) );
      }

#if (1 || LOG_CONSOLE_WRITES)
   if(  WrittenRegion.Top    != WriteRegion.Top
     || WrittenRegion.Bottom != WriteRegion.Bottom
     || WrittenRegion.Left   != WriteRegion.Left
     || WrittenRegion.Right  != WriteRegion.Right
     ) {
       DBG( "*** WriteConsoleOutput WrittenRegion != WriteRegion" );
       }
#endif

#if LOG_CONSOLE_WRITES
   g_WriteConsoleOutputCalls++;
   g_WriteConsoleOutputLines += WriteRegion.Bottom - WriteRegion.Top + 1;
   DBG( "%s --- Win32::WriteConsoleOutput [%3d..%3d], [%3d..%3d]", __func__, WriteRegion.Top, WriteRegion.Bottom, WriteRegion.Left, WriteRegion.Right );
   return 1;
#else
   return 0;
#endif
   }

void TConsoleOutputControl::FlushConsoleBufferToScreen() {
#if LOG_CONSOLE_WRITES
   auto ConWriteCount(0);
#endif
   AutoMutex mtx( d_mutex );  //##################################################

#define  CONSOLE_VIDEO_FLUSH_MODE   1

   // CONSOLE_VIDEO_FLUSH_MODE == 0
   // minimizes # of calls to Win32::WriteConsoleOutputA;
   // may unnecessarily rewrite many many bytes/lines

   // CONSOLE_VIDEO_FLUSH_MODE == 1
   // minimizes # of bytes written to video device; may call
   // Win32::WriteConsoleOutputA many many times

   if( d_LineToUpdt.first <= d_LineToUpdt.last ) {
#if LOG_CONSOLE_WRITES
      DBG( "%s *** line range %d..%d", __func__, d_LineToUpdt.first, d_LineToUpdt.last );
#endif

#if  CONSOLE_VIDEO_FLUSH_MODE
      auto yLine( d_LineToUpdt.first );
      while( yLine <= d_LineToUpdt.last ) {
         for( ; yLine <= d_LineToUpdt.last; ++yLine )
            if( d_paLineControl[yLine].fLineDirty() )  //*** skip leading not-dirty lines
               break;

         if( !(yLine <= d_LineToUpdt.last) )
            break;

         const auto firstDirtyLine( yLine ); // drop anchor
         auto xMin(COL_MAX);  auto xMax(-1);
         for( ; yLine < d_LineToUpdt.last; ++yLine )
            if( !d_paLineControl[yLine+1].fLineDirty() )
               break;

         for( auto iy(firstDirtyLine); iy <= yLine; ++iy ) {
            auto & lc = d_paLineControl[iy];
            if( lc.fLineDirty() ) {
               lc.BoundValidColumns( &xMin, &xMax );
               lc.Undirty();
               }
            }

         Assert( xMin != COL_MAX );

#else
         const auto xMin(0);  const auto xMax(d_xyState.size.col-1);
         const auto firstDirtyLine( d_LineToUpdt.first );
         const auto yLine         ( d_LineToUpdt.last  );
#endif
#if LOG_CONSOLE_WRITES
         ConWriteCount +=
#endif
         FlushConsoleBufferLineRangeToWin32( firstDirtyLine, yLine, xMin, xMax );
#if  CONSOLE_VIDEO_FLUSH_MODE
         }
#endif
      NullifyUpdtLineRange();
#if LOG_CONSOLE_WRITES
      DBG( "%s *** %d calls", __func__, ConWriteCount );
#endif
      }
   }


#if VIDEO_API_SUPPORTED_SCROLL

void Video::Scroll( LINE yUpperLeftCorner, COL xUpperLeftCorner, LINE yLowerRightCorner, COL xLowerRightCorner, LINE deltaLine ) {
   if( s_EditorScreen )
       s_EditorScreen->ScrollConsole( yUpperLeftCorner, xUpperLeftCorner, yLowerRightCorner, xLowerRightCorner, deltaLine );
   }

void TConsoleOutputControl::ScrollConsole(
     LINE yUpperLeftCorner , COL xUpperLeftCorner
   , LINE yLowerRightCorner, COL xLowerRightCorner
   , LINE deltaLine
   ) {
   if( deltaLine == 0 )
      return;

   AutoMutex mtx( d_mutex );  //##################################################

   if( d_LineToUpdt.first <= d_LineToUpdt.last )  // any flushes pending (maybe interrupted)?
      FlushConsoleBufferToScreen();               // complete them

   Win32::COORD dwDestinationOrigin;
   dwDestinationOrigin.X  = xUpperLeftCorner;
   dwDestinationOrigin.Y  = yUpperLeftCorner;
   if( deltaLine > 0 )  dwDestinationOrigin.Y = yUpperLeftCorner - deltaLine;

   Win32::SMALL_RECT ScrollRectangle;
   ScrollRectangle.Left   = xUpperLeftCorner;
   ScrollRectangle.Top    = yUpperLeftCorner;
   ScrollRectangle.Right  = xLowerRightCorner;
   ScrollRectangle.Bottom = yLowerRightCorner;
   if( deltaLine > 0 )  ScrollRectangle.Top    += deltaLine;
   else                 ScrollRectangle.Bottom += deltaLine;

   ScreenCell Fill;
   Fill.Char.AsciiChar = ' ';
   Fill.Attributes = d_W32ConAttr;

   ScrollConsoleScreenBufferA( d_hConsoleScreenBuffer, &ScrollRectangle, nullptr, dwDestinationOrigin, &Fill );

         auto pLC     ( d_paLineControl + yUpperLeftCorner      );
   const auto pPastEnd( d_paLineControl + yLowerRightCorner + 1 );
   for( ; pLC < pPastEnd ; ++pLC )
      pLC->Undirty();
   }

#endif


void Video::GetScreenSize( PPoint rv ) { // returning 8 byte struct msvc
   W32_ScreenSize_CursorLocn cxy;
   s_EditorScreen->GetSizeCursorLocn( &cxy );
   *rv = cxy.size;
   }

void TConsoleOutputControl::GetSizeCursorLocn( W32_ScreenSize_CursorLocn *cxy ) {
   AutoMutex mtx( d_mutex );  //##################################################
   *cxy = d_xyState;
   }

bool TConsoleOutputControl::WriteToFileOk( FILE *ofh ) { // would be const but use of d_mutex prevents...
   AutoMutex mtx( d_mutex );  //##################################################
   const struct {
      U32 magic          ;
      int ySize          ;
      int xSize          ;
      int yCursor        ;
      int xCursor        ;
      U32 fCursorVisible ;
      U32 fBigCursor     ;
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
   auto pPastEnd( d_pascOutputBufferCache + (d_xyState.size.lin * d_xyState.size.col) );
   for( auto pCI(d_pascOutputBufferCache); pCI < pPastEnd; ++pCI ) {
      U8 entry[2] = { pCI->Char.AsciiChar, U8(pCI->Attributes) };
      if( sizeof entry != fwrite( entry, 1, sizeof entry, ofh ) ) { DBG( "%s fwrite of entry FAILED", __func__ ); return false; }
      }
   return true;
   }

bool Video::WriteToFileOk( FILE *ofh ) {
   return s_EditorScreen ? s_EditorScreen->WriteToFileOk( ofh ) : false;
   }

STATIC_FXN bool savescreen( CPCChar ofnm ) {
   enum { SHOWDBG=0 };
   const auto ofh( fopen( ofnm, "wb" ) );
   if( !ofh ) { return Msg("open of file \"%s\" FAILED", ofnm ); }   SHOWDBG && DBG( "%s: opened ofh = '%s'", __func__, ofnm );
   const auto rv( Video::WriteToFileOk( ofh ) );
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
enum { gnConsoleSectionSize = sizeof(CONSOLE_INFO)+1024 };

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

bool TConsoleOutputControl::SetConsolePalette( const unsigned palette[16] ) {
   enum { DM=0 };
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

#define  MAKE_WORD(low, high)  ((U16)((((U16)(high)) << 8) | ((U8)(low))))

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
   ci.WindowSize.Y     = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;  DM && DBG( "%s: %ux%u", __func__, ci.WindowSize.X, ci.WindowSize.Y );
   ci.WindowPosX       = csbi.srWindow.Left;
   ci.WindowPosY       = csbi.srWindow.Top;
   }

   { // this DOES NOT work OK: only in Vista+ is GetCurrentConsoleFontEx available to obtain FontFamily & FontWeight
   Win32::CONSOLE_FONT_INFO cfi;
   auto curFont( Win32::GetCurrentConsoleFont( d_hConsoleScreenBuffer, FALSE, &cfi ) ? cfi.nFont : 0xFFFF );  DM && DBG( "CurFont=%lu", curFont );
   auto fsize( Win32::GetConsoleFontSize( d_hConsoleScreenBuffer, curFont ) );                                DM && DBG( "FontSiz=%u x %u", fsize.X,fsize.Y );

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
   enum { FontWeight_UNCHANGED = 1000+1 };  // 0 does NOT have the same effect as 1000+1
   ci.FontWeight = FontWeight_UNCHANGED;
   ci.FaceName[0] = L'\0';
   }

   return SetConsoleInfo( hwndConsole, &ci );
   }

bool Video::SetConsolePalette( const unsigned palette[16] ) {
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

   const auto rv( Video::SetConsolePalette( fTweaked ? DefaultColors : TweakedColors ) );
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

STATIC_FXN void Copy_CSBI_content_to_g_pFBufConsole( Win32::HANDLE hConout, const Win32::ConsoleScreenBufferInfo &parentCsbi ) {
   // scrollback buffer may be vast, and is absolutely empty below parentCsbi.srWindow().Bottom,
   // so to save time and RAM, don't read below parentCsbi.srWindow().Bottom

   const Point bufsize( parentCsbi.srWindow().Bottom, parentCsbi.BufferSize().col );
   enum { CON_DBG = 0 };
   CON_DBG && DBG( "parentCsbi seems to be valid, buf = (%dx%d) (x %Iu bytes) = %Iu bytes", bufsize.col, bufsize.lin, sizeof(ScreenCell), bufsize.col * bufsize.lin * sizeof(ScreenCell) );
   if( g_pFBufConsole->LineCount() == 0 ) {
      enum { maxReadConsoleBufsize = 48*1024 };

      // Documented maxReadConsoleBufsize is 64KB, however experiments and
      // http://www.tech-archive.net/Archive/Development/microsoft.public.win32.programmer.kernel/2005-12/msg00292.html
      // indicate that the limit is "somewhat smaller", so I said "fine, I'll
      // assume the limit is much smaller than the documented limit and be
      // done with it."  20090818 kgoodwin

      const auto max_lines( maxReadConsoleBufsize / (bufsize.col * sizeof(ScreenCell)) );
      CON_DBG && DBG( "max_lines = %Iu, 0x%IX bytes", max_lines, max_lines * bufsize.col * sizeof(ScreenCell) );
      ScreenCell *buf;
      AllocArrayNZ( buf, max_lines * bufsize.col, "<console> copy buffer" );
      Win32::COORD bufSize;
      bufSize.X  = bufsize.col;
      bufSize.Y  = max_lines;
      auto lcnt( max_lines );
      for( auto iy(0) ; iy < bufsize.lin ; iy += lcnt ) {
         if( lcnt > bufsize.lin - iy )
             lcnt = bufsize.lin - iy;

         Win32::COORD bufPos = {0,0};
         Win32::SMALL_RECT copyFromRect;
         copyFromRect.Left   = 0;
         copyFromRect.Top    = iy;
         copyFromRect.Right  = bufsize.col-1;
         copyFromRect.Bottom = iy + lcnt - 1;
         CON_DBG && DBG( "ReadConsoleOutputA( bufSize=(%dx%d), bufPos=(%d,%d), copyFromRect=[(%d,%d),(%d,%d)] )", bufSize.X, bufSize.Y, bufPos.X, bufPos.X
            , copyFromRect.Left
            , copyFromRect.Top
            , copyFromRect.Right
            , copyFromRect.Bottom
            );
         if( Win32::ReadConsoleOutputA( hConout, buf, bufSize, bufPos, &copyFromRect ) == 0 ) {
            linebuf oseb;
            g_pFBufConsole->PutLastLine( "***/" );
            g_pFBufConsole->FmtLastLine( "*** Win32::ReadConsoleOutputA FAILED: %s ***", OsErrStr( BSOB(oseb) ) );
            g_pFBufConsole->PutLastLine( "***\\" );
            }
         else {
            auto pc(buf);
            for( auto jy(0); jy < lcnt; ++jy ) {
               linebuf lbuf;
               auto jx(0);
               for( ; jx < bufsize.col; ++jx ) {
                  lbuf[jx] = (pc++)->Char.AsciiChar;
                  }
               lbuf[jx] = '\0';
               g_pFBufConsole->PutLastLine( lbuf );
               }
            }
         }

      Free0( buf );

      g_pFBufConsole->ClearUndo();
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

void Video::Startup( bool fForceNewConsole ) { enum { CON_DBG = 0 }; CON_DBG&&DBG( "%s+", __PRETTY_FUNCTION__ );
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
   CON_DBG && PRTF_IT( "\n" );
   CON_DBG && PRTF_IT( "STARTUPINFO.wShowWindow   = %d\n"  , startupInfo.wShowWindow   );
   CON_DBG && PRTF_IT( "STARTUPINFO.dwXSize       = %ld\n" , startupInfo.dwXSize       );
   CON_DBG && PRTF_IT( "STARTUPINFO.dwYSize       = %ld\n" , startupInfo.dwYSize       );
   CON_DBG && PRTF_IT( "STARTUPINFO.dwXCountChars = %ld\n" , startupInfo.dwXCountChars );
   CON_DBG && PRTF_IT( "STARTUPINFO.dwYCountChars = %ld\n" , startupInfo.dwYCountChars );
   CON_DBG && PRTF_IT( "STARTUPINFO.hStdInput     = %p\n"  , startupInfo.hStdInput     );
   CON_DBG && PRTF_IT( "STARTUPINFO.hStdOutput    = %p\n"  , startupInfo.hStdOutput    );
   CON_DBG && PRTF_IT( "STARTUPINFO.hStdError     = %p\n"  , startupInfo.hStdError     );
#undef   PRTF_IT
   }

   CON_DBG&&DBG( "%s 1", __PRETTY_FUNCTION__ );

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
         const Win32::ConsoleScreenBufferInfo parentCsbi( hConout, "parentConsole", true );
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
   }
   CON_DBG&&DBG( "%s 30", __PRETTY_FUNCTION__ );

   // PHASE 1: stderr
   Win32::CloseHandle( Win32::GetStdHandle( Win32::Std_Error_Handle() ) ); // we have no use for this handle

   //------------------------------------------------------------------------------------------------------------------------------------

   if( fNewConsole ) {
      {
      const auto fFreeConOK( Win32::FreeConsole() != 0 ); // detach from any associated console
      CON_DBG && DBG( "Win32::FreeConsole() %s" , fFreeConOK  ? "succeeded" : "FAILED" );
      }

      const auto fAllocConOK( Win32::AllocConsole() != 0 ); // attach to a new console
      if( fAllocConOK ) {
         CON_DBG && DBG( "Win32::AllocConsole() succeeded" );
         }
      else {
         linebuf oseb;
         CON_DBG && DBG( "Win32::AllocConsole() FAILED: %s", OsErrStr( BSOB(oseb) ) );
         }
      }
   CON_DBG&&DBG( "%s 40", __PRETTY_FUNCTION__ );

   //------------------------------------------------------------------------------------------------------------------------------------
   // Win32::HANDLE hConout = Win32::CreateFile( "CONOUT$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0L );
   s_EditorScreen = new TConsoleOutputControl( initialWinSize.lin, initialWinSize.col );

   CON_DBG&&DBG( "%s 50", __PRETTY_FUNCTION__ );
   //====================================================================================================================================

   Conin_Init();

   //------------------------------------------------------------------------------------------------------------------------------------
   CON_DBG&&DBG( "%s 60", __PRETTY_FUNCTION__ );

   ddi_console();
   DBG( "%s-", __PRETTY_FUNCTION__ );
   }


void ConsoleReleaseOnExit() {
   if( s_hParentActiveConsoleScreenBuffer ) {
      Win32::SetConsoleActiveScreenBuffer( s_hParentActiveConsoleScreenBuffer );
      }

   ConinRelease();
   }

//
//#############################################################################################################################
//#############################################################################################################################
