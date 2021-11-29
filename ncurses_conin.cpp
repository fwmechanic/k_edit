//
// Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include <functional>
#include <ncurses.h>
#include <stdio.h>
#include "ed_main.h"
#include "conio.h"


STATIC_FXN void terminfo_ch( PChar &dest, size_t &sizeofDest, int ch ) {
   switch( ch ) {
      break;case '\200' : snprintf_full( &dest, &sizeofDest, "\\0"  );
      break;case '\\'   : snprintf_full( &dest, &sizeofDest, "\\\\" );
      break;case 27     : snprintf_full( &dest, &sizeofDest, "\\E"  );
      break;case '^'    : snprintf_full( &dest, &sizeofDest, "\\^"  );
      break;case ','    : snprintf_full( &dest, &sizeofDest, "\\,"  );
      break;case ':'    : snprintf_full( &dest, &sizeofDest, "\\:"  );
      break;case '\n'   : snprintf_full( &dest, &sizeofDest, "\\n"  );
      break;case '\r'   : snprintf_full( &dest, &sizeofDest, "\\r"  );
      break;case '\t'   : snprintf_full( &dest, &sizeofDest, "\\t"  );
      break;case '\b'   : snprintf_full( &dest, &sizeofDest, "\\b"  );
      break;case '\f'   : snprintf_full( &dest, &sizeofDest, "\\f"  );
      break;case ' '    : snprintf_full( &dest, &sizeofDest, "\\s"  );
      break;default     : if( ch >= 1 && ch <= 26 ) { snprintf_full( &dest, &sizeofDest, "^%c", ch-1+'A' ); }
                          else                      { snprintf_full( &dest, &sizeofDest, isprint( ch ) ? "%c" : "\\%03o", ch ); }
      }
   }

STATIC_FXN PChar terminfo_str( PChar &dest, size_t &sizeofDest, const int *ach, int numCh ) {
   for( auto ix(0) ; ix < numCh ; ++ix ) {
      terminfo_ch( dest, sizeofDest, ach[ ix ] );
      }
   return dest;
   }

STATIC_FXN PChar terminfo_str( PChar &dest, size_t &sizeofDest, PCChar ach, int numCh ) {
   for( auto ix(0) ; ix < numCh ; ++ix ) {
      terminfo_ch( dest, sizeofDest, ach[ ix ] );
      }
   return dest;
   }

// http://emacswiki.org/emacs/PuTTY  seems a goldmine for the enigma that is putty
//
// from http://invisible-island.net/xterm/terminfo.html  "terminfo for XTERM"
//
// Some names are extensions allowed by ncurses, e.g.,
//       kDN, kDN5, kDN6, kLFT5, kLFT6, kRIT5, kRIT6, kUP, kUP5, kUP6
//
// The numbers correspond to the modifier parameters documented in Xterm
// Control Sequences:
//
//       2       Shift
//       3       Alt
//       4       Shift + Alt
//       5       Control
//       6       Shift + Control
//       7       Alt + Control
//       8       Shift + Alt + Control
//
STATIC_VAR uint16_t ncurses_ch_to_EdKC[600]; // indexed by ncurses_ch

