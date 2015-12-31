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

#include <ncurses.h>
#include <stdio.h>
#include "conio.h"
#include "ed_main.h"

#define PC_BLACK   0
#define PC_BLUE    1
#define PC_GREEN   2
#define PC_CYAN    3
#define PC_RED     4
#define PC_PURPLE  5
#define PC_YELLOW  6
#define PC_WHITE   7

// extern void  Event_ScreenSizeChanged( const Point &newSize );

static void set_pcattr( int attr ) {
   attr &= 0x7F; // ncurses does not have "high intensity" background attr(???), so clear it
   static unsigned char s_last_pcattr;
   if( s_last_pcattr == attr )  { return; }
   s_last_pcattr = attr;

   const auto pcfg_bold( attr & 0x8 );   const auto ncfg_bold( pcfg_bold ? A_BOLD : 0 );
   attr &= 0x77; // clear "bold" (FG high intensity) bit from search key
   auto color_pr_num( 0 );
   {
   static unsigned char nxt_color_pr_num;
   static std::array<unsigned char, 256> attr_to_color_pr_num;
   if( 0 == attr_to_color_pr_num[attr] ) {
      STATIC_CONST int pc_to_ncurses_color[] = {
          [PC_BLACK ] = COLOR_BLACK  ,
          [PC_BLUE  ] = COLOR_BLUE   ,
          [PC_GREEN ] = COLOR_GREEN  ,
          [PC_CYAN  ] = COLOR_CYAN   ,
          [PC_RED   ] = COLOR_RED    ,
          [PC_PURPLE] = COLOR_MAGENTA,
          [PC_YELLOW] = COLOR_YELLOW ,
          [PC_WHITE ] = COLOR_WHITE  ,
          };
      const auto pcfg(  attr       & 0x7 ); const auto ncfg( pc_to_ncurses_color[pcfg] );
      const auto pcbg( (attr >> 4) & 0x7 ); const auto ncbg( pc_to_ncurses_color[pcbg] );
      init_pair( ++nxt_color_pr_num, ncfg, ncbg );
      attr_to_color_pr_num[attr] = nxt_color_pr_num;
      }
   color_pr_num = attr_to_color_pr_num[attr];
   }
   attrset( COLOR_PAIR(color_pr_num) | ncfg_bold );
   }
void ConOut::SetCursorSize( bool fBigCursor ) {}
static YX_t s_cursor_pos;
bool ConOut::GetCursorState( YX_t *pt, bool *pfVisible ) {
   *pt = s_cursor_pos;
   *pfVisible = true;
   return true;
   }
bool ConOut::SetCursorVisibilityChanged( bool fVisible ) {
   curs_set( (fVisible ? 1 : 0) );
   refresh();
   return false;
   }
void ConOut::SetCursorLocn( int yLine, int xCol ) {
   s_cursor_pos.lin = yLine;
   s_cursor_pos.col = xCol;
   move( s_cursor_pos.lin, s_cursor_pos.col );  0 && DBG( "%s move(%d,%d)", FUNC, s_cursor_pos.lin, s_cursor_pos.col );
   refresh();
   }
int ConOut::BufferWriteString( const char *pszStringToDisp, int StringLen, int yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs ) {
   0 && DBG( "%s@%d,%d=%.*s'", __func__, yLineWithinConsoleWindow, xColWithinConsoleWindow, StringLen, pszStringToDisp );
   set_pcattr( colorAttribute );
   int sizeY, sizeX;  getmaxyx( stdscr, sizeY, sizeX );
   if( yLineWithinConsoleWindow >= sizeY ) { return 0; }
   if( xColWithinConsoleWindow  >= sizeX ) { return 0; }
   int slen = StringLen;
   for( auto ix(0) ; ix < slen; ++ix ) {
      if( '\0' == pszStringToDisp[ix] ) {
         slen = ix;
         if( !fPadWSpcs && slen < StringLen ) { DBG( "%s short string received: %d < %d", __func__, slen, StringLen ); }
         break;
         }
      }
   int maxX_notwritten = xColWithinConsoleWindow + slen;
   if( slen > 0 ) {
      if( maxX_notwritten > sizeX ) { slen -= (maxX_notwritten - sizeX); }
      mvaddnstr( yLineWithinConsoleWindow, xColWithinConsoleWindow, pszStringToDisp, slen );
      }
   if( fPadWSpcs && maxX_notwritten < sizeX ) {
      for( auto ix(maxX_notwritten) ; ix < sizeX ; ++ix ) {
         mvaddch( yLineWithinConsoleWindow, ix, ' ' );
         }
      return sizeX - xColWithinConsoleWindow + 1;
      }
   return slen;
   }
void ConOut::BufferFlushToScreen() {
   move( s_cursor_pos.lin, s_cursor_pos.col );  0 && DBG( "%s move(%d,%d)", FUNC, s_cursor_pos.lin, s_cursor_pos.col );
   refresh();
   }

bool ConOut::SetScreenSizeOk( YX_t &newSize ) { return false; }
void ConOut::GetScreenSize( YX_t *rv ) {
   getmaxyx( stdscr, rv->lin, rv->col );
   }
YX_t ConOut::GetMaxConsoleSize() {
   YX_t rv;
   ConOut::GetScreenSize( &rv );
   return rv;
   }
void ConOut::Resize() {
   const auto sizeNow( ConOut::GetMaxConsoleSize() );                  0 && DBG( "%s: size=y,x=%d,%d", __func__, sizeNow.lin, sizeNow.col );
   Event_ScreenSizeChanged( Point( sizeNow.lin, sizeNow.col ) );
   }

bool ConOut::WriteToFileOk( FILE *ofh ) {
   return false;
   }
bool ConOut::SetConsolePalette( const unsigned palette[16] ) { return false; }

extern void conin_ncurses_init();
bool ConIO::StartupOk( bool fForceNewConsole ) {
   use_extended_names(TRUE);
   ESCDELAY = 10;
   initscr();
   raw();
   if( has_colors() == FALSE ) {
      endwin();
      DBG( "Your terminal does not support color\n" );
      return false;
      }

   start_color();  DBG( "TERM=%s\ncurses_version()=\"%s\"\ncan_change_color() = %d\nKEY_MIN=%d, KEY_MAX=%d\nCOLORS = %d\nCOLOR_PAIRS = %d", getenv("TERM"), curses_version(), can_change_color(), KEY_MIN, KEY_MAX, COLORS, COLOR_PAIRS );
   conin_ncurses_init();
   ConOut::Resize();
   return true;
   }
void ConIO::Shutdown() {
   endwin();
   }
