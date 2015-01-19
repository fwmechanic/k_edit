//
// Copyright 2014 - 2015 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include <ncurses.h>
#include <stdio.h>
#include "conio.h"
#include "ed_main.h"

// get keyboard event
int ConGetEvent();
// get extended event (more komplex keystrokes)
int ConGetEscEvent();

void ConIn::WaitForKey(){}

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

EdKC_Ascii ConIn::EdKC_Ascii_FromNextKey() {
   return GetEdKC_Ascii( false );
   }

EdKC_Ascii ConIn::EdKC_Ascii_FromNextKey_Keystr( PChar dest, size_t sizeofDest ) {
   const auto rv( GetEdKC_Ascii( false ) );
   StrFromEdkc( dest, sizeofDest, rv.EdKcEnum );
   return rv;
   }

// return -1 indicates that event should be ignored (resize event as an example)
int ConGetEvent() {
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
      if (ch <  32)                 { return EdKC_c_a + (ch - 1); }
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
      case KEY_ENTER     : return EdKC_numEnter ; // shift enter
      case 560           : return EdKC_c_right  ; // ctr + right
      case 545           : return EdKC_c_left   ; // ctr + left
      case 566           : return EdKC_c_up     ; // ctr + up
      case 525           : return EdKC_c_down   ; // ctr + down
      case 558           : return EdKC_a_right  ; // alt + right
      case 543           : return EdKC_a_left   ; // alt + left

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
           return -1;
      }
   return -1;
   }

