//
// Copyright 2015-2022 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

STATIC_VAR int  s_DecodeErrCount = 0;
int  ConIn::DecodeErrCount() { return s_DecodeErrCount; }
STATIC_FXN int IncrDecodeErrCount() {
   ++s_DecodeErrCount;
   DispNeedsRedrawStatLn();
   return 1;  // for && chaining
   }

STATIC_VAR bool s_fDbg = false;
void ConIn::log_verbose() { s_fDbg = true ; }
void ConIn::log_quiet  () { s_fDbg = false; }

#define stESC "\x1b"

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

STATIC_FXN PChar terminfo_str( PChar dest, size_t sizeofDest, PCChar ach, int numCh ) {
   for( auto ix(0) ; ix < numCh ; ++ix ) {
      terminfo_ch( dest, sizeofDest, ach[ ix ] );
      }
   return dest;
   }

STATIC_FXN int  DBG_keybound( int ch ) {
   for( auto ix=0; ; ++ix ) {
      auto tinm = keybound( ch, ix );
      if( !tinm ) { return 1; }
      const auto tinm_len = Strlen(tinm);
      char tib[65]; terminfo_str( BSOB(tib), tinm, tinm_len );
      DBG( "0%04o=0x%04x=0d%3d; keybound[%d] = '%s' L %d (0x%x)", ch, ch, ch, ix, tib, tinm_len, tinm_len > 0 ? tinm[0] : 0 );
      free( tinm );
      }
   return 1; // for && chaining
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

//
// ncfkt: NCurses Function Key Token.
//
// When ncurses translates a keystroke directly to a unique KEY_ code (ncfkt)
// returned by *a single call to* getch(), ncurses docs call this value a
// "Function Key Token", which I have abbreviated "ncfkt".  Note that ncfkt have
// values > 0xFF. ncfkt may be either #defined as KEY_* in e.g. curses.h (see
// above), or apparently can be dynamically defined by e.g. terminfo (or
// termcap).
//
// If defined, an ncfkt is always returned in lieu of its associated escseqstr
// being coughed up char by char by getch() when its associated key is pressed.
//
// (ncurses') curses.h #define's KEY_MAX, which is apparently the max numeric
// value of any KEY_* macro defined therein.  However because ncfkt's can be
// dynamically defined, and these are apparently always given values > KEY_MAX,
// any static mapping tables indexed by ncfkt should have ELEMENTS >> KEY_MAX.
//
STATIC_VAR uint16_t ncfkt_to_EdKC[KEY_MAX+400]; // indexed by ncfkt; note that first 256 elements map to "regular" characters

STATIC_FXN void escseqstr_to_ncfkt( const char *escseqstr, uint16_t edkc, const char *cap_nm ) { enum { SD=0 };
   auto keyNm = KeyNmOfEdkc( edkc );
   if( !escseqstr || (long)(escseqstr) == -1 ) {
#define KEYMAPFMT  "0%04o=0x%04x=0d%3d cap=%-5s ; tistr=%-8s ; EdkeyNm=%s"
      1 && DBG( KEYMAPFMT, 0, 0, 0, cap_nm, "?", keyNm.c_str() );
      return;
      }
   char tib[65]; terminfo_str( BSOB(tib), escseqstr, Strlen(escseqstr) );
                                               SD && DBG( "key_defined+ %s", tib );
   const auto ncfkt = key_defined(escseqstr);  SD && DBG( "key_defined- %s", tib ); // key_defined() <- ncurses
   DBG( KEYMAPFMT, ncfkt, ncfkt, ncfkt, cap_nm, tib, keyNm.c_str() );
   if( ncfkt > 0 && has_key( ncfkt ) ) {  // `has_key( ncfkt )` may be redundant to `ncfkt = key_defined(escseqstr)`
      if( ncfkt < ELEMENTS( ncfkt_to_EdKC ) ) {
         if( ncfkt_to_EdKC[ ncfkt ] != edkc ) {
            if( ncfkt_to_EdKC[ ncfkt ] ) {
               keyNm = KeyNmOfEdkc( ncfkt_to_EdKC[ ncfkt ] );
               DBG( "0%04o=0d%d EdKC=%s overridden!", ncfkt, ncfkt, keyNm.c_str() );
               }
            ncfkt_to_EdKC[ ncfkt ] = edkc;
            }
         }
      else {
         Msg( "INTERNAL ERROR: ncfkt=%d exceeds capacity of ncfkt_to_EdKC[%lu]!", ncfkt, ELEMENTS( ncfkt_to_EdKC ) );
         }
      }
   }

STATIC_FXN void cap_nm_to_ncfkt( const char *cap_nm, uint16_t edkc ) { enum { SD=0 };
   /* if terminfo defines (as an escseqstr) a capability named cap_nm, _and_
      ncurses maps that escseqstr to an ncfkt (which getch() returns), then add
      an entry ncfkt_to_EdKC[ncfkt] = EdKC where EdKC is the EDITOR KEYCODE which
      corresponds to cap_nm.

      cap_nm is a terminfo "capability name", the key of a key=value mapping defined in a terminfo file
      see http://invisible-island.net/xterm/terminfo-contents.html  ; EX:
      xterm+pce3|fragment with modifyCursorKeys:3,
              kDC=\E[3;2~,                               key=cap_nm=kDC, value=escseqstr=\E[>3;2~
   */
                                                 SD && DBG( "tigetstr+ %s", cap_nm );
   const char *escseqstr( tigetstr( cap_nm ) );  SD && DBG( "tigetstr- %s", cap_nm ); // tigetstr() <- "retrieves a capability from the terminfo database"
   escseqstr_to_ncfkt( escseqstr, edkc, cap_nm );
   }

STATIC_VAR bool s_keypad_mode;
STATIC_FXN void keypad_mode_disable() { keypad(stdscr, 0); s_keypad_mode = false; }
STATIC_FXN void keypad_mode_enable()  { keypad(stdscr, 1); s_keypad_mode = true ; }

STATIC_VAR bool s_conin_blocking_read;
GLOBAL_VAR int  g_iConin_nonblk_rd_tmout = 10;
STATIC_FXN void conin_nonblocking_read() { timeout(g_iConin_nonblk_rd_tmout); s_conin_blocking_read = false; } // getCh blocks for (10) milliseconds, returns ERR if there is still no input
STATIC_FXN void conin_blocking_read()    { timeout(  -1                    ); s_conin_blocking_read = true;  } // getCh blocks waiting for next char

STATIC_FXN void init_kn2edkc() {
   DBG( "init_kn2edkc" );
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
      //
      // 20240415: The following were initially observed (on 20240410 on PopOS
      // 22.04 LTS) as undecoded ncfkt's (whose associated escseq was dumped by
      // DBG_keybound), however the ncfkt's, being > KEY_MAX, were clearly not
      // associated with KEY_ ncurses #define's, and `infocmp` output revealed no
      // cap_nm's associated with the observed escseq's, so in the interim I
      // concocted escseqstr_to_ncfkt to translate the escseq into its ncfkt (on
      // the theory that the escseq was less likely to change than its associated
      // ncfkt, a mere number whose source remains unknown).  After days of
      // seeking (and pure trial and error) the associated cap_nm's of these
      // escseq's, today I discovered that `infocmp -x` ("print[s] information for
      // user-defined capabilities") output contains them!!!:
      //
      // https://man.archlinux.org/man/core/ncurses/infocmp.1m.en
      // https://man.archlinux.org/man/user_caps.5.en
      //
      { "kpADD",  EdKC_numPlus   },  // stESC "Ok"  PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color
      { "kpDIV",  EdKC_numSlash  },  // stESC "Oo"  PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color
      { "kpMUL",  EdKC_numStar   },  // stESC "Oj"  PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color
      { "kpSUB",  EdKC_numMinus  },  // stESC "Om"  PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color
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
      cap_nm_to_ncfkt( el.cap_nm, el.edkc );
      }
   }

