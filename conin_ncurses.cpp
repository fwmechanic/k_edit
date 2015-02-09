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
   static int key_sup = -1;
   static int key_sdown = -1;
   // fill terminal dependant values on first call
   if (key_sup < 0 || key_sdown < 0) {
      for (auto ch = KEY_MAX + 1 ; ; ch++ ) {
         auto kn( keyname(ch) );
         if (!kn) break;
         if (     !strcmp(kn, "kUP")) { key_sup   = ch; }
         else if (!strcmp(kn, "kDN")) { key_sdown = ch; }
         if (key_sup > 0 && key_sdown > 0) break;
         }
      }

   const auto ch( wgetch(stdscr) );
   if (ch < 0)                      { return -1; }
   if (ch <= 0xFF) {
      if (ch == 27)                 { return ConGetEscEvent(); }
      if (ch == '\r' || ch == '\n') { return EdKC_enter; }
      if (ch == '\t')               { return EdKC_tab; }
      if (ch > 27 && ch < 32)       { return EdKC_c_4 + (ch - 28); }
      if (ch < 27)                  { return EdKC_c_a + (ch - 1); }
      return ch;
      }
   switch (ch) { // > 0xFF
      CR(KEY_RIGHT, EdKC_right) CR(KEY_SRIGHT , EdKC_s_right) CR(558, EdKC_a_right) CR(560, EdKC_c_right)
      CR(KEY_LEFT , EdKC_left ) CR(KEY_SLEFT  , EdKC_s_left ) CR(543, EdKC_a_left ) CR(545, EdKC_c_left )
      CR(KEY_DC   , EdKC_del  ) CR(KEY_SDC    , EdKC_s_del  ) CR(517, EdKC_a_del  ) CR(519, EdKC_c_del  )
      CR(KEY_IC   , EdKC_ins  ) CR(KEY_SIC    , EdKC_s_ins  )                       CR(538, EdKC_a_ins )
      CR(KEY_HOME , EdKC_home ) CR(KEY_SHOME  , EdKC_s_home )
      CR(KEY_END  , EdKC_end  ) CR(KEY_SEND   , EdKC_s_end  )
      CR(KEY_LL   , EdKC_end  ) // used in old termcap/infos
      CR(KEY_NPAGE, EdKC_pgdn ) CR(KEY_SNEXT    , EdKC_s_pgdn) CR(553, EdKC_a_pgup) CR(555, EdKC_c_pgup)
      CR(KEY_PPAGE, EdKC_pgup ) CR(KEY_SPREVIOUS, EdKC_s_pgup) CR(548, EdKC_a_pgdn) CR(550, EdKC_c_pgdn)
      CR(KEY_UP   , EdKC_up   )                                CR(564, EdKC_a_up  ) CR(566, EdKC_c_up  )
      CR(KEY_DOWN , EdKC_down )                                CR(523, EdKC_a_down) CR(525, EdKC_c_down)
      CR(KEY_BACKSPACE, EdKC_bksp)
      CR(KEY_F(1) , EdKC_f1 )                     CR(289, EdKC_c_f1 ) CR(313, EdKC_a_f1 )
      CR(KEY_F(2) , EdKC_f2 )                     CR(290, EdKC_c_f2 ) CR(314, EdKC_a_f2 )
      CR(KEY_F(3) , EdKC_f3 )                  /* CR(291, EdKC_c_f3 ) CR(315, EdKC_a_f3 ) decoded elsewhere */
      CR(KEY_F(4) , EdKC_f4 )                  /* CR(292, EdKC_c_f4 ) CR(316, EdKC_a_f4 ) decoded elsewhere */
      CR(KEY_F(5) , EdKC_f5 ) CR(281, EdKC_s_f5 ) CR(293, EdKC_c_f5 ) CR(317, EdKC_a_f5 )
      CR(KEY_F(6) , EdKC_f6 ) CR(282, EdKC_s_f6 ) CR(294, EdKC_c_f6 ) CR(318, EdKC_a_f6 )
      CR(KEY_F(7) , EdKC_f7 ) CR(283, EdKC_s_f7 ) CR(295, EdKC_c_f7 ) CR(319, EdKC_a_f7 )
      CR(KEY_F(8) , EdKC_f8 ) CR(284, EdKC_s_f8 ) CR(296, EdKC_c_f8 ) CR(320, EdKC_a_f8 )
      CR(KEY_F(9) , EdKC_f9 ) CR(285, EdKC_s_f9 ) CR(297, EdKC_c_f9 ) CR(321, EdKC_a_f9 )
      CR(KEY_F(10), EdKC_f10) CR(286, EdKC_s_f10) CR(298, EdKC_c_f10) CR(322, EdKC_a_f10)
      CR(KEY_F(11), EdKC_f11) CR(287, EdKC_s_f11) CR(299, EdKC_c_f11) CR(323, EdKC_a_f11)
      CR(KEY_F(12), EdKC_f12) CR(288, EdKC_s_f12) CR(300, EdKC_c_f12) CR(324, EdKC_a_f12)

      CR(KEY_B2   , EdKC_center)
      CR(KEY_ENTER, EdKC_enter ) // mimic Win32 behavior

      case KEY_RESIZE:     ConOut::Resize();  return -1;
      case KEY_MOUSE:
           /*Event->What = evNone;
           ConGetMouseEvent(Event);
           break;
               case KEY_SF:
                       KEvent->Code = kfShift | kbDown;
                       break;
               case KEY_SR:
                       KEvent->Code = kfShift | kbUp;
                       break;
               case KEY_SRIGHT:
           KEvent->Code = kfShift | kbRight; */
           DBG( "%s KEY_MOUSE event 0x%X %d\n", __func__, ch, ch );
           return -1;

      default:
           if (key_sdown != -1 && ch == key_sdown) { return EdKC_s_down; }
           if (key_sup != 0 && ch == key_sup)      { return EdKC_s_up;   }
           // fprintf(stderr, "Unknown 0x%x %d\n", ch, ch);
           DBG( "%s Unknown event 0x%X %d\n", __func__, ch, ch );
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
            default:  DBG( "%s unhandled event *cas=%X 0x%X %d\n", __func__, mod, endch, endch ); return -1;
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
            case 'a':
                if (!(mod & mod_ctrl))     { return EdKC_s_up;  }
                else if ((mod & mod_ctrl)) { return EdKC_cs_up; }
                else                       { return -1;         }
                break;
            case 'b':
                if (!(mod & mod_ctrl))     { return EdKC_s_down;  }
                else if ((mod & mod_ctrl)) { return EdKC_cs_down; }
                else                       { return -1;           }
                break;
            case 'c':
                if (!(mod & mod_ctrl))     { return EdKC_s_right;  }
                else if ((mod & mod_ctrl)) { return EdKC_cs_right; }
                else                       { return -1;            }
                break;
            case 'd':
                if (!(mod & mod_ctrl))     { return EdKC_s_left;  }
                else if ((mod & mod_ctrl)) { return EdKC_cs_left; }
                else                       { return -1;           }
                break;
            case '$': mod |= mod_shift;  /* FALL THRU!!! */
            case '~':
                switch (ch1 - '0') {
                   default: DBG( "%s unhandled event ~cas=%X 0x%X %d\n", __func__, mod, ch1 - '0', ch1 - '0' ); return -1;
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
      if (ch == '\r' || ch == '\n') { return EdKC_a_enter; }
      else if( ch == '\t' )         { return EdKC_a_tab;   }
      else if( ch < ' '   )         { DBG( "%s unhandled event alt+ 0x%X %d\n", __func__, ch, ch ); return -1; } // alt + ctr + key;  unsupported by 'K'
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