STATIC_FXN void cap_nm_to_ncurses_ch( const char *cap_nm, uint16_t edkc ) { enum { DB=0 };
   /* if terminfo defines a capability named cap_nm (as an escseqstr), _and_ ncurses maps that escseqstr
      to an ncurses_ch (which getch() returns), then add an entry ncurses_ch_to_EdKC[ncurses_ch] = EdKC #
      where EdKC is the EDITOR KEYCODE which corresponds to cap_nm

      cap_nm is a terminfo "capability name", the key of a key=value mapping defined in a terminfo file
      see http://invisible-island.net/xterm/terminfo-contents.html  ; EX:
      xterm+pce3|fragment with modifyCursorKeys:3,
              kDC=\E[3;2~,                               key=cap_nm=kDC, value=escseqstr=\E[>3;2~
   */
   auto keyNm( KeyNmOfEdkc( edkc ) );            DB && DBG( "tigetstr+ %s", cap_nm );
   const char *escseqstr( tigetstr( cap_nm ) );  DB && DBG( "tigetstr- %s", cap_nm ); // tigetstr() <- "retrieves a capability from the terminfo database"
   if( !escseqstr || (long)(escseqstr) == -1 ) {
#define KEYMAPFMT  "0%04o=0d%d <= %-5s => %-8s => %s"
      0 && DBG( KEYMAPFMT, 0, 0, cap_nm, "", keyNm.c_str() );
      return;
      }
   char tib[65]; auto pob( tib ); auto nob( sizeof( tib ) ); terminfo_str( pob, nob, escseqstr, Strlen(escseqstr) );
                                                    DB && DBG( "key_defined+ %s", tib );
   const auto ncurses_ch( key_defined(escseqstr) ); DB && DBG( "key_defined- %s", tib ); // key_defined() <- ncurses
   DBG( KEYMAPFMT, ncurses_ch, ncurses_ch, cap_nm, tib, keyNm.c_str() );
   if( ncurses_ch > 0 ) {
      if( ncurses_ch < ELEMENTS( ncurses_ch_to_EdKC ) ) {
         if( ncurses_ch_to_EdKC[ ncurses_ch ] != edkc ) {
            if( ncurses_ch_to_EdKC[ ncurses_ch ] ) {
               keyNm = KeyNmOfEdkc( ncurses_ch_to_EdKC[ ncurses_ch ] );
               DBG( "0%04o=0d%d EdKC=%s overridden!", ncurses_ch, ncurses_ch, keyNm.c_str() );
               }
            ncurses_ch_to_EdKC[ ncurses_ch ] = edkc;
            }
         }
      else {
         Msg( "INTERNAL ERROR: ncurses_ch=%d out of range of ncurses_ch_to_EdKC[]!", ncurses_ch );
         }
      }
   }

STATIC_VAR bool s_keypad_mode;
STATIC_FXN void keypad_mode_disable() { keypad(stdscr, 0); s_keypad_mode = false; }
STATIC_FXN void keypad_mode_enable()  { keypad(stdscr, 1); s_keypad_mode = true ; }

STATIC_VAR bool s_conin_blocking_read;
GLOBAL_VAR int  g_iConin_nonblk_rd_tmout = 10;
STATIC_FXN void conin_nonblocking_read() { timeout(g_iConin_nonblk_rd_tmout); s_conin_blocking_read = false; } // getCh blocks for (10) milliseconds, returns ERR if there is still no input
STATIC_FXN void conin_blocking_read()    { timeout(  -1                    ); s_conin_blocking_read = true;  } // getCh blocks waiting for next char