STATIC_FXN void init_ncfkt2edkc() {
   DBG( "init_ncfkt2edkc" );
   STATIC_VAR const struct { short ncfkt; uint16_t edkc; } s_ncfkt2edkc[] = {
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
                            {KEY_B2 , EdKC_center},
                            {KEY_BEG, EdKC_center},  // PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color
      {KEY_C1, EdKC_end  },                       {KEY_C3, EdKC_pgdn },

      {KEY_ENTER, EdKC_enter }, // mimic Win32 behavior

      // replaced (possibly unnecessarily}, by cap_nm_to_ncfkt(},
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

      // 20240410 how these magic #s (e.g. 0x23f) were discovered, then superseded by "something better":
      // ("Oh, the humanity!")
      // * execute EdFxn tell, hit key, and if necessary hit a defined key to finish decode and complete tell.
      // * consult editor log to find which logging corresponds to that an unrecognized key:
      //
      // A. if "Unknown event" followed by a line dumping a (probably 16-bit)
      //    int value > 0xFF in octal=hex=dec followed by an escseqstr, then
      //    ncurses translated the keystroke directly to a ncfkt.  If the ncfkt >
      //    KEY_MAX, it was dynamically defined by e.g. terminfo (or termcap).  I
      //    am unaware of how to discover the capability name (cap_nm) associated
      //    with such an ncfkt, thus as an expedient I first hard-coded the
      //    numeric value of ncfkt seen in the editor log back into the editor
      //    source code:

      // following "work" for as long as the number to key mapping persists.  Disabled as keying on escseqstr seems slightly more long-lived...
      // {0x23f, EdKC_numPlus },  //  ULTRA HACK!!! PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color
      // {0x241, EdKC_numSlash},  //  ULTRA HACK!!! PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color
      // {0x243, EdKC_numStar },  //  ULTRA HACK!!! PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color
      // {0x244, EdKC_numMinus},  //  ULTRA HACK!!! PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color

      //    This "worked" but is obviously horrendously bad practice and bound
      //    to break in the future when the mapping changes.

      //    A variant implementation (see below, s_escseqstr2edkc) performs
      //    runtime lookup of the ncfkt associated with a hard-coded escseqstr
      //    gleaned from the editor log (using ncurses key_defined()), and using
      //    this ncfkt create an entry into ncfkt_to_EdKC.  This approach is
      //    preferred/used because escseqstr seems a more stable key value (but
      //    who can be sure!?).

      // B. An escseqstr, being the escape sequence for a single keystroke (or
      //    chorded combo) has been read (7- or 8-bit) character by (7- or
      //    8-bit) character at a time via a sequence of calls to getch(), and
      //    the editor has failed to decode it.  In this case by definition
      //    ncurses is not providing a ncfkt in lieu of the escseqstr, and the
      //    (fairly unpleasant) editor escape sequence decoding code
      //    (DecodeEscSeq_xterm) needs to be updated to translate the received
      //    escseqstr to the desired EdKC.
      //
      // Note that all described above "works" *only* in dflt desktop-terminal
      // of the distro I'm running: `lsb_release -a`: "Pop!_OS 22.04 LTS". and
      // may or may not work on any other Linux distro.
      //
      // Also, *I can say from first-hand experience* that many of these key
      // mappings *do not work* when ssh'ing in to run k on my Ubuntu 22.04 LTS
      // server from Windows 10 using the git-for-windows bash toolset.
      };
   for( auto &el : s_ncfkt2edkc ) {
      if( has_key( el.ncfkt ) ) {
         ncfkt_to_EdKC[ el.ncfkt ] = el.edkc;
         DBG( KEYMAPFMT, el.ncfkt, el.ncfkt, el.ncfkt, "?", "?", KeyNmOfEdkc( el.edkc ).c_str() );
         }
      }
   }

