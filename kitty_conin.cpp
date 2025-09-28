#ifndef _WIN32

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <ncurses.h>

#include "ed_main.h"
#include "conio.h"

namespace {

using std::string;
using std::string_view;

STATIC_VAR bool s_initialized;
STATIC_VAR bool s_fDbg;
STATIC_VAR int  s_decodeErrCount;
STATIC_VAR bool s_conin_blocking_read;

GLOBAL_VAR int g_iConin_nonblk_rd_tmout = 10;

void KittyLogVerbose() { s_fDbg = true; }
void KittyLogQuiet()   { s_fDbg = false; }

#define KDBG(fmt, ...) do { if( s_fDbg ) { DBG( fmt, ##__VA_ARGS__ ); } } while(0)

void KittySendSequence( const char *seq ) {
   std::fputs( seq, stdout );
   std::fflush( stdout );
   }

void KittyEnterProtocol() {
   // Request progressive enhancements only (KKP proper):
   //  0b00001 (1)  = disambiguate
   //  0b01000 (8)  = report_all_keys
   //  0b10000 (16) = report_text
   // Use mode=1 (set bits, clear others) to make the state explicit.
   KittySendSequence( "\x1b[=25;1u" );
   KDBG( "KittyEnterProtocol" );
   // Query current flags: CSI ? u -> CSI ? flags u
   KittySendSequence( "\x1b[?u" );
   int old_delay = g_iConin_nonblk_rd_tmout;
   timeout( 200 );
   std::string buf;
   // Read up to a reasonable number of bytes or until we see a trailing 'u'
   for( int i=0; i<128; ++i ) {
      int ch = getch();
      if( ch == ERR ) break;
      buf.push_back( static_cast<char>(ch) );
      if( ch == 'u' ) break;
      }
   timeout( old_delay );
   // Parse CSI ? <digits> u
   int flags = -1;
   do {
      if( buf.size() < 5 ) break;
      size_t ix = 0;
      if( static_cast<unsigned char>(buf[ix++]) != 0x1b ) break;
      if( ix >= buf.size() || buf[ix++] != '[' ) break;
      if( ix >= buf.size() || buf[ix++] != '?' ) break;
      int v=0; bool got=false;
      while( ix < buf.size() && std::isdigit( static_cast<unsigned char>(buf[ix]) ) ) { got=true; v = v*10 + (buf[ix]-'0'); ++ix; }
      if( !got ) break;
      while( ix < buf.size() && buf[ix] != 'u' ) ++ix;
      if( ix >= buf.size() || buf[ix] != 'u' ) break;
      flags = v;
      } while(false);
   if( flags >= 0 ) {
      KDBG( "Kitty flags reply=%d raw='%.*s'", flags, (int)buf.size(), buf.data() );
      }
   else {
      KDBG( "Kitty flags reply parse failed: raw='%.*s'", (int)buf.size(), buf.data() );
      }
   }

void KittyExitProtocol() {
   // Restore stacked flags (if any), then leave KKP mode
   KittySendSequence( "\x1b[<u" );
   KDBG( "KittyExitProtocol" );
   }

void conin_blocking_read() {
   timeout( -1 );
   s_conin_blocking_read = true;
   }

void conin_nonblocking_read() {
   timeout( g_iConin_nonblk_rd_tmout );
   s_conin_blocking_read = false;
}

int IncrDecodeErrCount() {
   ++s_decodeErrCount;
   DispNeedsRedrawStatLn();
   return 1;
}

struct KittyVariants {
   uint16_t base;
   uint16_t shift;
   uint16_t alt;
   uint16_t ctrl;
   uint16_t ctrlShift;
   uint16_t ctrlAlt;
};

constexpr KittyVariants make_variants(uint16_t base,
                                      uint16_t shift,
                                      uint16_t alt,
                                      uint16_t ctrl,
                                      uint16_t ctrlShift,
                                      uint16_t ctrlAlt = 0) {
   return KittyVariants{ base, shift, alt, ctrl, ctrlShift, ctrlAlt };
}

constexpr KittyVariants make_variants(uint16_t base,
                                      uint16_t shift,
                                      uint16_t alt,
                                      uint16_t ctrl) {
   return KittyVariants{ base, shift, alt, ctrl, 0, 0 };
}

constexpr KittyVariants make_variants(uint16_t base,
                                      uint16_t shift,
                                      uint16_t alt) {
   return KittyVariants{ base, shift, alt, 0, 0, 0 };
}

uint16_t apply_variants(const KittyVariants &variants, int modbits) {
   constexpr int kShift = 0x1;
   constexpr int kAlt   = 0x2;
   constexpr int kCtrl  = 0x4;

   const bool shift = (modbits & kShift) != 0;
   const bool alt   = (modbits & kAlt) != 0;
   const bool ctrl  = (modbits & kCtrl) != 0;

   if( ctrl && alt ) {
      if( variants.ctrlAlt ) { return variants.ctrlAlt; }
      if( variants.ctrl )    { return variants.ctrl; }
      if( variants.alt )     { return variants.alt; }
   }
   if( ctrl && shift ) {
      if( variants.ctrlShift ) { return variants.ctrlShift; }
      if( variants.ctrl )      { return variants.ctrl; }
   }
   if( ctrl ) {
      if( variants.ctrl ) { return variants.ctrl; }
   }
   if( alt && shift ) {
      if( variants.alt )   { return variants.alt; }
      if( variants.shift ) { return variants.shift; }
   }
   if( alt ) {
      if( variants.alt ) { return variants.alt; }
   }
   if( shift ) {
      if( variants.shift ) { return variants.shift; }
   }
   return variants.base;
}

constexpr KittyVariants tab_variants {
   static_cast<uint16_t>(EdKC_tab),
   static_cast<uint16_t>(EdKC_s_tab),
   static_cast<uint16_t>(EdKC_a_tab),
   static_cast<uint16_t>(EdKC_c_tab),
   static_cast<uint16_t>(EdKC_cs_tab)
};

constexpr KittyVariants enter_variants {
   static_cast<uint16_t>(EdKC_enter),
   static_cast<uint16_t>(EdKC_s_enter),
   static_cast<uint16_t>(EdKC_a_enter),
   static_cast<uint16_t>(EdKC_c_enter),
   static_cast<uint16_t>(EdKC_cs_enter)
};

constexpr KittyVariants bksp_variants {
   static_cast<uint16_t>(EdKC_bksp),
   static_cast<uint16_t>(EdKC_s_bksp),
   static_cast<uint16_t>(EdKC_a_bksp),
   static_cast<uint16_t>(EdKC_c_bksp),
   static_cast<uint16_t>(EdKC_cs_bksp)
};

constexpr KittyVariants esc_variants {
   static_cast<uint16_t>(EdKC_esc),
   static_cast<uint16_t>(EdKC_s_esc),
   static_cast<uint16_t>(EdKC_a_esc),
   static_cast<uint16_t>(EdKC_c_esc)
};

constexpr KittyVariants space_variants {
   static_cast<uint16_t>(' '),
   static_cast<uint16_t>(EdKC_s_space),
   static_cast<uint16_t>(EdKC_a_space),
   static_cast<uint16_t>(EdKC_c_space),
   static_cast<uint16_t>(EdKC_cs_space)
};

constexpr KittyVariants insert_variants {
   static_cast<uint16_t>(EdKC_ins),
   static_cast<uint16_t>(EdKC_s_ins),
   static_cast<uint16_t>(EdKC_a_ins),
   static_cast<uint16_t>(EdKC_c_ins),
   static_cast<uint16_t>(EdKC_cs_ins)
};

constexpr KittyVariants delete_variants {
   static_cast<uint16_t>(EdKC_del),
   static_cast<uint16_t>(EdKC_s_del),
   static_cast<uint16_t>(EdKC_a_del),
   static_cast<uint16_t>(EdKC_c_del),
   static_cast<uint16_t>(EdKC_cs_del)
};

constexpr KittyVariants home_variants {
   static_cast<uint16_t>(EdKC_home),
   static_cast<uint16_t>(EdKC_s_home),
   static_cast<uint16_t>(EdKC_a_home),
   static_cast<uint16_t>(EdKC_c_home),
   static_cast<uint16_t>(EdKC_cs_home)
};

constexpr KittyVariants end_variants {
   static_cast<uint16_t>(EdKC_end),
   static_cast<uint16_t>(EdKC_s_end),
   static_cast<uint16_t>(EdKC_a_end),
   static_cast<uint16_t>(EdKC_c_end),
   static_cast<uint16_t>(EdKC_cs_end)
};

constexpr KittyVariants pgup_variants {
   static_cast<uint16_t>(EdKC_pgup),
   static_cast<uint16_t>(EdKC_s_pgup),
   static_cast<uint16_t>(EdKC_a_pgup),
   static_cast<uint16_t>(EdKC_c_pgup),
   static_cast<uint16_t>(EdKC_cs_pgup)
};

constexpr KittyVariants pgdn_variants {
   static_cast<uint16_t>(EdKC_pgdn),
   static_cast<uint16_t>(EdKC_s_pgdn),
   static_cast<uint16_t>(EdKC_a_pgdn),
   static_cast<uint16_t>(EdKC_c_pgdn),
   static_cast<uint16_t>(EdKC_cs_pgdn)
};

constexpr KittyVariants center_variants {
   static_cast<uint16_t>(EdKC_center),
   static_cast<uint16_t>(EdKC_s_center),
   static_cast<uint16_t>(EdKC_a_center),
   static_cast<uint16_t>(EdKC_c_center),
   static_cast<uint16_t>(EdKC_cs_center)
};

constexpr KittyVariants arrow_left_variants {
   static_cast<uint16_t>(EdKC_left),
   static_cast<uint16_t>(EdKC_s_left),
   static_cast<uint16_t>(EdKC_a_left),
   static_cast<uint16_t>(EdKC_c_left),
   static_cast<uint16_t>(EdKC_cs_left)
};

constexpr KittyVariants arrow_right_variants {
   static_cast<uint16_t>(EdKC_right),
   static_cast<uint16_t>(EdKC_s_right),
   static_cast<uint16_t>(EdKC_a_right),
   static_cast<uint16_t>(EdKC_c_right),
   static_cast<uint16_t>(EdKC_cs_right)
};

constexpr KittyVariants arrow_up_variants {
   static_cast<uint16_t>(EdKC_up),
   static_cast<uint16_t>(EdKC_s_up),
   static_cast<uint16_t>(EdKC_a_up),
   static_cast<uint16_t>(EdKC_c_up),
   static_cast<uint16_t>(EdKC_cs_up)
};

constexpr KittyVariants arrow_down_variants {
   static_cast<uint16_t>(EdKC_down),
   static_cast<uint16_t>(EdKC_s_down),
   static_cast<uint16_t>(EdKC_a_down),
   static_cast<uint16_t>(EdKC_c_down),
   static_cast<uint16_t>(EdKC_cs_down)
};

constexpr KittyVariants kp_enter_variants {
   static_cast<uint16_t>(EdKC_numEnter),
   static_cast<uint16_t>(EdKC_s_numEnter),
   static_cast<uint16_t>(EdKC_a_numEnter),
   static_cast<uint16_t>(EdKC_c_numEnter),
   static_cast<uint16_t>(EdKC_cs_numEnter)
};

constexpr KittyVariants kp_minus_variants {
   static_cast<uint16_t>(EdKC_numMinus),
   static_cast<uint16_t>(EdKC_s_numMinus),
   static_cast<uint16_t>(EdKC_a_numMinus),
   static_cast<uint16_t>(EdKC_c_numMinus),
   static_cast<uint16_t>(EdKC_cs_numMinus)
};

constexpr KittyVariants kp_plus_variants {
   static_cast<uint16_t>(EdKC_numPlus),
   static_cast<uint16_t>(EdKC_s_numPlus),
   static_cast<uint16_t>(EdKC_a_numPlus),
   static_cast<uint16_t>(EdKC_c_numPlus),
   static_cast<uint16_t>(EdKC_cs_numPlus)
};

constexpr KittyVariants kp_star_variants {
   static_cast<uint16_t>(EdKC_numStar),
   static_cast<uint16_t>(EdKC_s_numStar),
   static_cast<uint16_t>(EdKC_a_numStar),
   static_cast<uint16_t>(EdKC_c_numStar),
   static_cast<uint16_t>(EdKC_cs_numStar)
};

constexpr KittyVariants kp_slash_variants {
   static_cast<uint16_t>(EdKC_numSlash),
   static_cast<uint16_t>(EdKC_s_numSlash),
   static_cast<uint16_t>(EdKC_a_numSlash),
   static_cast<uint16_t>(EdKC_c_numSlash),
   static_cast<uint16_t>(EdKC_cs_numSlash)
};

constexpr KittyVariants num_variants(int digit) {
   return KittyVariants {
      static_cast<uint16_t>(EdKC_num0 + digit),
      static_cast<uint16_t>(EdKC_s_num0 + digit),
      static_cast<uint16_t>(EdKC_a_num0 + digit),
      static_cast<uint16_t>(EdKC_c_num0 + digit),
      static_cast<uint16_t>(EdKC_cs_num0 + digit)
   };
}

constexpr KittyVariants fkey_variants(int index) {
   return KittyVariants {
      static_cast<uint16_t>(EdKC_f1 + index - 1),
      static_cast<uint16_t>(EdKC_s_f1 + index - 1),
      static_cast<uint16_t>(EdKC_a_f1 + index - 1),
      static_cast<uint16_t>(EdKC_c_f1 + index - 1),
      static_cast<uint16_t>(EdKC_cs_f1 + index - 1),
      static_cast<uint16_t>(EdKC_ca_f1 + index - 1)
   };
}

int map_ascii_alt(int ch) {
   switch( ch ) {
      case 127: return static_cast<uint16_t>(EdKC_a_bksp);
      case '\r':
      case '\n': return static_cast<uint16_t>(EdKC_a_enter);
      case '\t': return static_cast<uint16_t>(EdKC_a_tab);
      case '\\': return static_cast<uint16_t>(EdKC_a_BACKSLASH);
      case ',':  return static_cast<uint16_t>(EdKC_a_COMMA);
      case '-':  return static_cast<uint16_t>(EdKC_a_MINUS);
      case '.':  return static_cast<uint16_t>(EdKC_a_DOT);
      case '/':  return static_cast<uint16_t>(EdKC_a_SLASH);
      case ';':  return static_cast<uint16_t>(EdKC_a_SEMICOLON);
      case '=':  return static_cast<uint16_t>(EdKC_a_EQUAL);
      case '[':  return static_cast<uint16_t>(EdKC_a_LEFT_SQ);
      case ']':  return static_cast<uint16_t>(EdKC_a_RIGHT_SQ);
      case '`':  return static_cast<uint16_t>(EdKC_a_BACKTICK);
      case '\'': return static_cast<uint16_t>(EdKC_a_TICK);
      case ' ':  return static_cast<uint16_t>(EdKC_a_space);
      }
   if( ch >= '0' && ch <= '9' ) { return static_cast<uint16_t>(EdKC_a_0 + (ch - '0')); }
   if( ch >= 'a' && ch <= 'z' ) { return static_cast<uint16_t>(EdKC_a_a + (ch - 'a')); }
   if( ch >= 'A' && ch <= 'Z' ) { return static_cast<uint16_t>(EdKC_a_a + (ch - 'A')); }
   return 0;
   }

int map_ascii_ctrl(int ch) {
   if( ch >= '0' && ch <= '9' ) { return static_cast<uint16_t>(EdKC_c_0 + (ch - '0')); }
   if( ch >= 'a' && ch <= 'z' ) { return static_cast<uint16_t>(EdKC_c_a + (ch - 'a')); }
   if( ch >= 'A' && ch <= 'Z' ) { return static_cast<uint16_t>(EdKC_c_a + (ch - 'A')); }
   switch( ch ) {
      case ' ': return static_cast<uint16_t>(EdKC_c_space);
      case '\t': return static_cast<uint16_t>(EdKC_c_tab);
      case '\r':
      case '\n': return static_cast<uint16_t>(EdKC_c_enter);
      case '[': return static_cast<uint16_t>(EdKC_c_LEFT_SQ);
      case ']': return static_cast<uint16_t>(EdKC_c_RIGHT_SQ);
      case '\\': return static_cast<uint16_t>(EdKC_c_BACKSLASH);
      case ';': return static_cast<uint16_t>(EdKC_c_SEMICOLON);
      case '=': return static_cast<uint16_t>(EdKC_c_EQUAL);
      case '-': return static_cast<uint16_t>(EdKC_c_MINUS);
      case '.': return static_cast<uint16_t>(EdKC_c_DOT);
      case '/': return static_cast<uint16_t>(EdKC_c_SLASH);
      }
   return 0;
   }

int map_ascii_ctrl_shift(int ch) {
   if( ch >= '0' && ch <= '9' ) { return static_cast<uint16_t>(EdKC_cs_0 + (ch - '0')); }
   if( ch >= 'a' && ch <= 'z' ) { return static_cast<uint16_t>(EdKC_cs_a + (ch - 'a')); }
   if( ch >= 'A' && ch <= 'Z' ) { return static_cast<uint16_t>(EdKC_cs_a + (ch - 'A')); }
   switch( ch ) {
      case ' ': return static_cast<uint16_t>(EdKC_cs_space);
      case '\t': return static_cast<uint16_t>(EdKC_cs_tab);
      case '\r':
      case '\n': return static_cast<uint16_t>(EdKC_cs_enter);
      case '-': return static_cast<uint16_t>(EdKC_cs_MINUS);
      case '=': return static_cast<uint16_t>(EdKC_cs_EQUAL);
      case '[': return static_cast<uint16_t>(EdKC_cs_LEFT_SQ);
      case ']': return static_cast<uint16_t>(EdKC_cs_RIGHT_SQ);
      case '\\': return static_cast<uint16_t>(EdKC_cs_BACKSLASH);
      case ';': return static_cast<uint16_t>(EdKC_cs_SEMICOLON);
      case ',': return static_cast<uint16_t>(EdKC_cs_COMMA);
      case '.': return static_cast<uint16_t>(EdKC_cs_DOT);
      case '/': return static_cast<uint16_t>(EdKC_cs_SLASH);
      case '`': return static_cast<uint16_t>(EdKC_cs_BACKTICK);
      case '\'': return static_cast<uint16_t>(EdKC_cs_TICK);
      }
   return 0;
   }

int map_ascii_from_csi(int ch, int modbits) {
   constexpr int kShift = 0x1;
   constexpr int kAlt   = 0x2;
   constexpr int kCtrl  = 0x4;

   const bool shift = (modbits & kShift) != 0;
   const bool alt   = (modbits & kAlt) != 0;
   const bool ctrl  = (modbits & kCtrl) != 0;

   if( ctrl && shift ) {
      if( const auto v = map_ascii_ctrl_shift( ch ); v ) { return v; }
      }
   if( ctrl ) {
      if( const auto v = map_ascii_ctrl( ch ); v ) { return v; }
      }
   if( alt ) {
      if( const auto v = map_ascii_alt( ch ); v ) { return v; }
      }
   if( shift ) {
      if( ch >= 'a' && ch <= 'z' ) { return static_cast<uint16_t>( std::toupper( static_cast<unsigned char>( ch ) ) ); }
      if( ch >= '0' && ch <= '9' ) { return ch; }
      }
   return ch;
   }

int map_csiu_key(int keyCode, int modbits) {
   KDBG( "Kitty map_csiu key=%d modbits=0x%x", keyCode, modbits );
   // Ignore pure modifier keys and locks: they are reported separately when
   // report_all_keys is enabled, but the editor does not bind them directly.
   if( (keyCode >= 57441 && keyCode <= 57446)   // LEFT_SHIFT..LEFT_META
    || (keyCode >= 57447 && keyCode <= 57452)   // RIGHT_SHIFT..RIGHT_META
    || (keyCode >= 57358 && keyCode <= 57360)   // CAPS_LOCK..NUM_LOCK
     ) {
      return -1; // not an error; just ignore as no-op event
      }
   switch( keyCode ) {
      break;case 27:  return apply_variants( esc_variants, modbits );
      break;case 13:  return apply_variants( enter_variants, modbits );
      break;case 9:   return apply_variants( tab_variants, modbits );
      break;case 127: return apply_variants( bksp_variants, modbits );
      break;case 32:  return apply_variants( space_variants, modbits );
      break;case 57399: return apply_variants( num_variants(0), modbits );
      break;case 57400: return apply_variants( num_variants(1), modbits );
      break;case 57401: return apply_variants( num_variants(2), modbits );
      break;case 57402: return apply_variants( num_variants(3), modbits );
      break;case 57403: return apply_variants( num_variants(4), modbits );
      break;case 57404: return apply_variants( num_variants(5), modbits );
      break;case 57405: return apply_variants( num_variants(6), modbits );
      break;case 57406: return apply_variants( num_variants(7), modbits );
      break;case 57407: return apply_variants( num_variants(8), modbits );
      break;case 57408: return apply_variants( num_variants(9), modbits );
      break;case 57409: return map_ascii_from_csi( '.', modbits );
      break;case 57410: return apply_variants( kp_slash_variants, modbits );
      break;case 57411: return apply_variants( kp_star_variants, modbits );
      break;case 57412: return apply_variants( kp_minus_variants, modbits );
      break;case 57413: return apply_variants( kp_plus_variants, modbits );
      break;case 57414: return apply_variants( kp_enter_variants, modbits );
      break;case 57415: return map_ascii_from_csi( '=', modbits );
      break;case 57416: return map_ascii_from_csi( ',', modbits );
      break;case 57417: return apply_variants( arrow_left_variants, modbits );
      break;case 57418: return apply_variants( arrow_right_variants, modbits );
      break;case 57419: return apply_variants( arrow_up_variants, modbits );
      break;case 57420: return apply_variants( arrow_down_variants, modbits );
      break;case 57425: return apply_variants( insert_variants, modbits );
      break;case 57426: return apply_variants( delete_variants, modbits );
      break;case 57421: return apply_variants( pgup_variants, modbits );
      break;case 57422: return apply_variants( pgdn_variants, modbits );
      break;case 57423: return apply_variants( home_variants, modbits );
      break;case 57424: return apply_variants( end_variants, modbits );
      break;case 57427: return apply_variants( center_variants, modbits );
      }
   if( keyCode >= 32 && keyCode <= 126 ) {
      return map_ascii_from_csi( keyCode, modbits );
      }
   return 0;
   }

int map_csitilde_key(int keyCode, int modbits) {
   switch( keyCode ) {
      break;case 2:  return apply_variants( insert_variants, modbits );
      break;case 3:  return apply_variants( delete_variants, modbits );
      break;case 5:  return apply_variants( pgup_variants, modbits );
      break;case 6:  return apply_variants( pgdn_variants, modbits );
      break;case 7:  return apply_variants( home_variants, modbits );
      break;case 8:  return apply_variants( end_variants, modbits );
      break;case 11: return apply_variants( fkey_variants(1), modbits );
      break;case 12: return apply_variants( fkey_variants(2), modbits );
      break;case 13: return apply_variants( fkey_variants(3), modbits );
      break;case 14: return apply_variants( fkey_variants(4), modbits );
      break;case 15: return apply_variants( fkey_variants(5), modbits );
      break;case 17: return apply_variants( fkey_variants(6), modbits );
      break;case 18: return apply_variants( fkey_variants(7), modbits );
      break;case 19: return apply_variants( fkey_variants(8), modbits );
      break;case 20: return apply_variants( fkey_variants(9), modbits );
      break;case 21: return apply_variants( fkey_variants(10), modbits );
      break;case 23: return apply_variants( fkey_variants(11), modbits );
      break;case 24: return apply_variants( fkey_variants(12), modbits );
      break;case 57427: return apply_variants( center_variants, modbits );
      }
   return 0;
   }

int map_csi_letter(int keyParam, char final, int modbits) {
   (void)keyParam;
   switch( final ) {
      break;case 'A': return apply_variants( arrow_up_variants, modbits );
      break;case 'B': return apply_variants( arrow_down_variants, modbits );
      break;case 'C': return apply_variants( arrow_right_variants, modbits );
      break;case 'D': return apply_variants( arrow_left_variants, modbits );
      break;case 'F': return apply_variants( end_variants, modbits );
      break;case 'H': return apply_variants( home_variants, modbits );
      break;case 'M': return apply_variants( enter_variants, modbits );
      break;case 'P': return apply_variants( fkey_variants(1), modbits );
      break;case 'Q': return apply_variants( fkey_variants(2), modbits );
      break;case 'R': return apply_variants( fkey_variants(3), modbits );
      break;case 'S': return apply_variants( fkey_variants(4), modbits );
      break;case 'E': return apply_variants( center_variants, modbits );
      }
   return 0;
   }

std::vector<std::string_view> split_params( std::string_view params ) {
   std::vector<std::string_view> out;
   size_t start = 0;
   for( size_t ix = 0; ix <= params.size(); ++ix ) {
      if( ix == params.size() || params[ix] == ';' ) {
         out.emplace_back( params.substr( start, ix - start ) );
         start = ix + 1;
         }
      }
   return out;
   }

int parse_int( std::string_view sv, int dflt ) {
   if( sv.empty() ) { return dflt; }
   int sign = 1;
   size_t pos = 0;
   if( sv[0] == '+' || sv[0] == '-' ) {
      if( sv[0] == '-' ) { sign = -1; }
      ++pos;
      }
   int value = 0;
   for( ; pos < sv.size(); ++pos ) {
      if( !std::isdigit( static_cast<unsigned char>( sv[pos] ) ) ) { break; }
      value = value * 10 + (sv[pos] - '0');
      }
   return value * sign;
   }

int map_csi_sequence( std::string_view params, char final ) {
   const auto tokens = split_params( params );
   const auto get_token = [&]( size_t index ) -> std::string_view {
      return index < tokens.size() ? tokens[index] : std::string_view{};
      };

   int keyCode = 0;
   if( final == 'A' || final == 'B' || final == 'C' || final == 'D'
    || final == 'E' || final == 'F' || final == 'P' || final == 'Q'
    || final == 'R' || final == 'S' || final == 'M' || final == 'H' ) {
      keyCode = parse_int( get_token(0), 1 );
      }
   else {
      keyCode = parse_int( get_token(0), 0 );
      }

   int modifierField = 1;
   if( final == 'u' || final == '~' || tokens.size() > 1 ) {
      if( tokens.size() > 1 ) {
         std::string_view modField = get_token(1);
         auto colonPos = modField.find(':');
         if( colonPos != std::string_view::npos ) {
            modField = modField.substr( 0, colonPos ); // strip any event-type or text subfields
            }
         if( !modField.empty() ) {
            modifierField = parse_int( modField, 1 );
            }
         }
      }

   int modbits = (modifierField > 0) ? (modifierField - 1) : 0;

   int rv = 0;
   switch( final ) {
      break;case 'u': rv = map_csiu_key( keyCode, modbits ); break;
      break;case '~': rv = map_csitilde_key( keyCode, modbits ); break;
      default:        rv = map_csi_letter( keyCode, final, modbits ); break;
      }

   if( rv == 0 ) {
      IncrDecodeErrCount();
      KDBG( "Kitty decode error params='%.*s' final='%c'", static_cast<int>(params.size()), params.data(), final );
      return -1;
      }
   KDBG( "Kitty CSI params='%.*s' final='%c' -> %d", static_cast<int>(params.size()), params.data(), final, rv );
   return rv;
}

int KittyDecodeCSI() {
   std::string collected;
   char final = 0;
   while( true ) {
      int ch = getch();
      if( ch == ERR ) { return -1; }
      if( ch >= 0x40 && ch <= 0x7e ) {
         final = static_cast<char>( ch );
         break;
         }
      collected.push_back( static_cast<char>( ch ) );
      }
   return map_csi_sequence( collected, final );
   }

int map_7bit_alt_sequence( int ch ) {
   switch( ch ) {
      break;case 127 : return static_cast<uint16_t>( EdKC_a_bksp );
      break;case '\r':
      break;case '\n': return static_cast<uint16_t>( EdKC_a_enter );
      break;case '\t': return static_cast<uint16_t>( EdKC_a_tab );
      break;case '\'': return static_cast<uint16_t>( EdKC_a_TICK );
      break;case '\\': return static_cast<uint16_t>( EdKC_a_BACKSLASH );
      break;case ',':   return static_cast<uint16_t>( EdKC_a_COMMA );
      break;case '-':   return static_cast<uint16_t>( EdKC_a_MINUS );
      break;case '.':   return static_cast<uint16_t>( EdKC_a_DOT );
      break;case '/':   return static_cast<uint16_t>( EdKC_a_SLASH );
      break;case ';':   return static_cast<uint16_t>( EdKC_a_SEMICOLON );
      break;case '=':   return static_cast<uint16_t>( EdKC_a_EQUAL );
      break;case '[':   return static_cast<uint16_t>( EdKC_a_LEFT_SQ );
      break;case ']':   return static_cast<uint16_t>( EdKC_a_RIGHT_SQ );
      break;case '`':   return static_cast<uint16_t>( EdKC_a_BACKTICK );
      }
   if( ch >= '0' && ch <= '9' ) { return static_cast<uint16_t>(EdKC_a_0 + (ch - '0')); }
   if( ch >= 'a' && ch <= 'z' ) { return static_cast<uint16_t>(EdKC_a_a + (ch - 'a')); }
   if( ch >= 'A' && ch <= 'Z' ) { return static_cast<uint16_t>(EdKC_a_a + (ch - 'A')); }
   return 0;
   }

int KittyMapPlainChar( int ch ) {
   if( ch == '\r' || ch == '\n' ) { return static_cast<uint16_t>( EdKC_enter ); }
   if( ch == '\t' ) { return static_cast<uint16_t>( EdKC_tab ); }
   if( ch == 0 ) { return -1; }
   if( ch >= 28 && ch <= 31 ) {
      auto rv = static_cast<uint16_t>( EdKC_c_4 + (ch - 28) );
      return rv;
      }
   if( ch < 27 ) {
      auto rv = static_cast<uint16_t>( EdKC_c_a + (ch - 1) );
      return rv;
      }
   return ch;
   }

int KittyGetEvent() {
   int ch = getch();
   if( ch == ERR ) { return -1; }

    KDBG( "KittyGetEvent first byte=0x%02x", ch );

   if( ch == 0x9b ) { // 8-bit CSI
      return KittyDecodeCSI();
      }

   if( ch == 0x1b ) {
      int next = getch();
      KDBG( "Kitty ESC next=0x%02x", next );
      if( next == ERR ) {
         return static_cast<uint16_t>( EdKC_esc );
         }
      if( next == '[' ) {
         return KittyDecodeCSI();
         }
      // No SS3 fallback in KKP mode: rely on CSI u
      if( next == 0x1b ) {
         return static_cast<uint16_t>( EdKC_esc );
         }
      if( const auto altMap = map_7bit_alt_sequence( next ); altMap ) {
         KDBG( "Kitty ALT map %d", altMap );
         return altMap;
         }
      const auto mapped = KittyMapPlainChar( next );
      KDBG( "Kitty ESC plain mapped=%d", mapped );
      return mapped;
      }

   const auto mapped = KittyMapPlainChar( ch );
   KDBG( "Kitty plain mapped=%d", mapped );
   return mapped;
   }

EdKC_Ascii GetEdKC_Ascii( bool freezeOtherThreads ) {
   while( true ) {
      const bool passLock = !freezeOtherThreads;
      if( passLock ) { MainThreadGiveUpGlobalVariableLock(); }
      const auto ev = KittyGetEvent();
      if( passLock ) { MainThreadWaitForGlobalVariableLock(); }
      if( ev >= 0 ) {
         if( ev < EdKC_COUNT ) {
            if( ev == 0 ) {
               continue;
               }
            EdKC_Ascii rv;
            rv.Ascii    = (ev == EdKC_tab) ? '\t' : static_cast<char>(ev & 0xFF);
            rv.EdKcEnum = ModalKeyRemap( static_cast<EdKc_t>( ev ) );
            KDBG( "Kitty decoded ev=%d mapped=%d ascii=0x%02x", ev, rv.EdKcEnum, static_cast<unsigned char>( rv.Ascii ) );
            return rv;
            }
         CleanupAnyExecutionHaltRequest();
         }
      }
   }

bool KittyFlushKeyQueueAnythingFlushed() {
   return flushinp();
   }

bool KittyKbHit() {
   if( s_conin_blocking_read ) {
      conin_nonblocking_read();
      const int ch = getch();
      if( ch != ERR ) {
         ungetch( ch );
         conin_blocking_read();
         return true;
         }
      conin_blocking_read();
      return false;
      }
   const int ch = getch();
   if( ch != ERR ) {
      ungetch( ch );
      return true;
      }
   return false;
   }

bool KittyInitialize() {
   if( s_initialized ) { return true; }
   if( std::getenv("KITTY_WINDOW_ID") == nullptr ) {
      Msg( "Kitty backend unavailable: KITTY_WINDOW_ID not set" );
      return false;
      }
   noecho();
   nonl();
   keypad( stdscr, FALSE );
   meta( stdscr, TRUE );
   conin_blocking_read();
   KittyEnterProtocol();
   s_initialized = true;
   return true;
   }

void KittyShutdown() {
   if( s_initialized ) {
      KittyExitProtocol();
      s_initialized = false;
      }
   }

} // namespace