void conin_ncurses_init() {  // this MIGHT need to be made $TERM-specific
   noecho();              // we do not change
   nonl();                // we do not change
   conin_blocking_read();
   keypad_mode_enable();
   meta(stdscr, 1);       // we do not change

   STATIC_VAR const struct { short nckc; uint16_t edkc; } s_nckc2edkc[] = {
      {KEY_RIGHT     , EdKC_right }, {KEY_SRIGHT    , EdKC_s_right },
      {KEY_LEFT      , EdKC_left  }, {KEY_SLEFT     , EdKC_s_left  },
      {KEY_DC        , EdKC_del   }, {KEY_SDC       , EdKC_s_del   },
      {KEY_IC        , EdKC_ins   }, {KEY_SIC       , EdKC_s_ins   },
      {KEY_HOME      , EdKC_home  }, {KEY_SHOME     , EdKC_s_home  },
      {KEY_END       , EdKC_end   }, {KEY_SEND      , EdKC_s_end   },
      {KEY_NPAGE     , EdKC_pgdn  }, {KEY_SNEXT     , EdKC_s_pgdn  },
      {KEY_PPAGE     , EdKC_pgup  }, {KEY_SPREVIOUS , EdKC_s_pgup  },
      {KEY_UP        , EdKC_up    },
      {KEY_DOWN      , EdKC_down  },
      {KEY_BACKSPACE , EdKC_bksp  },

      {KEY_LL   , EdKC_end }, // used in old termcap/infos

      // see also capabilities ka1, ka3, kb2, kc1, kc3 above
      {KEY_A1, EdKC_home },                       {KEY_A3, EdKC_pgup },
                            {KEY_B2, EdKC_center},
      {KEY_C1, EdKC_end  },                       {KEY_C3, EdKC_pgdn },

      {KEY_ENTER, EdKC_enter }, // mimic Win32 behavior

      // replaced (possibly unnecessarily}, by cap_nm_to_ncurses_ch(},
      {KEY_F( 1), EdKC_f1 }, {KEY_F(13), EdKC_s_f1 },  {KEY_F(25), EdKC_c_f1 }, {KEY_F(49), EdKC_a_f1 },
      {KEY_F( 2), EdKC_f2 }, {KEY_F(14), EdKC_s_f2 },  {KEY_F(26), EdKC_c_f2 }, {KEY_F(50), EdKC_a_f2 },
      {KEY_F( 3), EdKC_f3 }, {KEY_F(15), EdKC_s_f3 },  {KEY_F(27), EdKC_c_f3 }, {KEY_F(51), EdKC_a_f3 }, // decoded elsewhere too
      {KEY_F( 4), EdKC_f4 }, {KEY_F(16), EdKC_s_f4 },  {KEY_F(28), EdKC_c_f4 }, {KEY_F(52), EdKC_a_f4 }, // decoded elsewhere too
      {KEY_F( 5), EdKC_f5 }, {KEY_F(17), EdKC_s_f5 },  {KEY_F(29), EdKC_c_f5 }, {KEY_F(53), EdKC_a_f5 },
      {KEY_F( 6), EdKC_f6 }, {KEY_F(18), EdKC_s_f6 },  {KEY_F(30), EdKC_c_f6 }, {KEY_F(54), EdKC_a_f6 },
      {KEY_F( 7), EdKC_f7 }, {KEY_F(19), EdKC_s_f7 },  {KEY_F(31), EdKC_c_f7 }, {KEY_F(55), EdKC_a_f7 },
      {KEY_F( 8), EdKC_f8 }, {KEY_F(20), EdKC_s_f8 },  {KEY_F(32), EdKC_c_f8 }, {KEY_F(56), EdKC_a_f8 },
      {KEY_F( 9), EdKC_f9 }, {KEY_F(21), EdKC_s_f9 },  {KEY_F(33), EdKC_c_f9 }, {KEY_F(57), EdKC_a_f9 },
      {KEY_F(10), EdKC_f10}, {KEY_F(22), EdKC_s_f10},  {KEY_F(34), EdKC_c_f10}, {KEY_F(58), EdKC_a_f10},
      {KEY_F(11), EdKC_f11}, {KEY_F(23), EdKC_s_f11},  {KEY_F(35), EdKC_c_f11}, {KEY_F(59), EdKC_a_f11},
      {KEY_F(12), EdKC_f12}, {KEY_F(24), EdKC_s_f12},  {KEY_F(36), EdKC_c_f12}, {KEY_F(60), EdKC_a_f12},
      };
   DBG( "%s", "" );
   for( auto &el : s_nckc2edkc ) {
      if( has_key( el.nckc ) ) {
         ncurses_ch_to_EdKC[ el.nckc ] = el.edkc;
         DBG( KEYMAPFMT, el.nckc, el.nckc, "nckc#", "?", KeyNmOfEdkc( el.edkc ).c_str() );
         }
      }
   DBG( "%s", "" );

   STATIC_VAR const struct { const char *cap_nm; uint16_t edkc; } s_kn2edkc[] = {
      // early/leading instances are overridden by later

      //------------------------------------------------------------------------------
      // from (any?) terminfo(5) man page   man 5 terminfo
      // "In addition, if the keypad has a 3 by 3 array of keys including the
      // four arrow keys, the other five keys can be given as ka1, ka3, kb2, kc1,
      // and kc3.  These keys are useful when the effects of a 3 by 3 directional
      // pad are needed."
      { "ka1"  , EdKC_home, },                           { "ka3", EdKC_pgup },
                               { "kb2"  , EdKC_center },
      { "kc1"  , EdKC_end,  },                           { "kc3", EdKC_pgdn },
      //------------------------------------------------------------------------------

      { "kDC"  , EdKC_del    }, { "kDC3" , EdKC_a_del   }, { "kDC5" , EdKC_c_del   }, { "kDC6" , EdKC_cs_del  },
      /* 0=shifted */           { "kDN"  , EdKC_s_down  }, { "kDN3" , EdKC_a_down  }, { "kDN5" , EdKC_c_down  }, { "kDN6" , EdKC_cs_down  },

      { "kEND" , EdKC_end    }, { "kEND2", EdKC_s_end   }, { "kEND3", EdKC_a_end   }, { "kEND5", EdKC_c_end   }, { "kEND6", EdKC_cs_end   },
      { "kHOM" , EdKC_home   }, { "kHOM2", EdKC_s_home  }, { "kHOM3", EdKC_a_home  }, { "kHOM5", EdKC_c_home  }, { "kHOM6", EdKC_cs_home  },
      // maybe?
      { "kIC"  , EdKC_ins    }, { "kIC2" , EdKC_s_ins   }, { "kIC3" , EdKC_a_ins   }, { "kIC5" , EdKC_c_ins   }, { "kIC6" , EdKC_cs_ins   },
      { "kLFT" , EdKC_left   }, { "kLFT2", EdKC_s_left  }, { "kLFT3", EdKC_a_left  }, { "kLFT5", EdKC_c_left  }, { "kLFT6", EdKC_cs_left  },
      { "kNXT" , EdKC_pgdn   }, { "kNXT2", EdKC_s_pgdn  }, { "kNXT3", EdKC_a_pgdn  }, { "kNXT5", EdKC_c_pgdn  }, { "kNXT6", EdKC_cs_pgdn  },
      { "kPRV" , EdKC_pgup   }, { "kPRV2", EdKC_s_pgup  }, { "kPRV3", EdKC_a_pgup  }, { "kPRV5", EdKC_c_pgup  }, { "kPRV6", EdKC_cs_pgup  },
      { "kRIT" , EdKC_right  }, { "kRIT2", EdKC_s_right }, { "kRIT3", EdKC_a_right }, { "kRIT5", EdKC_c_right }, { "kRIT6", EdKC_cs_right },
      /* 0=shifted */           { "kUP"  , EdKC_s_up    }, { "kUP3" , EdKC_a_up    }, { "kUP5" , EdKC_c_up    }, { "kUP6" , EdKC_cs_up    },

   // { "kind"                   },
   // { "kri"                    },
      { "kf1" , EdKC_f1  }, { "kf13", EdKC_s_f1  }, { "kf25", EdKC_c_f1  }, { "kf37" , EdKC_cs_f1  }, { "kf49", EdKC_a_f1  },
      { "kf2" , EdKC_f2  }, { "kf14", EdKC_s_f2  }, { "kf26", EdKC_c_f2  }, { "kf38" , EdKC_cs_f2  }, { "kf50", EdKC_a_f2  },
      { "kf3" , EdKC_f3  }, { "kf15", EdKC_s_f3  }, { "kf27", EdKC_c_f3  }, { "kf39" , EdKC_cs_f3  }, { "kf51", EdKC_a_f3  }, // decoded elsewhere
      { "kf4" , EdKC_f4  }, { "kf16", EdKC_s_f4  }, { "kf28", EdKC_c_f4  }, { "kf40" , EdKC_cs_f4  }, { "kf52", EdKC_a_f4  }, // decoded elsewhere
      { "kf5" , EdKC_f5  }, { "kf17", EdKC_s_f5  }, { "kf29", EdKC_c_f5  }, { "kf41" , EdKC_cs_f5  }, { "kf53", EdKC_a_f5  },
      { "kf6" , EdKC_f6  }, { "kf18", EdKC_s_f6  }, { "kf30", EdKC_c_f6  }, { "kf42" , EdKC_cs_f6  }, { "kf54", EdKC_a_f6  },
      { "kf7" , EdKC_f7  }, { "kf19", EdKC_s_f7  }, { "kf31", EdKC_c_f7  }, { "kf43" , EdKC_cs_f7  }, { "kf55", EdKC_a_f7  },
      { "kf8" , EdKC_f8  }, { "kf20", EdKC_s_f8  }, { "kf32", EdKC_c_f8  }, { "kf44" , EdKC_cs_f8  }, { "kf56", EdKC_a_f8  },
      { "kf9" , EdKC_f9  }, { "kf21", EdKC_s_f9  }, { "kf33", EdKC_c_f9  }, { "kf45" , EdKC_cs_f9  }, { "kf57", EdKC_a_f9  },
      { "kf10", EdKC_f10 }, { "kf22", EdKC_s_f10 }, { "kf34", EdKC_c_f10 }, { "kf46" , EdKC_cs_f10 }, { "kf58", EdKC_a_f10 },
      { "kf11", EdKC_f11 }, { "kf23", EdKC_s_f11 }, { "kf35", EdKC_c_f11 }, { "kf47" , EdKC_cs_f11 }, { "kf59", EdKC_a_f11 },
      { "kf12", EdKC_f12 }, { "kf24", EdKC_s_f12 }, { "kf36", EdKC_c_f12 }, { "kf48" , EdKC_cs_f12 }, { "kf60", EdKC_a_f12 },

      // from the Putty manual, chapter 4
      // In SCO mode,
      // the function keys F1 to F12          generate ESC [M through to ESC [X.        kf1..kf12
      // Together with shift,            they generate ESC [Y through to ESC [j.        kf13..kf24
      // With control                    they generate ESC [k through to ESC [v, and    kf25..kf36
      // with shift and control together they generate ESC [w through to ESC [{.        kf37..kf48
      // (note that "together with alt" is not supported)                                   ^
      //                                                                                    |
      // these can be mapped into ----------------------------------------------------------
      // by `export TERM=putty-sco` _and_, in Putty config / Terminal / Keyboard , selecting SCO in "the function keys and keypad" section
      // putty-sco terminfo is probably installed (on ubuntu) by installing the ncurses-term package
      };
   for( auto &el : s_kn2edkc ) {
      cap_nm_to_ncurses_ch( el.cap_nm, el.edkc );
      }
   DBG( "%s", "" );
   }

