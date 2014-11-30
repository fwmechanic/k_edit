//
//  Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

struct YX_t {
   int  lin;
   int  col;
   YX_t() {} //lint !e1401
   YX_t( int lin_, int col_ ) : lin(lin_) , col(col_) {}
   bool operator==( const YX_t &rhs ) const { return lin == rhs.lin && col == rhs.col; }
   bool operator!=( const YX_t &rhs ) const { return !(*this == rhs); }
   };

namespace Video {
   void  Startup( bool fForceNewConsole );
   YX_t  GetMaxConsoleSize();

   void  GetScreenSize( YX_t *rv);
   bool  SetScreenSizeOk( YX_t &newSize );

   bool  GetCursorState( YX_t *pt, bool *pfVisible );
   void  SetCursorLocn( LINE yLine, COL xCol );
   void  SetCursorSize( bool fBigCursor );
   bool  SetCursorVisibilityChanged( bool fVisible );

   COL   BufferWriteString( PCChar pszStringToDisp, COL StringLen, LINE yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs );
   void  BufferFlushToScreen();

   bool  WriteToFileOk( FILE *ofh );
   bool  SetConsolePalette( const unsigned palette[16] );
   }
