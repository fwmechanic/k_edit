//
// Copyright 2015-2016 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
#include "ed_main.h"
#include "conio.h"

#define PC_BLACK   0
#define PC_BLUE    1
#define PC_GREEN   2
#define PC_CYAN    3
#define PC_RED     4
#define PC_PURPLE  5
#define PC_YELLOW  6
#define PC_WHITE   7

STATIC_FXN int sat_add( int a1, int a2, int max, int min ) {
   auto sum = a1 + a2;
   if( sum > max ) { return max; }
   if( sum < min ) { return min; }
   return sum;
   }

STATIC_FXN int sat_add_nccolor( int a1, int a2 ) {
   return sat_add( a1, a2, 1000, 0 );
   }

struct TACp {
   int attr_A;
   int color_pr_num;
   };

STATIC_FXN void gen_attr_set( int pcattr, TACp &new_aset ) {
   STATIC_VAR unsigned char                  s_nxt_color_pr_num;
   STATIC_VAR std::array<unsigned char, 256> s_pcattr_to_color_pr_num;
   if( 0 == s_pcattr_to_color_pr_num[pcattr] ) {
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
      // init_color/color_content[COLOR_BLACK  ..COLOR_WHITE  ] == [0.. 7] are low  intensity definitions
      // init_color/color_content[COLOR_BLACK+8..COLOR_WHITE+8] == [8..15] are high intensity definitions: +(pcnib & 0x8) references these
      auto pc2nc = []( int pcnib ) { return( pc_to_ncurses_color[pcnib & 0x7]+(pcnib & 0x8) ); };
      const auto ncfg = pc2nc( pcattr      );
      const auto ncbg = pc2nc( pcattr >> 4 );
      if( 0 ) { // table-based modification of default colors; works, but possibly superfluous given FG high intensity bit is supported.
         STATIC_VAR struct { bool modified; int dr; int dg; int db; } s_ncurses_color_mods[] = {
             [COLOR_BLACK  ] = { .modified=false, .dr=   0, .dg=   0, .db=   0 },
             [COLOR_RED    ] = { .modified=false, .dr=   0, .dg=   0, .db=   0 },
             [COLOR_GREEN  ] = { .modified=false, .dr=   0, .dg= 400, .db=   0 },
             [COLOR_YELLOW ] = { .modified=false, .dr=   0, .dg=   0, .db=   0 },
             [COLOR_BLUE   ] = { .modified=false, .dr=   0, .dg=   0, .db=-125 },
             [COLOR_MAGENTA] = { .modified=false, .dr=   0, .dg=   0, .db=   0 },
             [COLOR_CYAN   ] = { .modified=false, .dr=   0, .dg=   0, .db=   0 },
             [COLOR_WHITE  ] = { .modified=false, .dr=   0, .dg=   0, .db=   0 },
             };
         auto modNcColor = []( int cn ) {
            auto& ncm = s_ncurses_color_mods[cn];
            if( !ncm.modified ) {
               ncm.modified = true;
               short r,g,b;
               color_content(cn, &r,&g,&b);
               init_color( cn, sat_add_nccolor(r,ncm.dr), sat_add_nccolor(g,ncm.dg), sat_add_nccolor(b,ncm.db) );
               }
            };
         modNcColor( ncfg );
         modNcColor( ncbg );
         }
      init_pair( ++s_nxt_color_pr_num, ncfg, ncbg );
      s_pcattr_to_color_pr_num[pcattr] = s_nxt_color_pr_num;
      }
   new_aset.color_pr_num = s_pcattr_to_color_pr_num[pcattr];
   }

void ConOut::SetCursorSize( bool fBigCursor ) {}

STATIC_VAR YX_t s_cursor_pos;
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
   move( s_cursor_pos.lin, s_cursor_pos.col );  0 && DBG( "%s move(%d,%d)", __func__, s_cursor_pos.lin, s_cursor_pos.col );
   refresh();
   }

// setcchar(cchar_t) takes a *nul-termd string of* wchar_t because each ncurses cell is able to depict the fusion of CCHARW_MAX unicode chars(!).
// So even though we only ever supply 1x unicode char per cell, we need a 2-element array to do so; define a type for the purpose
typedef wchar_t wchar_t_duo[2];