// get keyboard event
STATIC_FXN int ConGetEvent();
// get extended event (more complex keystrokes)

STATIC_FXN int DecodeEscSeq_xterm( std::function<int()> getCh );

bool ConIn::FlushKeyQueueAnythingFlushed(){ return flushinp(); }

int ConIO::DbgPopf( PCChar fmt, ... ){ return 0; }

STATIC_FXN EdKC_Ascii GetEdKC_Ascii( bool fFreezeOtherThreads ) { // PRIMARY API for reading a key
   while( true ) {
      const auto fPassThreadBaton( !fFreezeOtherThreads );
      if( fPassThreadBaton ) { MainThreadGiveUpGlobalVariableLock(); }
      0 && DBG( "%s %s", __func__, fPassThreadBaton ? "passing" : "holding" );
      const auto ev( ConGetEvent() );
      if( fPassThreadBaton ) { MainThreadWaitForGlobalVariableLock(); }
      if( ev >= 0 ) {
         if( ev < EdKC_COUNT ) {
            if( ev == 0 ) { // (ev == 0) corresponds to keys we currently do not decode
               }
            else {
               EdKC_Ascii rv;
               rv.Ascii    = ev == EdKC_tab ? '\t' : ev;  // hack alert!
               rv.EdKcEnum = ModalKeyRemap( ev );
               return rv;  // normal exit path
               }
            }
         else {
            0 && DBG( "%s %u (vs. %u)", __func__, ev, EdKC_COUNT );
            switch( ev ) {
               break;case EdKC_EVENT_ProgramGotFocus:      RefreshCheckAllWindowsFBufs();
               break;case EdKC_EVENT_ProgramExitRequested: EditorExit( 0, true );
               }
            }
         CleanupAnyExecutionHaltRequest();
         }
      }
   }