STATIC_FXN void init_escseqstr2edkc() {
   DBG( "init_escseqstr2edkc" );
   STATIC_VAR const struct { const char *escseqstr; uint16_t edkc; } s_escseqstr2edkc[] = {
      // 20240415: superseded by entries in s_kn2edkc thanks to `infocmp -x`
      // { stESC "Ok", EdKC_numPlus  },  // PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color   kpADD
      // { stESC "Oo", EdKC_numSlash },  // PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color   kpDIV
      // { stESC "Oj", EdKC_numStar  },  // PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color   kpMul
      // { stESC "Om", EdKC_numMinus },  // PopOS 22.04 LTS, dflt desktop terminal, TERM=xterm-256color   kpSUB
      };
   for( auto &el : s_escseqstr2edkc ) {
      escseqstr_to_ncfkt( el.escseqstr, el.edkc, "?" );
      }
   }

void conin_ncurses_init() {  // this MIGHT need to be made $TERM-specific
   DBG( "%s ++++++++++++++++", __func__ );
   noecho();              // we do not change
   nonl();                // we do not change
   conin_blocking_read();
   keypad_mode_enable();
   meta(stdscr, 1);       // we do not change

   init_kn2edkc();
   init_ncfkt2edkc();
   init_escseqstr2edkc();

   DBG( "%s ----------------", __func__ );
   }