// it's probably safe to say that xlat_first_CP437_char is a *horrendous* hack!  But learning how to display Unicode chars (interleaved with 8-bit chars) was useful.
STATIC_FXN int xlat_first_CPnil_char( int ix, stref sseg, wchar_t_duo &wc2 ) { return sseg.length(); }
STATIC_FXN int xlat_first_CP437_char( int ix, stref sseg, wchar_t_duo &wc2 ) {
   CompileTimeAssert( sizeof(wc2[0])==4 );
   STATIC_CONST uint16_t xlat_cp437[] = {  // https://en.wikipedia.org/wiki/Code_page_437#Character_set
      0x2205, 0x263a, 0x263b, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 0x25d8, 0x25CB, 0x25d9, 0x2642, 0x2640, 0x266a, 0x266b, 0x263c,  // 0x00
      0x25ba, 0x25c4, 0x2195, 0x203c, 0x00b6, 0x00a7, 0x25ac, 0x21a8, 0x2191, 0x2193, 0x2192, 0x2190, 0x221f, 0x2194, 0x25b2, 0x25bc,  // 0x10
           0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,  // 0x20
           0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,  // 0x30
           0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,  // 0x40
           0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,  // 0x50
           0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,  // 0x60
           0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,  // 0x70
      0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,  // 0x80
      0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,  // 0x90
      0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,  // 0xa0
      0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,  // 0xb0 https://en.wikipedia.org/wiki/Box-drawing_characters#DOS
      0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,  // 0xc0 https://en.wikipedia.org/wiki/Box-drawing_characters#DOS
      0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,  // 0xd0 https://en.wikipedia.org/wiki/Box-drawing_characters#DOS
      0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4, 0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,  // 0xe0
      0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0,  // 0xf0
      };
   for( ; ix < sseg.length(); ++ix ) {
      const unsigned ch = sseg[ix];
      if( ch < ELEMENTS(xlat_cp437) && xlat_cp437[ch] && ch != xlat_cp437[ch] ) {
         wc2[0] = xlat_cp437[ch];    0 && DBG( "%s at %d found 0x%02x -> 0x%04x", __func__, ix, ch, wc2[0] );
         wc2[1] = 0;  // nul
         break;
         }
      }
   return ix;
   }