void ConIn::WaitForKey() {
   GetEdKC_Ascii( true );
   }

EdKC_Ascii ConIn::EdKC_Ascii_FromNextKey() {
   return GetEdKC_Ascii( false );
   }

EdKC_Ascii ConIn::EdKC_Ascii_FromNextKey_Keystr( std::string &dest ) {
   const auto rv( GetEdKC_Ascii( false ) );
   dest.assign( KeyNmOfEdkc( rv.EdKcEnum ) );
   return rv;
   }

struct kpto_er {
   kpto_er() {
      keypad_mode_disable();
      conin_nonblocking_read();
      }
   ~kpto_er() {
      conin_blocking_read();
      keypad_mode_enable();
      }
   };

#define CR( is, rv )  case is: return rv;

#define CAS5( kynm ) \
   switch( mod ) { \
      case mod_cas: return EdKC_##kynm;    \
      case mod_Cas: return EdKC_c_##kynm;  \
      case mod_cAs: return EdKC_a_##kynm;  \
      case mod_caS: return EdKC_s_##kynm;  \
      case mod_CaS: return EdKC_cs_##kynm; \
      default:      return -1;            \
      }

// return -1 indicates that event should be ignored (resize event as an example)
STATIC_FXN int ConGetEvent() {
   const auto ch( getch() );
   if( ch < 0 )                      { return -1; }
   if( ch < ELEMENTS(ncurses_ch_to_EdKC) && ncurses_ch_to_EdKC[ ch ] ) {
      const auto rv( ncurses_ch_to_EdKC[ ch ] );
      // DBG( "ncurses_ch_to_EdKC[ %d ] => %s", ch, KeyNmOfEdkc( rv ).c_str() );
      return rv;
      }
   if( ch <= 0xFF ) {
      if( ch == 27 ) {
         int chin[32];
         int chinIx( 0 );
         chin[ chinIx++ ] = ch;
         auto getCh = [&chin, &chinIx]() {
            int newch = getch();
            if( newch >= 0 && chinIx < ELEMENTS(chin) ) {
               chin[ chinIx++ ] = newch;
               }
            return newch;
            };

         kpto_er kpto_cleaner;
         const auto rv( DecodeEscSeq_xterm( getCh ) );

         char tib[65]; auto pob( tib ); auto nob( sizeof( tib ) ); terminfo_str( pob, nob, chin, chinIx );
         if( rv < 0 ) { Msg( "unrecognized escseq %s\n", tib ); }
         else { 0 && DBG( "escseq: %s=%s", KeyNmOfEdkc( rv ).c_str(), tib ); }
         return rv;
         }

      if( ch == '\r' || ch == '\n' ) { return EdKC_enter; }
      if( ch == '\t' )               { return EdKC_tab; }
      if( ch >= 28 && ch <= 31 )     { return EdKC_c_4 + (ch - 28); }
      if( ch < 27 )                  { return EdKC_c_a + (ch -  1); }
      return ch;
      }
   switch (ch) { // translate "active" ncurses keycodes:
      case KEY_RESIZE:   ConOut::Resize();  return -1;
      case KEY_MOUSE:  /*Event->What = evNone;
                         ConGetMouseEvent(Event);  break;
      case KEY_SF:       KEvent->Code = kfShift | kbDown;  break;
      case KEY_SR:       KEvent->Code = kfShift | kbUp;    break;
      case KEY_SRIGHT:   KEvent->Code = kfShift | kbRight; */
           // Msg( "%s KEY_MOUSE event 0%o %d\n", __func__, ch, ch );
           return -1;

      default:
           // fprintf(stderr, "Unknown 0%o %d\n", ch, ch);
           // Msg( "%s Unknown event 0%o %d\n", __func__, ch, ch );
           return -1;
      }
   }

