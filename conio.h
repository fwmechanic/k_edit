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

enum class fg: uint8_t {
   BLK =  0x0,
   BLU =  0x1,
   GRN =  0x2,
   CYN =  0x3,
   RED =  0x4,
   PNK =  0x5,
   BRN =  0x6,
   DGR =  0x7,
   MGR =  0x8,
   LBL =  0x9,
   LGR =  0xA,
   LCY =  0xB,
   LRD =  0xC,
   LPK =  0xD,
   YEL =  0xE,
   WHT =  0xF,
   };

enum class bg: uint8_t {
   //------------
   BLK = (to_underlying(fg::BLK)<<4),
   BLU = (to_underlying(fg::BLU)<<4),
   GRN = (to_underlying(fg::GRN)<<4),
   CYN = (to_underlying(fg::CYN)<<4),
   RED = (to_underlying(fg::RED)<<4),
   PNK = (to_underlying(fg::PNK)<<4),
   BRN = (to_underlying(fg::BRN)<<4),
   DGR = (to_underlying(fg::DGR)<<4),
   MGR = (to_underlying(fg::MGR)<<4),
   LBL = (to_underlying(fg::LBL)<<4),
   LGR = (to_underlying(fg::LGR)<<4),
   LCY = (to_underlying(fg::LCY)<<4),
   LRD = (to_underlying(fg::LRD)<<4),
   LPK = (to_underlying(fg::LPK)<<4),
   YEL = (to_underlying(fg::YEL)<<4),
   WHT = (to_underlying(fg::WHT)<<4),
   };

typedef uint8_t colorval_t;  // as comprehended by the system's conio API; a.k.a. color attribute a.k.a. attr

STIL constexpr colorval_t fb( fg ff, bg bb ) { return to_underlying( ff ) | to_underlying( bb ); }

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