int ConGetEscEvent() {
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
        bool kbCtr = false;
        bool kbSft = false;
        int ch1 = getch();
        int endch = '\0';
        int modch = '\0';

        if (ch1 == ERR) { // translate to Alt-[ or Alt-O
            // TODO check
            result = EdKC_a_a + (ch - 1);
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

            if (modch != '\0') {
                int ctAlSh = ch1 - '1';
                if (ctAlSh & 0x4) kbCtr = true;
                if (ctAlSh & 0x2) kbAlt = true;
                if (ctAlSh & 0x1) kbSft = true;
                if (modch == 51) kbAlt = true;
                if (modch == 53) kbCtr = true;
                if (modch == 50) kbSft = true;
            }

            switch (endch) {

            case 'A':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_up;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_up;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_up;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_up;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_up;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'B':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_down;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_down;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_down;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_down;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_down;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'C':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_right;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_right;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_right;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_right;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_right;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'D':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_left;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_left;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_left;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_left;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_left;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'E':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_center;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_center;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_center;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_center;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_center;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'F':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_end;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_end;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_end;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_end;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_end;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'H':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_home;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_home;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_home;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_home;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_home;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'R':

                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_f3;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_f3;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_f3;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_f3;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_f3;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'S':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_f4;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_f4;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_f4;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_f4;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_f4;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'j':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_numStar;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_numStar;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_numStar;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_numStar;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_numStar;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'k':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_numPlus;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_numPlus;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_numPlus;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_numPlus;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_numPlus;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'm':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_numMinus;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_numMinus;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_numMinus;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_numMinus;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_numMinus;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'o':
                if (!kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_numSlash;
                } else if (kbCtr && !kbAlt && !kbSft) {
                    result = EdKC_c_numSlash;
                } else if (!kbCtr && kbAlt && !kbSft) {
                    result = EdKC_a_numSlash;
                } else if (!kbCtr && !kbAlt && kbSft) {
                    result = EdKC_s_numSlash;
                } else if (kbCtr && !kbAlt && kbSft) {
                    result = EdKC_cs_numSlash;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;

            case 'a':
                if (!kbCtr) {
                    result = EdKC_s_up;
                } else if (kbCtr) {
                    result = EdKC_cs_up;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'b':
                if (!kbCtr) {
                    result = EdKC_s_down;
                } else if (kbCtr) {
                    result = EdKC_cs_down;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'c':
                if (!kbCtr) {
                    result = EdKC_s_right;
                } else if (kbCtr) {
                    result = EdKC_cs_right;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case 'd':
                if (!kbCtr) {
                    result = EdKC_s_left;
                } else if (kbCtr) {
                    result = EdKC_cs_left;
                } else {
                    // unsupported by 'K'
                    result = -1;
                }
                break;
            case '$':
                kbSft = true;
            case '~':
                switch (ch1 - '0') {
                case 1:
                    if (!kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_home;
                    } else if (kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_c_home;
                    } else if (!kbCtr && kbAlt && !kbSft) {
                        result = EdKC_a_home;
                    } else if (!kbCtr && !kbAlt && kbSft) {
                        result = EdKC_s_home;
                    } else if (kbCtr && !kbAlt && kbSft) {
                        result = EdKC_cs_home;
                    } else {
                        // unsupported by 'K'
                        result = -1;
                    }
                    break;
                case 2:
                    if (!kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_ins;
                    } else if (kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_c_ins;
                    } else if (!kbCtr && kbAlt && !kbSft) {
                        result = EdKC_a_ins;
                    } else if (!kbCtr && !kbAlt && kbSft) {
                        result = EdKC_s_ins;
                    } else if (kbCtr && !kbAlt && kbSft) {
                        result = EdKC_cs_ins;
                    } else {
                        // unsupported by 'K'
                        result = -1;
                    }
                    break;
                case 3:
                    if (!kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_del;
                    } else if (kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_c_del;
                    } else if (!kbCtr && kbAlt && !kbSft) {
                        result = EdKC_a_del;
                    } else if (!kbCtr && !kbAlt && kbSft) {
                        result = EdKC_s_del;
                    } else if (kbCtr && !kbAlt && kbSft) {
                        // unsupported by 'K'
                        result = -1;
                    } else {
                        // unsupported by 'K'
                        result = -1;
                    }
                    break;
                case 4:
                    if (!kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_end;
                    } else if (kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_c_end;
                    } else if (!kbCtr && kbAlt && !kbSft) {
                        result = EdKC_a_end;
                    } else if (!kbCtr && !kbAlt && kbSft) {
                        result = EdKC_s_end;
                    } else if (kbCtr && !kbAlt && kbSft) {
                        result = EdKC_cs_end;
                    } else {
                        // unsupported by 'K'
                        result = -1;
                    }
                    break;
                case 5:
                    if (!kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_pgup;
                    } else if (kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_c_pgup;
                    } else if (!kbCtr && kbAlt && !kbSft) {
                        result = EdKC_a_pgup;
                    } else if (!kbCtr && !kbAlt && kbSft) {
                        result = EdKC_s_pgup;
                    } else if (kbCtr && !kbAlt && kbSft) {
                        result = EdKC_cs_pgup;
                    } else {
                        // unsupported by 'K'
                        result = -1;
                    }
                    break;
                case 6:
                    if (!kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_pgdn;
                    } else if (kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_c_pgdn;
                    } else if (!kbCtr && kbAlt && !kbSft) {
                        result = EdKC_a_pgdn;
                    } else if (!kbCtr && !kbAlt && kbSft) {
                        result = EdKC_s_pgdn;
                    } else if (kbCtr && !kbAlt && kbSft) {
                        result = EdKC_cs_pgdn;
                    } else {
                        // unsupported by 'K'
                        result = -1;
                    }
                    break;
                case 7:
                    if (!kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_home;
                    } else if (kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_c_home;
                    } else if (!kbCtr && kbAlt && !kbSft) {
                        result = EdKC_a_home;
                    } else if (!kbCtr && !kbAlt && kbSft) {
                        result = EdKC_s_home;
                    } else if (kbCtr && !kbAlt && kbSft) {
                        result = EdKC_cs_home;
                    } else {
                        // unsupported by 'K'
                        result = -1;
                    }
                    break;
                case 8:
                    if (!kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_end;
                    } else if (kbCtr && !kbAlt && !kbSft) {
                        result = EdKC_c_end;
                    } else if (!kbCtr && kbAlt && !kbSft) {
                        result = EdKC_a_end;
                    } else if (!kbCtr && !kbAlt && kbSft) {
                        result = EdKC_s_end;
                    } else if (kbCtr && !kbAlt && kbSft) {
                        result = EdKC_cs_end;
                    } else {
                        // unsupported by 'K'
                        result = -1;
                    }
                    break;
                default:
                    result = -1;
                    break;
                }
                break;
            default:
                result = -1;
                break;
            }
        }
    } else {

        if (ch == '\r' || ch == '\n') {
            result = EdKC_a_enter;
        } else if (ch == '\t') {
            result = EdKC_a_tab;
        } else if (ch < 32) {
            // alt + ctr + key
            // seems to be unsupported by 'K'
            result = -1;
        } else {
            // alt+numbers
            if (47 < ch && ch < 58) {
                result = EdKC_a_0 + (ch - 48);
            } else if ((ch > 64 && ch < 91) || (ch > 96 && ch < 123)) {
                if (ch > 64 && ch < 91) {
                    /* Alt-A == Alt-a*/
                    ch += 32;
                }
                result = EdKC_a_a + (ch - 97);
            } else if (ch == 127) {
                result = EdKC_a_bksp;
            } else {
                result = ch;
            }
        }
    }

    timeout(-1);
    keypad(stdscr, 1);

    return result;
}