// get keyboard event
STATIC_FXN int ConGetEvent();
// get extended event (more complex keystrokes)

STATIC_FXN int DecodeEscSeq_xterm( stref escseq, std::function<int()> getCh );

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

#define CR( is, rv )  break;case is: return rv;

// return -1 indicates that event should be ignored (resize event as an example)
STATIC_FXN int ConGetEvent() {                             0 && DBG( "++++++ ConGetEvent()" );
   const auto ch( getch() );
   if( ch < 0 ) { return -1; }
   if( ch < ELEMENTS(ncfkt_to_EdKC) && ncfkt_to_EdKC[ ch ] ) {
      const auto rv( ncfkt_to_EdKC[ ch ] );           s_fDbg && DBG( "ncfkt_to_EdKC[ %d ] => %s", ch, KeyNmOfEdkc( rv ).c_str() );
      return rv;
      }
   if( ch == 27 ) { // escseq ...
      char chin[32];
      int wrIx = 0;
      chin[ wrIx++ ] = ch;
      auto getCh = [&chin, &wrIx]() {
         int newch = getch();
         if( newch > 0xFF ) {  DBG( "INTERNAL ERROR: getch returned non-8-bit value: 0x%x", newch );
            return newch;
            }
         if( newch >= 0 ) {
            if( wrIx < ELEMENTS(chin) ) {
               chin[ wrIx++ ] = newch;        s_fDbg && DBG( "getCh => %c (0x%02X)", newch, newch );
               }
            else {                                      DBG( "INTERNAL ERROR: getCh BUFFER OVERRUN" );
               }
            }
         return newch;
         };
      kpto_er kpto_cleaner;
      while( getCh() >= 0 ) {}  // slurp entire escseq
      char tib[5*ELEMENTS(chin)]; terminfo_str( BSOB(tib), chin, wrIx ); // 5 is worst-case '\o000' encoding
      auto slurped = stref(chin,wrIx);

      // "simple" escseq's are not treated as exceptional for logging purposes
      if( slurped.length() == 1 ) { return EdKC_esc; }
      if( slurped.length() == 2 && slurped[1] < 0x80 ) { // 7-bit alt chars (8-bit are exceptional, handled in DecodeEscSeq_xterm)
         const auto altCh = slurped[1];
         switch( altCh ) {
            CR( 127 , EdKC_a_bksp      );
            CR( '\r', EdKC_a_enter     );
            CR( '\n', EdKC_a_enter     );
            CR( '\t', EdKC_a_tab       );
            CR( '\'', EdKC_a_TICK      );
            CR( '\\', EdKC_a_BACKSLASH );
            CR( ',' , EdKC_a_COMMA     );
            CR( '-' , EdKC_a_MINUS     );
            CR( '.' , EdKC_a_DOT       );
            CR( '/' , EdKC_a_SLASH     );
            CR( ';' , EdKC_a_SEMICOLON );
            CR( '=' , EdKC_a_EQUAL     );
            CR( '[' , EdKC_a_LEFT_SQ   );   // CID128049
            CR( ']' , EdKC_a_RIGHT_SQ  );
            CR( '`' , EdKC_a_BACKTICK  );
            break;default: // ranges follow ...
            if     ( altCh < ' ' )                  { DBG( "DECODE ERROR: unmapped ctrl-alt ch=0x%02x", altCh ); IncrDecodeErrCount(); return -1; } // alt + ctr + key;  unsupported by 'K'
            else if( altCh >= '0' && altCh <= '9' ) { return EdKC_a_0 + (altCh - '0'); }
            else if( altCh >= 'a' && altCh <= 'z' ) { return EdKC_a_a + (altCh - 'a'); }
            else if( altCh >= 'A' && altCh <= 'Z' ) { return EdKC_a_a + (altCh - 'A'); } // Alt-A == Alt-a
            else                              { DBG( "DECODE ERROR: unmapped 7-bit alt ch=%c", altCh ); IncrDecodeErrCount();
                                                return -1;
                                              }
            }
         }                                                         s_fDbg && DBG( "+++ DecodeEscSeq_xterm %s", tib );
      int rdIx = 1; // skip initial esc
      const auto rv = DecodeEscSeq_xterm( slurped, [&chin,&wrIx=std::as_const(wrIx),&rdIx]() {
            const int newch = rdIx < wrIx ? chin[ rdIx++ ] : ERR;  s_fDbg && DBG( "rdCh => %c (0x%02X)", newch, newch );
            return newch;
            }
         );
      if( rv < 0 ) { DBG( "DECODE ERROR: unrecognized escseq %s", tib ); IncrDecodeErrCount(); }
      else { DBG( "--- escseq %s = %s (%d)", tib, KeyNmOfEdkc( rv ).c_str(), rv ); }
      return rv;
      }

   if( ch <= 0xFF ) {
      if( ch == '\r' || ch == '\n' ) { s_fDbg && DBG( "ch == '\r' || ch == '\n' => EdKC_enter" ); return EdKC_enter; }
      if( ch == '\t' )               { s_fDbg && DBG( "ch == '\t' => EdKC_tab"                 ); return EdKC_tab; }
      s_fDbg && DBG( "ch == '\\x%02x'", ch );
      if( ch == 0 )                  { return -1; }
      if( ch >= 28 && ch <= 31 )     { auto rv = EdKC_c_4 + (ch - 28); s_fDbg && DBG( "ch >= 28 && ch <= 31 => %s", KeyNmOfEdkc( rv ).c_str() ); return rv; }
      if( ch < 27 )                  { auto rv = EdKC_c_a + (ch -  1); s_fDbg && DBG( "ch < 27 => %s"   , KeyNmOfEdkc( rv ).c_str() ); return rv; }
                                        s_fDbg && DBG( "(dflt) => %c (0x%02X)", ch, ch );
      return ch;
      }
   switch (ch) { // translate "active" ncurses keycodes:
      case KEY_RESIZE:   ConOut::Resize();  return -1;
      case KEY_MOUSE:  /*Event->What = evNone;
                         ConGetMouseEvent(Event);  break;
      case KEY_SF:       KEvent->Code = kfShift | kbDown;  break;
      case KEY_SR:       KEvent->Code = kfShift | kbUp;    break;
      case KEY_SRIGHT:   KEvent->Code = kfShift | kbRight; */
                                       s_fDbg && DBG( "%s KEY_MOUSE event 0%o %d\n", __func__, ch, ch );
           return -1;

      default:                         DBG( "DECODE ERROR: %s Unknown event", __func__ ) && DBG_keybound( ch ) && IncrDecodeErrCount();
           return -1;
      }
   }

