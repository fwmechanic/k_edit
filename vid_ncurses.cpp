

#include <vector>
#include <ncurses.h>


#define PC_BLACK   0
#define PC_BLUE    1
#define PC_GREEN   2
#define PC_CYAN    3
#define PC_RED     4
#define PC_PURPLE  5
#define PC_YELLOW  6
#define PC_WHITE   7

static const int pc_to_ncurses_color[] = {
    [PC_BLACK ] = COLOR_BLACK  ,
    [PC_BLUE  ] = COLOR_BLUE   ,
    [PC_GREEN ] = COLOR_GREEN  ,
    [PC_CYAN  ] = COLOR_CYAN   ,
    [PC_RED   ] = COLOR_RED    ,
    [PC_PURPLE] = COLOR_PURPLE ,
    [PC_YELLOW] = COLOR_YELLOW ,
    [PC_WHITE ] = COLOR_WHITE  ,
    };


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
   for( auto it( s_color_map.cbegin() ; it != s_color_map.cend() ; ++it ) {
      if( *it == attr ) {
         color_pr_num( 1+ std::distance( s_color_map.cbegin(), it ) );
         break;
         }
      }
   if( 0 == color_pr_num ) {
      s_color_map.emplace_back( attr );
      const auto pcfg(  attr       & 0x7 ); const auto ncfg( pc_to_ncurses_color[pcfg] );
      const auto pcbg( (attr >> 4) & 0x7 ); const auto ncbg( pc_to_ncurses_color[pcbg] );
      color_pr_num = 1+ (s_color_map.size() - 1);
      init_pair( rv, ncfg, ncbg );
      }
   }
   attr_set( COLOR_PAIR(color_pr_num) | ncfg_bold );
   }
void Video::SetCursorSize( bool fBigCursor ) {}
static Point s_cursor_pos;
bool Video::GetCursorState( Point *pt, bool *pfVisible ) {
   *pt = s_cursor_pos;
   *pfVisible = true;
   }
bool Video::SetCursorVisibilityChanged( bool fVisible ) { return false; }
void Video::SetCursorLocn( LINE yLine, COL xCol ) {
   s_cursor_pos.lin = yLine;
   s_cursor_pos.col = xCol;
   }
COL Video::BufferWriteString( PCChar pszStringToDisp, COL StringLen, LINE yLineWithinConsoleWindow, int xColWithinConsoleWindow, int colorAttribute, bool fPadWSpcs ) {
   set_pcattr( colorAttribute );
   int sizeY, sizeX;  getmaxyx( stdscr, sizeY, sizeX );
   if( xColWithinConsoleWindow >= sizeX ) { return 0; }
   int slen = StringLen;
   { PCChar pNul = memchr( pszStringToDisp, 0, StringLen );
     if( pNul ) { slen = pNul - pszStringToDisp; }
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
bool Video::SetScreenSizeOk( Point &newSize ) { return false; }
void Video::GetScreenSize( PPoint rv ) {
   getmaxyx( stdscr, rv->lin, rv->col );
   }
Point Video::GetMaxConsoleSize() {
   Point rv;
   Video::GetScreenSize( &rv );
   return rv;
   }
void Video::BufferFlushToScreen() {
   move( s_cursor_pos.lin, s_cursor_pos.col );
   refresh();
   }
bool Video::WriteToFileOk( FILE *ofh ) {
   return false;
   }
bool Video::SetConsolePalette( const unsigned palette[16] ) { return false; }
void Video::Startup( bool fForceNewConsole ) {
   initscr();
   if(has_colors() == FALSE) {
      endwin();
      printf("Your terminal does not support color\n");
      exit(1);
      }

   start_color();
   keypad(stdscr, TRUE);
   noecho();
   }
