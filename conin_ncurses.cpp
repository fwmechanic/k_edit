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

//
// from http://invisible-island.net/xterm/terminfo.html
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
STATIC_VAR U16 s_to_EdKC[600];

STATIC_FXN void keyname_to_code( const char *name, U16 edkc ) {
   linebuf edkcnmbuf; StrFromEdkc( BSOB(edkcnmbuf), edkc );
   const auto s( tigetstr( name ) );
   if( !s && (long)(s) == -1 ) {
      DBG( "0%04o=%d=tigetstr(%s)=%s EdKC=%s", 0, 0, name, "", edkcnmbuf );
      return;
      }
   const auto code( key_defined(s) );
   DBG( "0%04o=%d=tigetstr(%s)=%s EdKC=%s", code, code, name, s, edkcnmbuf );
   if( code > 0 ) {
      if( code < ELEMENTS( s_to_EdKC ) ) {
         if( s_to_EdKC[ code ] ) {
            StrFromEdkc( BSOB(edkcnmbuf), s_to_EdKC[ code ] );
            DBG( "0%04o=%d EdKC=%s overridden!", code, code, edkcnmbuf );
            }
         s_to_EdKC[ code ] = edkc;
         }
      else {
         Msg( "INTERNAL ERROR: code=%d out of range of s_to_EdKC[]!", code );
         }
      }
   }

void conin_ncurses_init() {
   noecho();
   nonl();
   keypad(stdscr, TRUE);
   meta(stdscr, 1);

   STATIC_VAR const struct { const char *kynm; U16 edkc; } s_kn2kc[] = {
      { "kDC"  , EdKC_del    }, { "kDC3"  , EdKC_a_del   }, { "kDC5" , EdKC_c_del }, { "kDC6" , EdKC_cs_del },
      // 0=shifted:
      { "kDN"  , EdKC_s_down }, { "kDN3"  , EdKC_a_down  }, { "kDN5"  , EdKC_c_down  }, { "kDN6"  , EdKC_cs_down  },
      { "kEND" , EdKC_end    }, { "kEND3" , EdKC_a_end   }, { "kEND5" , EdKC_c_end   }, { "kEND6" , EdKC_cs_end   },
      { "kHOM" , EdKC_home   }, { "kHOM3" , EdKC_a_home  }, { "kHOM5" , EdKC_c_home  }, { "kHOM6" , EdKC_cs_home  },
      // maybe?
      { "kIC"  , EdKC_ins    }, { "kIC3"  , EdKC_a_ins   }, { "kIC5"  , EdKC_c_ins   }, { "kIC6"  , EdKC_cs_ins   },
      { "kLFT" , EdKC_left   }, { "kLFT3" , EdKC_a_left  }, { "kLFT5" , EdKC_c_left  }, { "kLFT6" , EdKC_cs_left  },
      { "kNXT" , EdKC_pgdn   }, { "kNXT3" , EdKC_a_pgdn  }, { "kNXT5" , EdKC_c_pgdn  }, { "kNXT6" , EdKC_cs_pgdn  },
      { "kPRV" , EdKC_pgup   }, { "kPRV3" , EdKC_a_pgup  }, { "kPRV5" , EdKC_c_pgup  }, { "kPRV6" , EdKC_cs_pgup  },
      { "kRIT" , EdKC_right  }, { "kRIT3" , EdKC_a_right }, { "kRIT5" , EdKC_c_right }, { "kRIT6" , EdKC_cs_right },
      // 0=shifted:
      { "kUP"  , EdKC_s_up   }, { "kUP3"  , EdKC_a_up    }, { "kUP5"  , EdKC_c_up    }, { "kUP6"  , EdKC_cs_up    },
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
   };
   for( auto ix( 0u ) ; ix < ELEMENTS(s_kn2kc) ; ++ix ) {
      keyname_to_code( s_kn2kc[ix].kynm, s_kn2kc[ix].edkc );
      }
   }

// get keyboard event
STATIC_FXN int ConGetEvent();
// get extended event (more komplex keystrokes)
STATIC_FXN int ConGetEscEvent();

bool ConIn::FlushKeyQueueAnythingFlushed(){ return flushinp(); }

int ConIO::DbgPopf( PCChar fmt, ... ){ return 0; }

#define  EdKC_EVENT_ProgramGotFocus       (EdKC_COUNT+1)
#define  EdKC_EVENT_ProgramLostFocus      (EdKC_COUNT+2)
#define  EdKC_EVENT_ProgramExitRequested  (EdKC_COUNT+3)

