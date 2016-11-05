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

enum { // these are COLOR CODES!
   fgBLK =  0x0,
   fgBLU =  0x1,
   fgGRN =  0x2,
   fgCYN =  0x3,
   fgRED =  0x4,
   fgPNK =  0x5,
   fgBRN =  0x6,
   fgDGR =  0x7,
   fgMGR =  0x8,
   fgLBL =  0x9,
   fgLGR =  0xA,
   fgLCY =  0xB,
   fgLRD =  0xC,
   fgLPK =  0xD,
   fgYEL =  0xE,
   fgWHT =  0xF,
   //------------
   bgBLK = (fgBLK<<4),
   bgBLU = (fgBLU<<4),
   bgGRN = (fgGRN<<4),
   bgCYN = (fgCYN<<4),
   bgRED = (fgRED<<4),
   bgPNK = (fgPNK<<4),
   bgBRN = (fgBRN<<4),
   bgDGR = (fgDGR<<4),
   bgMGR = (fgMGR<<4),
   bgLBL = (fgLBL<<4),
   bgLGR = (fgLGR<<4),
   bgLCY = (fgLCY<<4),
   bgLRD = (fgLRD<<4),
   bgLPK = (fgLPK<<4),
   bgYEL = (fgYEL<<4),
   bgWHT = (fgWHT<<4),

   //------------ masks
   FGmask=0x0F, BGmask=0xF0,
   FGhi  =0x08, BGhi  =0x80,
   FGbase=0x07, BGbase=0x70,
   };

typedef uint8_t colorval_t;

struct YX_t {
   int  lin;
   int  col;
   YX_t() : lin(0) , col(0) {}
   YX_t( int lin_, int col_ ) : lin(lin_) , col(col_) {}
   bool operator==( const YX_t &rhs ) const { return lin == rhs.lin && col == rhs.col; }
   bool operator!=( const YX_t &rhs ) const { return !(*this == rhs); }
   };

typedef unsigned short EdKc_t;
struct EdKC_Ascii {
   EdKc_t EdKcEnum;
   char   Ascii;       // exists because NUMLOCK-masked EdKC values != correct number key ascii values
   };

   enum ConfirmResponse { crYES, crNO, crCANCEL };

namespace ConIO {
   bool  StartupOk( bool fForceNewConsole );
   void  Shutdown();

   int   DbgPopf( const char *fmt, ... ) ATTR_FORMAT(1, 2);
   bool  Confirm( const char *pszPrompt );
   ConfirmResponse Confirm_wCancel( const char *pszPrompt );
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
   EdKC_Ascii EdKC_Ascii_FromNextKey_Keystr( std::string &dest );

   bool  FlushKeyQueueAnythingFlushed();
   void  WaitForKey();
   bool  KbHit();
   }