namespace {

int KittyDecodeErrCount() { return s_decodeErrCount; }

bool KittyBackendFlush() { return KittyFlushKeyQueueAnythingFlushed(); }

void KittyBackendWaitForKey() { (void)GetEdKC_Ascii( true ); }

EdKC_Ascii KittyBackendNextKey() { return GetEdKC_Ascii( false ); }

EdKC_Ascii KittyBackendNextKeyStr( std::string &dest ) {
   const auto rv = GetEdKC_Ascii( false );
   dest.assign( KeyNmOfEdkc( rv.EdKcEnum ) );
   return rv;
   }

const ConIn::BackendOps s_kittyBackendOps {
   .Initialize                         = KittyInitialize,
   .Shutdown                           = KittyShutdown,
   .log_verbose                        = KittyLogVerbose,
   .log_quiet                          = KittyLogQuiet,
   .DecodeErrCount                     = KittyDecodeErrCount,
   .FlushKeyQueueAnythingFlushed       = KittyBackendFlush,
   .WaitForKey                         = KittyBackendWaitForKey,
   .EdKC_Ascii_FromNextKey             = KittyBackendNextKey,
   .EdKC_Ascii_FromNextKey_Keystr      = KittyBackendNextKeyStr,
   .KbHit                              = KittyKbHit
   };

struct KittyBackendRegistration {
   KittyBackendRegistration() {
      ConIn::RegisterBackend( ConIn::BackendId::Kitty, s_kittyBackendOps );
      if( std::getenv( "KITTY_WINDOW_ID" ) ) {
         ConIn::SelectBackend( ConIn::BackendId::Kitty );
         }
      }
   };

STATIC_VAR KittyBackendRegistration s_registerKittyBackend;

} // namespace

#endif // !_WIN32