STATIC_FXN int DecodeEscSeq_xterm( std::function<int()> getCh ) { // http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-PC-Style-Function-Keys
   enum {mod_ctrl=0x4,mod_alt=0x2,mod_shift=0x1};
   auto decode_modch = []( int ch ) {
      // http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-PC-Style-Function-Keys
      //
      //   Code     Modifiers
      // ---------+---------------------------
      //    2     | Shift
      //    3     | Alt
      //    4     | Shift + Alt
      //    5     | Control
      //    6     | Shift + Control
      //    7     | Alt + Control
      //    8     | Shift + Alt + Control
      int mod;  enum {mod_ctrl=0x4,mod_alt=0x2,mod_shift=0x1};
      switch( ch ) {
         break;default : mod =    0     +    0    +    0     ;
         break;case '2': mod =    0     +    0    + mod_shift;
         break;case '3': mod =    0     + mod_alt +    0     ;
         break;case '4': mod =    0     + mod_alt + mod_shift;
         break;case '5': mod = mod_ctrl +    0    +    0     ;
         break;case '6': mod = mod_ctrl +    0    + mod_shift;
         break;case '7': mod = mod_ctrl + mod_alt +    0     ;
         break;case '8': mod = mod_ctrl + mod_alt + mod_shift;
         }
      return mod;
      };

   bool kbAlt = false;
   int ch = getCh();
   if( ch == ERR ) { return EdKC_esc; }
   if( ch == 27 ) { // 2nd consecutive ESC?
      ch = getCh();
      if( ch == '[' || ch == 'O' ) {
         kbAlt = true;
         }
      }
   if( ch == '[' || ch == 'O' ) { // decode CSI and SS3 sequences; 98% identical
      const auto fSS3( ch == 'O' ); const auto fCSI( !fSS3 );
      int ch1 = getCh();
      if( ch1 == ERR ) { return fCSI ? EdKC_a_LEFT_SQ : EdKC_a_o ; }

      // https://en.wikipedia.org/wiki/ANSI_escape_code
      // "Old versions of Terminator generate `SS3 1 ; modifiers char` when F1-F4 are
      // pressed with modifiers.  The faulty behavior was copied from GNOME Terminal."

      auto endch(0);
      if( /* fCSI && */ ch1 >= '1' && ch1 <= '8' ) { // CSI 1..8
         endch = getCh();
         if( endch == ERR ) { // //[n, not valid
            // TODO, should this be ALT-7 ?
            endch = '\0';
            ch1 = '\0';
            }
         }
      else { // CSI !(1..8) || SS3 do not have trailing ~
         endch = ch1;
         ch1 = '\0';
         }

      auto modch(0);
      if( endch == ';' ) { // [n;mX
         modch = getCh();
         endch = getCh();
         }
      else if( ch1 != '\0' && endch != '~' && endch != '$' ) { // [mA
         modch = ch1;
         ch1 = '\0';
         }
      auto mod( decode_modch( modch ) );
      if( kbAlt ) mod |= mod_alt;
      enum { mod_cas= 0          ,
             mod_caS= mod_shift  ,
             mod_cAs= mod_alt    ,
             mod_Cas= mod_ctrl   ,
             mod_CaS= mod_ctrl | mod_shift,
           };
      switch (endch) {
         default : return -1;
         case 'A': CAS5( up       ); break;
         case 'B': CAS5( down     ); break;
         case 'C': CAS5( right    ); break;
         case 'D': CAS5( left     ); break;
         case 'E': CAS5( center   ); break;
         case 'F': CAS5( end      ); break;
         case 'G': CAS5( center   ); break;  // TERM=screen (only mod_cas==mod seen)
         case 'H': CAS5( home     ); break;
         case 'M': CAS5( enter    ); break; // hack for PuTTY
         case 'P': CAS5( f1       ); break; // SS3 P
         case 'Q': CAS5( f2       ); break; // SS3 Q
         case 'R': CAS5( f3       ); break; // SS3 R
         case 'S': CAS5( f4       ); break; // SS3 S
         case 'j': CAS5( numStar  ); break;
         case 'k': CAS5( numPlus  ); break;
         case 'm': CAS5( numMinus ); break;
         case 'o': CAS5( numSlash ); break;
         case 'a': return (mod & mod_ctrl) ? EdKC_cs_up    : EdKC_s_up;
         case 'b': return (mod & mod_ctrl) ? EdKC_cs_down  : EdKC_s_down;
         case 'c': return (mod & mod_ctrl) ? EdKC_cs_right : EdKC_s_right;
         case 'd': return (mod & mod_ctrl) ? EdKC_cs_left  : EdKC_s_left;
                   //----------------------------------------------------
         case '$': mod |= mod_shift;  /* FALL THRU!!! */
         case '~':
             switch (ch1) { // CSI n ~
                default : return -1;
                case '1': CAS5( home ); break;
                case '7': CAS5( home ); break;
                case '4': CAS5( end  ); break;
                case '8': CAS5( end  ); break;
                case '2': CAS5( ins  ); break;
                case '3': CAS5( del  ); break; // CSI 3 ~
                case '5': CAS5( pgup ); break;
                case '6': CAS5( pgdn ); break;
                }
             break;
         }
   } else { // alt+...
      if (ch == '\r' || ch == '\n')        { return EdKC_a_enter; }
      else if( ch == '\t' )                { return EdKC_a_tab;   }
      else if( ch < ' '   )                { return -1; } // alt + ctr + key;  unsupported by 'K'
      else {
         if     ( ch >= '0' && ch <= '9' ) { return EdKC_a_0 + (ch - '0'); }
         else if( ch >= 'a' && ch <= 'z' ) { return EdKC_a_a + (ch - 'a'); }
         else if( ch >= 'A' && ch <= 'Z' ) { return EdKC_a_a + (ch - 'A'); } // Alt-A == Alt-a
         else if( ch == '\'' )             { return EdKC_a_TICK;           }
         else if( ch == ','  )             { return EdKC_a_COMMA;          }
         else if( ch == '-'  )             { return EdKC_a_MINUS;          }
         else if( ch == '.'  )             { return EdKC_a_DOT;            }
         else if( ch == '/'  )             { return EdKC_a_SLASH;          }
         else if( ch == ';'  )             { return EdKC_a_SEMICOLON;      }
         else if( ch == '='  )             { return EdKC_a_EQUAL;          }
      // else if( ch == '['  )             { return EdKC_a_LEFT_SQ;        } CID128049
         else if( ch == '\\' )             { return EdKC_a_BACKSLASH;      }
         else if( ch == ']'  )             { return EdKC_a_RIGHT_SQ;       }
         else if( ch == '`'  )             { return EdKC_a_BACKTICK;       }
         else if( ch == 127  )             { return EdKC_a_bksp;           }
         else                              { return ch;                    }
         }
      }

   return ERR;
   }
