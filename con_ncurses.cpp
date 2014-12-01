//
//  Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include <vector>
#include <ncurses.h>
#include <stdio.h>
#include "conio.h"


#define PC_BLACK   0
#define PC_BLUE    1
#define PC_GREEN   2
#define PC_CYAN    3
#define PC_RED     4
#define PC_PURPLE  5
#define PC_YELLOW  6
#define PC_WHITE   7

static void set_pcattr( int attr ) {
   attr &= 0x7F; // ncurses does not have "high intensity" background attr, so clear it
   static unsigned char s_last_pcattr;
   if( s_last_pcattr == attr )  { return; }
   s_last_pcattr = attr;

   const auto pcfg_bold( attr & 0x8 );   const auto ncfg_bold( pcfg_bold ? A_BOLD : 0 );
   attr &= 0x77; // clear "bold" (FG high intensity) bit from search key
   auto color_pr_num( 0 );
   {
   static std::vector<int> s_color_map; // mapping of attributes to ncurses API's is dynamic (vs. on PC's it's fixed)
   for( auto it( s_color_map.cbegin() ) ; it != s_color_map.cend() ; ++it ) {
      if( *it == attr ) {
         color_pr_num = 1+ std::distance( s_color_map.cbegin(), it );
         break;
         }
      }
   if( 0 == color_pr_num ) {
      s_color_map.emplace_back( attr );
      static const int pc_to_ncurses_color[] = {
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
      color_pr_num = 1+ (s_color_map.size() - 1);
      init_pair( color_pr_num, ncfg, ncbg );
      }
   }
   attrset( COLOR_PAIR(color_pr_num) | ncfg_bold );
   }
void ConIO::SetCursorSize( bool fBigCursor ) {}
static YX_t s_cursor_pos;
bool ConIO::GetCursorState( YX_t *pt, bool *pfVisible ) {
   *pt = s_cursor_pos;
   *pfVisible = true;
   return true;
   }
bool ConIO::SetCursorVisibilityChanged( bool fVisible ) { return false; }
void ConIO::SetCursorLocn( int yLine, int xCol ) {
   s_cursor_pos.lin = yLine;
   s_cursor_pos.col = xCol;
   }
int ConIO::BufferWriteString( const char *pszStringToDisp, int StringLen, int yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs ) {
   set_pcattr( colorAttribute );
   int sizeY, sizeX;  getmaxyx( stdscr, sizeY, sizeX );
   if( sizeY==sizeY && xColWithinConsoleWindow >= sizeX ) { return 0; }
   int slen = StringLen;
   for( auto ix(0) ; ix < slen; ++ix ) {
      if( '\0' == pszStringToDisp[ix] ) {
         slen = ix;
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
bool ConIO::SetScreenSizeOk( YX_t &newSize ) { return false; }
void ConIO::GetScreenSize( YX_t *rv ) {
   getmaxyx( stdscr, rv->lin, rv->col );
   }
YX_t ConIO::GetMaxConsoleSize() {
   YX_t rv;
   ConIO::GetScreenSize( &rv );
   return rv;
   }
void ConIO::BufferFlushToScreen() {
   move( s_cursor_pos.lin, s_cursor_pos.col );
   refresh();
   }
bool ConIO::WriteToFileOk( FILE *ofh ) {
   return false;
   }
bool ConIO::SetConsolePalette( const unsigned palette[16] ) { return false; }
bool ConIO::StartupOk( bool fForceNewConsole ) {
   initscr();
   if(has_colors() == FALSE) {
      endwin();
      fprintf( stderr, "Your terminal does not support color\n" );
      return false;
      }

   start_color();
   keypad(stdscr, TRUE);
   noecho();
   return true;
   }
void ConIO::Shutdown() {
   endwin();
   }