int ConOut::BufferWriteString( stref src, int yLine, int xCol, int pcattr, bool fPadWSpcs ) {
   enum { DB=0 };
   int sizeY, sizeX;  getmaxyx( stdscr, sizeY, sizeX );
   if( yLine < 0 || yLine >= sizeY ) { return 0; }
   if( xCol  < 0 || xCol  >= sizeX ) { return 0; }   DB && DBG( "%s@%d,%d=%" PR_BSR "'", __func__, yLine, xCol, BSR(src) );

   pcattr &= 0x7F; // we don't use ncurses' "blink" ("high intensity" background) or A_BLINK attr, so mask it
   STATIC_VAR unsigned char s_last_pcattr;
   STATIC_VAR TACp          s_acpr;  // for newer ncurses API (eg setcchar) that merge "wr string" and "set attr for written string" ops
   if( s_last_pcattr != pcattr ) {
      s_last_pcattr = pcattr;
      gen_attr_set( pcattr, s_acpr );
      attrset( COLOR_PAIR(s_acpr.color_pr_num) | s_acpr.attr_A ); // for older ncurses API that separate "wr string" and "set attr for written string" ops
      }
   const auto ixEmbdNul = src.find_first_of( '\0' );
         auto slen = (0 && eosr != ixEmbdNul) ? ixEmbdNul : src.length();
   const auto xMinNotWritten = xCol + slen;
   if( slen > 0 ) {
      if( xMinNotWritten > sizeX ) { slen -= (xMinNotWritten - sizeX); }
      const auto sseg = src.substr( 0, slen );
      const auto xlat_first_CP_char = g_fXlatCP437 ? xlat_first_CP437_char : xlat_first_CPnil_char;
      for( int x0=0; x0 < sseg.length(); ) {
         wchar_t_duo wc2; const auto x1 = xlat_first_CP_char( x0, sseg, wc2 );
         if( x1 > x0 ) { // write any leading non-xlat'd substring
            mvaddnstr( yLine, xCol+x0, sseg.data()+x0, x1 - x0 );   DB && DBG( "%s %c%d: writing [%d] st [%d .. %d]", __func__, x0==0 ? '+':' ', yLine, x0, xCol+x0, xCol+x0+ (x1 - x0 -1) );
            }
         if( x1 < sseg.length() ) { // write any trailing translated wchar at x1
            cchar_t cct; setcchar( &cct, wc2, s_acpr.attr_A, s_acpr.color_pr_num, nullptr );
            mvadd_wch( yLine, xCol+x1, &cct );                      DB && DBG( "%s %c%d: writing at [%d] 0x%04x", __func__, x1==0 ? '+':' ', yLine, xCol+x1, wc2[0] );
            }
         x0 = x1 + 1;
         }
      }
   if( fPadWSpcs && xMinNotWritten < sizeX ) {
      for( auto ix(xMinNotWritten) ; ix < sizeX ; ++ix ) {
         mvaddch( yLine, ix, ' ' );
         }
      return sizeX - xCol + 1;
      }
   if( !fPadWSpcs && slen < src.length() ) {
      for( auto ix(xMinNotWritten) ; ix < xCol + src.length() ; ++ix ) {
         mvaddch( yLine, ix, '%' );
         }
      return src.length();
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

STATIC_FXN void show_color_defs() {
   if( 0 ) {
      short r,g,b;
      for( auto cn=0; color_content(cn, &r,&g,&b) != ERR ; ++cn ) {
         DBG( "  color_content[%3d] = { %4d, %4d, %4d }", cn, r,g,b );
         }
      }
   else {
      STATIC_CONST PCChar cn_to_nm[] = {
         [COLOR_BLACK  ] = "COLOR_BLACK"  ,
         [COLOR_RED    ] = "COLOR_RED"    ,
         [COLOR_GREEN  ] = "COLOR_GREEN"  ,
         [COLOR_YELLOW ] = "COLOR_YELLOW" ,
         [COLOR_BLUE   ] = "COLOR_BLUE"   ,
         [COLOR_MAGENTA] = "COLOR_MAGENTA",
         [COLOR_CYAN   ] = "COLOR_CYAN"   ,
         [COLOR_WHITE  ] = "COLOR_WHITE"  ,
         };
      for( auto cn=0; cn < 3*ELEMENTS(cn_to_nm); ++cn ) {
         short r,g,b;
         if( color_content(cn, &r,&g,&b) != ERR ) {
            DBG( "  color_content[%2d=%-13s] = { %4d, %4d, %4d }", cn, cn < ELEMENTS(cn_to_nm) ? cn_to_nm[cn] : "?", r,g,b );
            }
         }
      }
   }

STATIC_FXN void modify_color_content( int cn, int dr, int dg, int db ) {
   short r,g,b;
   color_content(cn, &r,&g,&b);
   init_color( cn, sat_add_nccolor(r,dr), sat_add_nccolor(g,dg), sat_add_nccolor(b,db) );
   }

STATIC_FXN void modify_colors() {
   show_color_defs();
   modify_color_content( COLOR_BLUE, 0, 0, -150 );
   show_color_defs();
   }

bool ConIO::StartupOk( bool fForceNewConsole ) {
   auto lc_ctype_orig = setlocale(LC_CTYPE, nullptr);
   //
   // Setting LC_CTYPE="...UTF-8" is necessary for ncurses' setcchar and
   // mvadd_wch to correctly display UTF-8 characters.  By default LC_CTYPE="C",
   // which prevents ncurses from correctly displaying UTF-8 characters.
   //
   // Passing "" as locale/2nd param to setlocale(3) means 'set from environment
   // variables'; see `localectl` output for your active locale settings.  In my
   // case: "en_US.UTF-8".  I'm not sure "en_US" affects LC_CTYPE?
   //
   // Note that we *don't* want to call setlocale(LC_ALL,...) because this also
   // modifies LC_COLLATE which changes the sort order behavior of qsort(3).
   //
   auto lc_ctype = setlocale(LC_CTYPE, ""); // In my understanding, Linux today defaults to some UTF-8 locale, so

   use_extended_names(TRUE);
   set_escdelay(10);
   initscr();
   raw();
   if( has_colors() == FALSE ) {
      endwin();
      DBG( "Your terminal does not support color\n" );
      return false;
      }

   start_color();  DBG( "\nlc_ctype_orig=%s\nlc_ctype=%s\nTERM=%s\ncurses_version()=\"%s\"\ncan_change_color() = %d\nKEY_MIN=%d, KEY_MAX=%d\nCOLORS = %d\nCOLOR_PAIRS = %d\nCCHARW_MAX = %d", lc_ctype_orig, lc_ctype, getenv("TERM"), curses_version(), can_change_color(), KEY_MIN, KEY_MAX, COLORS, COLOR_PAIRS, CCHARW_MAX );
   show_color_defs();
   // modify_colors();
   if( !ConIn::EnsureBackendInitialized() ) {
      endwin();
      DBG( "ConIn::EnsureBackendInitialized failed" );
      return false;
      }
   MaximizeTerminal();
   ConOut::Resize();
   return true;
   }
void ConIO::Shutdown() {
   endwin();
   }