STATIC_FXN EdKC_Ascii GetEdKC_Ascii( bool fFreezeOtherThreads ) { // PRIMARY API for reading a key
   while( true ) {
      const auto fPassThreadBaton( !fFreezeOtherThreads );
      if( fPassThreadBaton )  MainThreadGiveUpGlobalVariableLock();
      0 && DBG( "%s %s", __func__, fPassThreadBaton ? "passing" : "holding" );

      const auto ev( ConGetEvent() );

      if( fPassThreadBaton )  MainThreadWaitForGlobalVariableLock();

      if( ev != -1 ) {
         if( ev >= 0 && ev < EdKC_COUNT ) {
            if( ev == 0 ) { // (ev == 0) corresponds to keys we currently do not decode
               }
            else {
               EdKC_Ascii rv;
               rv.Ascii    = ev;
               rv.EdKcEnum = ev;
               return rv;  // normal exit path
               }
            }
         else {
            0 && DBG( "%s %u (vs. %u)", __func__, ev, EdKC_COUNT );
            switch( ev ) {
               case EdKC_EVENT_ProgramGotFocus:      RefreshCheckAllWindowsFBufs();  break;
               case EdKC_EVENT_ProgramExitRequested: EditorExit( 0, true );          break;
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

EdKC_Ascii ConIn::EdKC_Ascii_FromNextKey_Keystr( PChar dest, size_t sizeofDest ) {
   const auto rv( GetEdKC_Ascii( false ) );
   StrFromEdkc( dest, sizeofDest, rv.EdKcEnum );
   return rv;
   }

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
   // terminal specific values for shift + up / down
   const auto ch( wgetch(stdscr) );
   if( ch < 0 )                      { return -1; }
   if( ch <= 0xFF ) {
      if( ch == 27 )                 { return ConGetEscEvent(); }
      if( ch == '\r' || ch == '\n' ) { return EdKC_enter; }
      if( ch == '\t' )               { return EdKC_tab; }
      if( ch >= 28 && ch <= 31 )     { return EdKC_c_4 + (ch - 28); }
      if( ch < 27 )                  { return EdKC_c_a + (ch -  1); }
      return ch;
      }                          // KEY_F0 264   281-264
   if( s_to_EdKC[ ch ] ) {
      const auto rv( s_to_EdKC[ ch ] );
      DBG( "s_to_EdKC[ %d ]", ch );
      return rv;
      }
   switch (ch) { // > 0xFF      //                                   ubu 14.10             ubu 14.04             ubu 14.10             ubu 14.04
      CR(KEY_RIGHT, EdKC_right) // CR(KEY_SRIGHT , EdKC_s_right) CR(558, EdKC_a_right) CR(557, EdKC_a_right) CR(560, EdKC_c_right) CR(559, EdKC_c_right)
      CR(KEY_LEFT , EdKC_left ) // CR(KEY_SLEFT  , EdKC_s_left ) CR(543, EdKC_a_left ) CR(542, EdKC_a_left ) CR(545, EdKC_c_left ) CR(544, EdKC_c_left )
      CR(KEY_DC   , EdKC_del  ) // CR(KEY_SDC    , EdKC_s_del  ) CR(517, EdKC_a_del  ) CR(516, EdKC_a_del  ) CR(519, EdKC_c_del  ) CR(518, EdKC_c_del  )
      CR(KEY_IC   , EdKC_ins  ) // CR(KEY_SIC    , EdKC_s_ins  ) CR(538, EdKC_a_ins  ) CR(537, EdKC_a_ins  )
      CR(KEY_HOME , EdKC_home ) // CR(KEY_SHOME  , EdKC_s_home )
      CR(KEY_END  , EdKC_end  ) // CR(KEY_SEND   , EdKC_s_end  )
      CR(KEY_NPAGE, EdKC_pgdn ) // CR(KEY_SNEXT    , EdKC_s_pgdn) CR(553, EdKC_a_pgup) CR(552, EdKC_a_pgup)  CR(555, EdKC_c_pgup)  CR(554, EdKC_c_pgup)
      CR(KEY_PPAGE, EdKC_pgup ) // CR(KEY_SPREVIOUS, EdKC_s_pgup) CR(548, EdKC_a_pgdn) CR(547, EdKC_a_pgdn)  CR(550, EdKC_c_pgdn)  CR(549, EdKC_c_pgdn)
      CR(KEY_UP   , EdKC_up   ) //                                CR(564, EdKC_a_up  ) CR(563, EdKC_a_up  )  CR(566, EdKC_c_up  )  CR(565, EdKC_c_up  )
      CR(KEY_DOWN , EdKC_down ) //                                CR(523, EdKC_a_down) CR(522, EdKC_a_down)  CR(525, EdKC_c_down)  CR(524, EdKC_c_down)
      CR(KEY_BACKSPACE, EdKC_bksp)

   // CR(KEY_F(1) , EdKC_f1 ) CR(KEY_F(13), EdKC_s_f1 )   CR(KEY_F(25), EdKC_c_f1 ) CR(KEY_F(49), EdKC_a_f1 )
   // CR(KEY_F(2) , EdKC_f2 ) CR(KEY_F(14), EdKC_s_f2 )   CR(KEY_F(26), EdKC_c_f2 ) CR(KEY_F(50), EdKC_a_f2 )
   // CR(KEY_F(3) , EdKC_f3 ) CR(KEY_F(15), EdKC_s_f3 ) /*CR(KEY_F(27), EdKC_c_f3 ) CR(KEY_F(51), EdKC_a_f3 ) decoded elsewhere */
   // CR(KEY_F(4) , EdKC_f4 ) CR(KEY_F(16), EdKC_s_f4 ) /*CR(KEY_F(28), EdKC_c_f4 ) CR(KEY_F(52), EdKC_a_f4 ) decoded elsewhere */
   // CR(KEY_F(5) , EdKC_f5 ) CR(KEY_F(17), EdKC_s_f5 )   CR(KEY_F(29), EdKC_c_f5 ) CR(KEY_F(53), EdKC_a_f5 )
   // CR(KEY_F(6) , EdKC_f6 ) CR(KEY_F(18), EdKC_s_f6 )   CR(KEY_F(30), EdKC_c_f6 ) CR(KEY_F(54), EdKC_a_f6 )
   // CR(KEY_F(7) , EdKC_f7 ) CR(KEY_F(19), EdKC_s_f7 )   CR(KEY_F(31), EdKC_c_f7 ) CR(KEY_F(55), EdKC_a_f7 )
   // CR(KEY_F(8) , EdKC_f8 ) CR(KEY_F(20), EdKC_s_f8 )   CR(KEY_F(32), EdKC_c_f8 ) CR(KEY_F(56), EdKC_a_f8 )
   // CR(KEY_F(9) , EdKC_f9 ) CR(KEY_F(21), EdKC_s_f9 )   CR(KEY_F(33), EdKC_c_f9 ) CR(KEY_F(57), EdKC_a_f9 )
   // CR(KEY_F(10), EdKC_f10) CR(KEY_F(22), EdKC_s_f10)   CR(KEY_F(34), EdKC_c_f10) CR(KEY_F(58), EdKC_a_f10)
   // CR(KEY_F(11), EdKC_f11) CR(KEY_F(23), EdKC_s_f11)   CR(KEY_F(35), EdKC_c_f11) CR(KEY_F(59), EdKC_a_f11)
   // CR(KEY_F(12), EdKC_f12) CR(KEY_F(24), EdKC_s_f12)   CR(KEY_F(36), EdKC_c_f12) CR(KEY_F(60), EdKC_a_f12)

      // used in old termcap/infos
      CR(KEY_LL   , EdKC_end   )
      CR(KEY_B2   , EdKC_center)
      CR(KEY_ENTER, EdKC_enter ) // mimic Win32 behavior

      case KEY_RESIZE:   ConOut::Resize();  return -1;
      case KEY_MOUSE:  /*Event->What = evNone;
                         ConGetMouseEvent(Event);  break;
      case KEY_SF:       KEvent->Code = kfShift | kbDown;  break;
      case KEY_SR:       KEvent->Code = kfShift | kbUp;    break;
      case KEY_SRIGHT:   KEvent->Code = kfShift | kbRight; */
           Msg( "%s KEY_MOUSE event 0%o %d\n", __func__, ch, ch );
           return -1;

      default:
           // fprintf(stderr, "Unknown 0%o %d\n", ch, ch);
           Msg( "%s Unknown event 0%o %d\n", __func__, ch, ch );
           return -1;
      }
   }

struct kpto_er {
   kpto_er() {
      keypad(stdscr, 0);
      timeout(10);
      }
   ~kpto_er() {
      timeout(-1);
      keypad(stdscr, 1);
      }
   };

STATIC_FXN int ConGetEscEvent() {
   int result = -1;
   bool kbAlt = false;
   kpto_er kpto_cleaner;
   int ch = getch();
   if( ch == 27 ) { // ESC?
      ch = getch();
      if( ch == '[' || ch == 'O' ) {
         kbAlt = true;
         }
      }

   if( ch == ERR ) {
      result = kbAlt ? EdKC_a_esc : EdKC_esc;
      }
   else if (ch == '[' || ch == 'O') {
      int ch1 = getch();
      int endch = '\0';
      int modch = '\0';

      if (ch1 == ERR) { // translate to Alt-[ or Alt-O
         result = EdKC_a_LEFT_SQ;
         }
      else {
         if( ch1 >= '1' && ch1 <= '8' ) { // [n...
            endch = getch();
            if( endch == ERR) { // //[n, not valid
               // TODO, should this be ALT-7 ?
               endch = '\0';
               ch1 = '\0';
               }
            }
         else { // [A
            endch = ch1;
            ch1 = '\0';
            }

         if( endch == ';' ) { // [n;mX
            modch = getch();
            endch = getch();
            }
         else if (ch1 != '\0' && endch != '~' && endch != '$') { // [mA
            modch = ch1;
            ch1 = '\0';
            }

         auto mod( 0 );  enum {mod_ctrl=0x4,mod_alt=0x2,mod_shift=0x1};
         if( modch != '\0' ) {
            const int ctAlSh( ch1 - '1' );
            if( (ctAlSh & 0x4) || modch == 53) { mod |= mod_ctrl;  }
            if( (ctAlSh & 0x2) || modch == 51) { kbAlt = true;     }
            if( (ctAlSh & 0x1) || modch == 50) { mod |= mod_shift; }
            }
         if( kbAlt ) mod |= mod_alt;
         enum { mod_cas= 0          ,
                mod_caS= mod_shift  ,
                mod_cAs= mod_alt    ,
                mod_Cas= mod_ctrl   ,
                mod_CaS= mod_ctrl | mod_shift,
              };
         switch (endch) {
            default:  Msg( "%s unhandled event *cas=%X 0%o %d\n", __func__, mod, endch, endch ); return -1;
            case 'A': CAS5( up       ); break;
            case 'B': CAS5( down     ); break;
            case 'C': CAS5( right    ); break;
            case 'D': CAS5( left     ); break;
            case 'E': CAS5( center   ); break;
            case 'F': CAS5( end      ); break;
            case 'H': CAS5( home     ); break;
            case 'P': CAS5( f1       ); break;
            case 'Q': CAS5( f2       ); break;
            case 'R': CAS5( f3       ); break;
            case 'S': CAS5( f4       ); break;
            case 'j': CAS5( numStar  ); break;
            case 'k': CAS5( numPlus  ); break;
            case 'm': CAS5( numMinus ); break;
            case 'o': CAS5( numSlash ); break;
            case 'a': return (mod & mod_ctrl) ? EdKC_cs_up    : EdKC_s_up;
            case 'b': return (mod & mod_ctrl) ? EdKC_cs_down  : EdKC_s_down;
            case 'c': return (mod & mod_ctrl) ? EdKC_cs_right : EdKC_s_right;
            case 'd': return (mod & mod_ctrl) ? EdKC_cs_left  : EdKC_s_left;
            case '$': mod |= mod_shift;  /* FALL THRU!!! */
            case '~':
                switch (ch1 - '0') {
                   default: Msg( "%s unhandled event ~cas=%X 0%o %d\n", __func__, mod, ch1 - '0', ch1 - '0' ); return -1;
                   case 1: CAS5( home ); break;
                   case 7: CAS5( home ); break;
                   case 4: CAS5( end  ); break;
                   case 8: CAS5( end  ); break;
                   case 2: CAS5( ins  ); break;
                   case 3: CAS5( del  ); break;
                   case 5: CAS5( pgup ); break;
                   case 6: CAS5( pgdn ); break;
                   }
                break;
            }
      }
   } else { // alt+...
      if (ch == '\r' || ch == '\n')        { return EdKC_a_enter; }
      else if( ch == '\t' )                { return EdKC_a_tab;   }
      else if( ch < ' '   )                { Msg( "%s unhandled event alt+ 0%o %d\n", __func__, ch, ch ); return -1; } // alt + ctr + key;  unsupported by 'K'
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
         else if( ch == '['  )             { return EdKC_a_LEFT_SQ;        }
         else if( ch == '\\' )             { return EdKC_a_BACKSLASH;      }
         else if( ch == ']'  )             { return EdKC_a_RIGHT_SQ;       }
         else if( ch == '`'  )             { return EdKC_a_BACKTICK;       }
         else if( ch == 127  )             { return EdKC_a_bksp;           }
         else                              { return ch;                    }
         }
      }

   return result;
   }
