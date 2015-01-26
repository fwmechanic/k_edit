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

bool ConIO::Confirm( PCChar pszPrompt, ... ){ return false; }

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
      case KEY_SLEFT     : return EdKC_s_left   ;
      case KEY_SDC       : return EdKC_s_del    ;
      case KEY_SIC       : return EdKC_s_ins    ;
      case KEY_SHOME     : return EdKC_s_home   ;
      case KEY_SEND      : return EdKC_s_end    ;
      case KEY_SNEXT     : return EdKC_s_pgdn   ;
      case KEY_SPREVIOUS : return EdKC_s_pgup   ;
      case KEY_UP        : return EdKC_up       ;
      case KEY_DOWN      : return EdKC_down     ;
      case KEY_RIGHT     : return EdKC_right    ;
      case KEY_LEFT      : return EdKC_left     ;
      case KEY_DC        : return EdKC_del      ;
      case KEY_IC        : return EdKC_ins      ;
      case KEY_BACKSPACE : return EdKC_bksp     ;
      case KEY_HOME      : return EdKC_home     ;
      case KEY_END       : return EdKC_end      ;
      case KEY_LL        : return EdKC_end      ; // used in old termcap/infos
      case KEY_NPAGE     : return EdKC_pgdn     ;
      case KEY_PPAGE     : return EdKC_pgup     ;
      case KEY_F(1)      : return EdKC_f1       ;
      case KEY_F(2)      : return EdKC_f2       ;
      case KEY_F(3)      : return EdKC_f3       ;
      case KEY_F(4)      : return EdKC_f4       ;
      case KEY_F(5)      : return EdKC_f5       ;
      case KEY_F(6)      : return EdKC_f6       ;
      case KEY_F(7)      : return EdKC_f7       ;
      case KEY_F(8)      : return EdKC_f8       ;
      case KEY_F(9)      : return EdKC_f9       ;
      case KEY_F(10)     : return EdKC_f10      ;
      case KEY_F(11)     : return EdKC_f11      ;
      case KEY_F(12)     : return EdKC_f12      ;
      case KEY_B2        : return EdKC_center   ;
      case KEY_ENTER     : return EdKC_enter    ; // mimic Win32 behavior
      case 560           : return EdKC_c_right  ; // ctr + right
      case 545           : return EdKC_c_left   ; // ctr + left
      case 566           : return EdKC_c_up     ; // ctr + up
      case 525           : return EdKC_c_down   ; // ctr + down
      case 558           : return EdKC_a_right  ; // alt + right
      case 543           : return EdKC_a_left   ; // alt + left
      case 564           : return EdKC_a_up     ; // alt + up
      case 523           : return EdKC_a_down   ; // alt + down
      case 553           : return EdKC_a_pgup   ; // alt + pgup
      case 548           : return EdKC_a_pgdn   ; // alt + pgdown
      case 538           : return EdKC_a_ins    ; // alt + ins
      case 517           : return EdKC_a_del    ; // alt + del
      case 519           : return EdKC_c_del    ; // ctr + del

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
           return -1;

      default:
           if (key_sdown != -1 && ch == key_sdown) { return EdKC_s_down; }
           if (key_sup != 0 && ch == key_sup)      { return EdKC_s_up;   }
           // fprintf(stderr, "Unknown 0x%x %d\n", ch, ch);
           DBG( "%s Unknown event 0x%X %d\n", __func__, ch, ch );
           return -1;
      }
   return -1;
   }

