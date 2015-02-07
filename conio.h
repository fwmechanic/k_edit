//
// Copyright 2015 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#pragma once

#include "attr_format.h"

struct YX_t {
   int  lin;
   int  col;
   YX_t() {} //lint !e1401
   YX_t( int lin_, int col_ ) : lin(lin_) , col(col_) {}
   bool operator==( const YX_t &rhs ) const { return lin == rhs.lin && col == rhs.col; }
   bool operator!=( const YX_t &rhs ) const { return !(*this == rhs); }
   };

struct EdKC_Ascii {
   unsigned short EdKcEnum;
            char  Ascii;       // exists because NUMLOCK-masked EdKC values != correct number key ascii values
   };

namespace ConIO {
   bool  StartupOk( bool fForceNewConsole );
   void  Shutdown();

   int   DbgPopf( const char *fmt, ... ) ATTR_FORMAT(1, 2);
   bool  Confirm( const char *pszPrompt );
   }

namespace ConOut {
   void  Bell();

   YX_t  GetMaxConsoleSize();

   void  GetScreenSize( YX_t *rv);
   bool  SetScreenSizeOk( YX_t &newSize );
   void  Resize();

   bool  GetCursorState( YX_t *pt, bool *pfVisible );
   void  SetCursorLocn( int yLine, int xCol );
   void  SetCursorSize( bool fBigCursor );
   bool  SetCursorVisibilityChanged( bool fVisible );

   int   BufferWriteString( const char *pszStringToDisp, int StringLen, int yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs );
   void  BufferFlushToScreen();

   bool  WriteToFileOk( FILE *ofh );
   bool  SetConsolePalette( const unsigned palette[16] );
   }

namespace ConIn {
   EdKC_Ascii EdKC_Ascii_FromNextKey();
   EdKC_Ascii EdKC_Ascii_FromNextKey_Keystr( char *pKeyStringBuffer, size_t pKeyStringBufferBytes );

   bool  FlushKeyQueueAnythingFlushed();
   void  WaitForKey();
   bool  KbHit();
   }