#define CAS5( kynm ) \
   switch( mod ) { \
      default:     return -1;        \
      CR( mod_cas, EdKC_##kynm    )  \
      CR( mod_Cas, EdKC_c_##kynm  )  \
      CR( mod_cAs, EdKC_a_##kynm  )  \
      CR( mod_caS, EdKC_s_##kynm  )  \
      CR( mod_CaS, EdKC_cs_##kynm )  \
      }

#define CAS6( kynm ) \
   switch( mod ) { \
      default:     return -1;       \
      CR( mod_cas, EdKC_##kynm    ) \
      CR( mod_Cas, EdKC_c_##kynm  ) \
      CR( mod_cAs, EdKC_a_##kynm  ) \
      CR( mod_caS, EdKC_s_##kynm  ) \
      CR( mod_CaS, EdKC_cs_##kynm ) \
      CR( mod_CAs, EdKC_ca_##kynm ) \
      }

// http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-PC-Style-Function-Keys
STATIC_FXN int DecodeEscSeq_xterm( stref escseq, std::function<int()> getCh ) {
   if( escseq.length() == 2 ) { // 8-bit alt chars (7-bit were handled by caller)
      DBG( "DECODE ERROR: unmapped 8-bit alt ch=%c (0x%02x)", escseq[1], escseq[1] );
      return -1;
      }

   if( escseq.length() == 3 && escseq[1] == 'O' ) {  // backup in case s_escseqstr2edkc ncfkt mappings have not been assigned
      switch( escseq[1] ) {
         CR( 'k', EdKC_numPlus  );
         CR( 'o', EdKC_numSlash );
         CR( 'j', EdKC_numStar  );
         CR( 'm', EdKC_numMinus );
         }
      }

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
         }                                        s_fDbg && DBG( "decode_modch => 0x%02X", mod );
      return mod;
      };

   bool kbAlt = false;
   int ch = getCh();
   if( ch == 27 ) { // 2nd consecutive ESC?
      ch = getCh();
      if( ch == '[' || ch == 'O' ) {
         kbAlt = true;                DBG( "2nd consecutive ESC + [O: kbAlt = true" );
         }
      else {                          DBG( "DECODE ERROR: 2nd consecutive ESC + ![O: ???" );
         return -1;
         }
      }
   if( ch == '[' || ch == 'O' ) { // decode CSI and SS3 sequences; 98% identical
      const auto fCSI( ch == '[' );
      const auto fSS3( ch == 'O' );                  s_fDbg && fSS3 && DBG( "SS3" );
      int ch1 = getCh();                             s_fDbg && DBG( "ch1=%c", ch1 );
      if( ch1 == ERR ) {
         auto rv = fCSI ? EdKC_a_LEFT_SQ : EdKC_a_o; s_fDbg && DBG( "ch1 == ERR, rv => %s", KeyNmOfEdkc( rv ).c_str() );
         return rv;
         }

      enum { mod_cas= 0          , // used by CAS5, etc.
             mod_caS= mod_shift  ,
             mod_cAs= mod_alt    ,
             mod_Cas= mod_ctrl   ,
             mod_CaS= mod_ctrl | mod_shift,
             mod_CAs= mod_ctrl | mod_alt,
           };

#define LINUX_CONSOLE 0
#if LINUX_CONSOLE
      if( fCSI && ch1 == '[' ) { // CSI [ [A-E]  Linux Console (incl ssh terminal) F1-F5 (with optional dup-esc prefix for alt+)
         const auto mod( kbAlt ? mod_alt : 0 );     s_fDbg && DBG( "fCSI && ch1 == '[', mod=0x%02X", mod );
         switch( getCh() ) {
            break;default : return -1;
            break;case 'A': CAS5( f1 );
            break;case 'B': CAS5( f2 );
            break;case 'C': CAS5( f3 );
            break;case 'D': CAS5( f4 );
            break;case 'E': CAS5( f5 );
            }
         }
      if( fCSI && (ch1 == '1' || ch1 == '2') ) {  // Linux Console (incl ssh terminal) specific (reversed via ./odkey.sh)
         const auto ch2 = getCh();
         const auto endch = getCh();
         auto mod( kbAlt ? mod_alt : 0 );            s_fDbg && DBG( "fCSI && (ch1 == '1' || ch1 == '2'), mod=0x%02X", mod );
         switch( endch ) {
            break;case '^': mod |= mod_ctrl;
            break;case '~':
            break;default:                          s_fDbg && DBG( "CSI %c ? followed by %c ?", ch1, endch );
                            return -1;
            }                                       s_fDbg && DBG( "--> CSI %c %c, mod=0x%02X", ch1, ch2, mod );
         switch( ch1 ) {
            break;case '1': // CSI 1 [1-57-9]  Linux console [Ctrl+]F[1-8] (with optional dup-esc prefix for alt+)
                                                     s_fDbg && DBG( "CSI 1 [1-57-9], mod=0x%02X", mod );
               switch( ch2 ) {
                  break;default : return -1;
                  break;case '1': CAS6( f1  );
                  break;case '2': CAS6( f2  );
                  break;case '3': CAS6( f3  );
                  break;case '4': CAS6( f4  );
                  break;case '5': CAS6( f5  );
                  break;case '7': CAS6( f6  );
                  break;case '8': CAS6( f7  );
                  break;case '9': CAS6( f8  );
                  }
            break;case '2': // CSI 2 [0134]  Linux console [Ctrl+]F[9-12] (with optional dup-esc prefix for alt+)
                                                     s_fDbg && DBG( "CSI 2 [0134], mod=0x%02X", mod );
               switch( ch2 ) {
                  break;default : return -1;
                  break;case '0': CAS6( f9  );
                  break;case '1': CAS6( f10 );
                  break;case '3': CAS6( f11 );
                  break;case '4': CAS6( f12 );
                  }
            }
         }
#endif  // if LINUX_CONSOLE

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
      auto mod = decode_modch( modch ) | (kbAlt ? mod_alt : 0);  s_fDbg && DBG( "switch (endch=%c), mod=0x%02X", endch, mod );
      switch( endch ) {
         break;default : DBG( "DECODE ERROR: 'switch( endch )' %c", endch ); return -1;
         break;case 'A': CAS5( up       );
         break;case 'B': CAS5( down     );
         break;case 'C': CAS5( right    );
         break;case 'D': CAS5( left     );
         break;case 'E': CAS5( center   );
         break;case 'F': CAS5( end      );
         break;case 'G': CAS5( center   );  // TERM=screen (only mod_cas==mod seen)
         break;case 'H': CAS5( home     );
         break;case 'M': CAS5( enter    );  // hack for PuTTY
         break;case 'P': CAS5( f1       );  // SS3 P
         break;case 'Q': CAS5( f2       );  // SS3 Q
         break;case 'R': CAS5( f3       );  // SS3 R
         break;case 'S': CAS5( f4       );  // SS3 S
         break;case 'j': CAS5( numStar  );
         break;case 'k': CAS5( numPlus  );
         break;case 'm': CAS5( numMinus );
         break;case 'o': CAS5( numSlash );
         break;case 'a': return (mod & mod_ctrl) ? EdKC_cs_up    : EdKC_s_up;
         break;case 'b': return (mod & mod_ctrl) ? EdKC_cs_down  : EdKC_s_down;
         break;case 'c': return (mod & mod_ctrl) ? EdKC_cs_right : EdKC_s_right;
         break;case 'd': return (mod & mod_ctrl) ? EdKC_cs_left  : EdKC_s_left;
                   //----------------------------------------------------
         break;case '$': mod |= mod_shift;  /* FALL THRU!!! */
         /*!*/ case '~':                                   s_fDbg && DBG( "CSI %c ~, mod=0x%02X", ch1, mod );
             switch (ch1) { // CSI n ~
                break;default : DBG( "DECODE ERROR: 'endch = %c, switch( ch1 )' %c", endch, ch1 ); return -1;
                break;case '1': CAS5( home );
                break;case '7': CAS5( home );
                break;case '4': CAS5( end  );
                break;case '8': CAS5( end  );
                break;case '2': CAS5( ins  );
                break;case '3': CAS5( del  );  // CSI 3 ~
                break;case '5': CAS5( pgup );
                break;case '6': CAS5( pgdn );
                }
         }
      } // decode CSI and SS3 sequences; 98% identical
   return -1;
   }