STATIC_FXN int ConGetEscEvent() {
    int result = -1;
    int ch;
    bool kbAlt = false;

    keypad(stdscr, 0);

    timeout(10);

    ch = getch();
    if (ch == 033) {
        ch = getch();
        if (ch == '[' || ch == 'O') {
            kbAlt = true;
        }
    }

    if (ch == ERR) {
        if (kbAlt) {
             result = EdKC_a_esc;
        } else {
             result = EdKC_esc;
        }
    } else if (ch == '[' || ch == 'O') {
        int ch1 = getch();
        int endch = '\0';
        int modch = '\0';

        if (ch1 == ERR) { // translate to Alt-[ or Alt-O
            result = EdKC_a_LEFT_SQ;
        } else {
            if (ch1 >= '1' && ch1 <= '8') { // [n...
                endch = getch();
                if (endch == ERR) { // //[n, not valid
                    // TODO, should this be ALT-7 ?
                    endch = '\0';
                    ch1 = '\0';
                }
            } else { // [A
                endch = ch1;
                ch1 = '\0';
            }

            if (endch == ';') { // [n;mX
                modch = getch();
                endch = getch();
            } else if (ch1 != '\0' && endch != '~' && endch != '$') { // [mA
                modch = ch1;
                ch1 = '\0';
            }

            auto mod( 0 );  enum {mod_ctrl=0x4,mod_alt=0x2,mod_shift=0x1};
            if (modch != '\0') {
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
               default: result = -1; break;
               case 'A':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_up;     break;
                   case mod_Cas:   result = EdKC_c_up;   break;
                   case mod_cAs:   result = EdKC_a_up;   break;
                   case mod_caS:   result = EdKC_s_up;   break;
                   case mod_CaS:   result = EdKC_cs_up;  break;
                   default:        result = -1;          break;
                   }
                   break;
               case 'B':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_down;     break;
                   case mod_Cas:   result = EdKC_c_down;   break;
                   case mod_cAs:   result = EdKC_a_down;   break;
                   case mod_caS:   result = EdKC_s_down;   break;
                   case mod_CaS:   result = EdKC_cs_down;  break;
                   default:        result = -1;            break;
                   }
                   break;
               case 'C':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_right;    break;
                   case mod_Cas:   result = EdKC_c_right;  break;
                   case mod_cAs:   result = EdKC_a_right;  break;
                   case mod_caS:   result = EdKC_s_right;  break;
                   case mod_CaS:   result = EdKC_cs_right; break;
                   default:        result = -1;            break;
                   }
                   break;
               case 'D':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_left;      break;
                   case mod_Cas:   result = EdKC_c_left;    break;
                   case mod_cAs:   result = EdKC_a_left;    break;
                   case mod_caS:   result = EdKC_s_left;    break;
                   case mod_CaS:   result = EdKC_cs_left;   break;
                   default:        result = -1;             break;
                   }
                   break;
               case 'E':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_center;    break;
                   case mod_Cas:   result = EdKC_c_center;  break; // IMPOSSIBLE
                   case mod_cAs:   result = EdKC_a_center;  break; // IMPOSSIBLE
                   case mod_caS:   result = EdKC_s_center;  break;
                   case mod_CaS:   result = EdKC_cs_center; break;
                   default:        result = -1;             break;
                   }
                   break;
               case 'F':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_end;       break;
                   case mod_Cas:   result = EdKC_c_end;     break; // IMPOSSIBLE
                   case mod_cAs:   result = EdKC_a_end;     break;
                   case mod_caS:   result = EdKC_s_end;     break;
                   case mod_CaS:   result = EdKC_cs_end;    break;
                   default:        result = -1;             break;
                   }
                   break;
               case 'H':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_home;      break;
                   case mod_Cas:   result = EdKC_c_home;    break; // IMPOSSIBLE
                   case mod_cAs:   result = EdKC_a_home;    break;
                   case mod_caS:   result = EdKC_s_home;    break;
                   case mod_CaS:   result = EdKC_cs_home;   break;
                   default:        result = -1;             break;
                   }
                   break;
               case 'R':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_f3;        break;
                   case mod_Cas:   result = EdKC_c_f3;      break;
                   case mod_cAs:   result = EdKC_a_f3;      break;
                   case mod_caS:   result = EdKC_s_f3;      break;
                   case mod_CaS:   result = EdKC_cs_f3;     break;
                   default:        result = -1;             break;
                   }
                   break;
               case 'S':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_f4;        break;
                   case mod_Cas:   result = EdKC_c_f4;      break;
                   case mod_cAs:   result = EdKC_a_f4;      break;
                   case mod_caS:   result = EdKC_s_f4;      break;
                   case mod_CaS:   result = EdKC_cs_f4;     break;
                   default:        result = -1;             break;
                   }
                   break;
               case 'j':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_numStar;    break;
                   case mod_Cas:   result = EdKC_c_numStar;  break;
                   case mod_cAs:   result = EdKC_a_numStar;  break;
                   case mod_caS:   result = EdKC_s_numStar;  break;
                   case mod_CaS:   result = EdKC_cs_numStar; break;
                   default:        result = -1;              break;
                   }
                   break;
               case 'k':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_numPlus;    break;
                   case mod_Cas:   result = EdKC_c_numPlus;  break;
                   case mod_cAs:   result = EdKC_a_numPlus;  break;
                   case mod_caS:   result = EdKC_s_numPlus;  break;
                   case mod_CaS:   result = EdKC_cs_numPlus; break;
                   default:        result = -1;              break;
                   }
                   break;
               case 'm':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_numMinus;    break;
                   case mod_Cas:   result = EdKC_c_numMinus;  break;
                   case mod_cAs:   result = EdKC_a_numMinus;  break;
                   case mod_caS:   result = EdKC_s_numMinus;  break;
                   case mod_CaS:   result = EdKC_cs_numMinus; break;
                   default:        result = -1;               break;
                   }
                   break;
               case 'o':
                   switch( mod ) {
                   case mod_cas:   result = EdKC_numSlash;    break;
                   case mod_Cas:   result = EdKC_c_numSlash;  break;
                   case mod_cAs:   result = EdKC_a_numSlash;  break;
                   case mod_caS:   result = EdKC_s_numSlash;  break;
                   case mod_CaS:   result = EdKC_cs_numSlash; break;
                   default:        result = -1;               break;
                   }
                   break;
               case 'a':
                   if (!(mod & mod_ctrl))       { result = EdKC_s_up;
                   } else if ((mod & mod_ctrl)) { result = EdKC_cs_up;
                   } else                       { result = -1;
                   }
                   break;
               case 'b':
                   if (!(mod & mod_ctrl))       { result = EdKC_s_down;
                   } else if ((mod & mod_ctrl)) { result = EdKC_cs_down;
                   } else                       { result = -1;
                   }
                   break;
               case 'c':
                   if (!(mod & mod_ctrl))       { result = EdKC_s_right;
                   } else if ((mod & mod_ctrl)) { result = EdKC_cs_right;
                   } else                       { result = -1;
                   }
                   break;
               case 'd':
                   if (!(mod & mod_ctrl))       { result = EdKC_s_left;
                   } else if ((mod & mod_ctrl)) { result = EdKC_cs_left;
                   } else                       { result = -1;
                   }
                   break;
               case '$': mod |= mod_shift;  /* FALL THRU!!! */
               case '~':
                   switch (ch1 - '0') {
                   default: result = -1; break;
                   case 1:
                       switch( mod ) {
                       case mod_cas:   result = EdKC_home;    break;
                       case mod_Cas:   result = EdKC_c_home;  break; // IMPOSSIBLE
                       case mod_cAs:   result = EdKC_a_home;  break;
                       case mod_caS:   result = EdKC_s_home;  break;
                       case mod_CaS:   result = EdKC_cs_home; break;
                       default:        result = -1;           break;
                       }
                       break;
                   case 2:
                       switch( mod ) {
                       case mod_cas:   result = EdKC_ins;     break;
                       case mod_Cas:   result = EdKC_c_ins;   break; // IMPOSSIBLE
                       case mod_cAs:   result = EdKC_a_ins;   break;
                       case mod_caS:   result = EdKC_s_ins;   break;
                       case mod_CaS:   result = EdKC_cs_ins;  break;
                       default:        result = -1;           break;
                       }
                       break;
                   case 3:
                       switch( mod ) {
                       case mod_cas:   result = EdKC_del;     break;
                       case mod_Cas:   result = EdKC_c_del;   break;
                       case mod_cAs:   result = EdKC_a_del;   break;
                       case mod_caS:   result = EdKC_s_del;   break;
                       case mod_CaS:   result = -1;           break;
                       default:        result = -1;           break;
                       }
                       break;
                   case 4:
                       switch( mod ) {
                       case mod_cas:   result = EdKC_end;     break;
                       case mod_Cas:   result = EdKC_c_end;   break; // IMPOSSIBLE
                       case mod_cAs:   result = EdKC_a_end;   break;
                       case mod_caS:   result = EdKC_s_end;   break;
                       case mod_CaS:   result = EdKC_cs_end;  break;
                       default:        result = -1;           break;
                       }
                       break;
                   case 5:
                       switch( mod ) {
                       case mod_cas:   result = EdKC_pgup;    break;
                       case mod_Cas:   result = EdKC_c_pgup;  break; // IMPOSSIBLE
                       case mod_cAs:   result = EdKC_a_pgup;  break;
                       case mod_caS:   result = EdKC_s_pgup;  break;
                       case mod_CaS:   result = EdKC_cs_pgup; break;
                       default:        result = -1;           break;
                       }
                       break;
                   case 6:
                       switch( mod ) {
                       case mod_cas:   result = EdKC_pgdn;    break;
                       case mod_Cas:   result = EdKC_c_pgdn;  break; // IMPOSSIBLE
                       case mod_cAs:   result = EdKC_a_pgdn;  break;
                       case mod_caS:   result = EdKC_s_pgdn;  break;
                       case mod_CaS:   result = EdKC_cs_pgdn; break;
                       default:        result = -1;           break;
                       }
                       break;
                   case 7:
                       switch( mod ) {
                       case mod_cas:   result = EdKC_home;    break;
                       case mod_Cas:   result = EdKC_c_home;  break;
                       case mod_cAs:   result = EdKC_a_home;  break; // IMPOSSIBLE
                       case mod_caS:   result = EdKC_s_home;  break;
                       case mod_CaS:   result = EdKC_cs_home; break;
                       default:        result = -1;           break;
                       }
                       break;
                   case 8:
                       switch( mod ) {
                       case mod_cas:   result = EdKC_end;     break;
                       case mod_Cas:   result = EdKC_c_end;   break;
                       case mod_cAs:   result = EdKC_a_end;   break; // IMPOSSIBLE
                       case mod_caS:   result = EdKC_s_end;   break;
                       case mod_CaS:   result = EdKC_cs_end;  break;
                       default:        result = -1;           break;
                       }
                       break;
                   }
                   break;
               }
        }
    } else {
        if (ch == '\r' || ch == '\n') {  result = EdKC_a_enter;
        } else if (ch == '\t') {         result = EdKC_a_tab;
        } else if (ch < 32) {            result = -1;  // alt + ctr + key;  unsupported by 'K'
        } else { // alt+numbers
            if( ch >= '0' && ch <= '9' ) { result = EdKC_a_0 + (ch - '0');
            } else if( (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
                if( ch >= 'A' && ch <= 'Z' ) {
                    ch += 'a' - 'A';  // Alt-A == Alt-a
                }
                result = EdKC_a_a + (ch - 'a');
            } else if (ch == '\'') { result = EdKC_a_TICK;
            } else if (ch == ',' ) { result = EdKC_a_COMMA;
            } else if (ch == '-' ) { result = EdKC_a_MINUS;
            } else if (ch == '.' ) { result = EdKC_a_DOT;
            } else if (ch == '/' ) { result = EdKC_a_SLASH;
            } else if (ch == ';' ) { result = EdKC_a_SEMICOLON;
            } else if (ch == '=' ) { result = EdKC_a_EQUAL;
            } else if (ch == '[' ) { result = EdKC_a_LEFT_SQ;
            } else if (ch == '\\') { result = EdKC_a_BACKSLASH;
            } else if (ch == ']' ) { result = EdKC_a_RIGHT_SQ;
            } else if (ch == '`' ) { result = EdKC_a_BACKTICK;
            } else if (ch == 127 ) { result = EdKC_a_bksp;
            } else                 { result = ch;
            }
        }
    }

    timeout(-1);
    keypad(stdscr, 1);

    return result;
}
