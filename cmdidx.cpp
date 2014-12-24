//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"
#include "krbtree.h"

GLOBAL_VAR CMD g_CmdTable[] = { // *** THIS IS GLOBAL, NOT STATIC, BECAUSE #defines of pCMD_xxx in cmdtbl.kh refer to it
//-----------------------------------
#define CMDTBL_H_CMD_TBL_INITIALIZERS
#include "cmdtbl.h" // <-- Lua-generated array initializers and #defines
#undef  CMDTBL_H_CMD_TBL_INITIALIZERS
//-----------------------------------
   };

#ifdef GCC
#define  IDX_EQ( idx_val )  [ idx_val ] =
#else
#define  IDX_EQ( idx_val )
#endif


// nearly identical declarations:
#if 1
GLOBAL_VAR PCMD     g_Key2CmdTbl[] =         // use this so assert @ end of initializer will work
#else
GLOBAL_VAR AKey2Cmd g_Key2CmdTbl   =         // use this to prove it (still) works
#endif
   {
   // first 256 [0x00..0xFF] are for ASCII codes
   #define  gfc   pCMD_graphic
   // 0..3
     gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc

   // 4..7
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc

   // 8..11
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc

   // 12..15
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc

   #undef  gfc

#if SEL_KEYMAP
   , IDX_EQ( EdKC_sela         )  pCMD_graphic   // a
   , IDX_EQ( EdKC_selb         )  pCMD_graphic   // b
   , IDX_EQ( EdKC_selc         )  pCMD_graphic   // c
   , IDX_EQ( EdKC_seld         )  pCMD_graphic   // d
   , IDX_EQ( EdKC_sele         )  pCMD_graphic   // e
   , IDX_EQ( EdKC_self         )  pCMD_graphic   // f
   , IDX_EQ( EdKC_selg         )  pCMD_graphic   // g
   , IDX_EQ( EdKC_selh         )  pCMD_left      // h (left)
   , IDX_EQ( EdKC_seli         )  pCMD_graphic   // i
   , IDX_EQ( EdKC_selj         )  pCMD_down      // j (down)
   , IDX_EQ( EdKC_selk         )  pCMD_up        // k (up)
   , IDX_EQ( EdKC_sell         )  pCMD_right     // l (right)
   , IDX_EQ( EdKC_selm         )  pCMD_graphic   // m
   , IDX_EQ( EdKC_seln         )  pCMD_graphic   // n
   , IDX_EQ( EdKC_selo         )  pCMD_graphic   // o
   , IDX_EQ( EdKC_selp         )  pCMD_graphic   // p
   , IDX_EQ( EdKC_selq         )  pCMD_graphic   // q
   , IDX_EQ( EdKC_selr         )  pCMD_graphic   // r
   , IDX_EQ( EdKC_sels         )  pCMD_graphic   // s
   , IDX_EQ( EdKC_selt         )  pCMD_graphic   // t
   , IDX_EQ( EdKC_selu         )  pCMD_graphic   // u
   , IDX_EQ( EdKC_selv         )  pCMD_graphic   // v
   , IDX_EQ( EdKC_selw         )  pCMD_graphic   // w
   , IDX_EQ( EdKC_selx         )  pCMD_graphic   // x
   , IDX_EQ( EdKC_sely         )  pCMD_graphic   // y
   , IDX_EQ( EdKC_selz         )  pCMD_graphic   // z
   , IDX_EQ( EdKC_selA         )  pCMD_graphic   // A
   , IDX_EQ( EdKC_selB         )  pCMD_graphic   // B
   , IDX_EQ( EdKC_selC         )  pCMD_graphic   // C
   , IDX_EQ( EdKC_selD         )  pCMD_graphic   // D
   , IDX_EQ( EdKC_selE         )  pCMD_graphic   // E
   , IDX_EQ( EdKC_selF         )  pCMD_graphic   // F
   , IDX_EQ( EdKC_selG         )  pCMD_graphic   // G
   , IDX_EQ( EdKC_selH         )  pCMD_graphic   // H
   , IDX_EQ( EdKC_selI         )  pCMD_graphic   // I
   , IDX_EQ( EdKC_selJ         )  pCMD_graphic   // J
   , IDX_EQ( EdKC_selK         )  pCMD_graphic   // K
   , IDX_EQ( EdKC_selL         )  pCMD_graphic   // L
   , IDX_EQ( EdKC_selM         )  pCMD_graphic   // M
   , IDX_EQ( EdKC_selN         )  pCMD_graphic   // N
   , IDX_EQ( EdKC_selO         )  pCMD_graphic   // O
   , IDX_EQ( EdKC_selP         )  pCMD_graphic   // P
   , IDX_EQ( EdKC_selQ         )  pCMD_graphic   // Q
   , IDX_EQ( EdKC_selR         )  pCMD_graphic   // R
   , IDX_EQ( EdKC_selS         )  pCMD_graphic   // S
   , IDX_EQ( EdKC_selT         )  pCMD_graphic   // T
   , IDX_EQ( EdKC_selU         )  pCMD_graphic   // U
   , IDX_EQ( EdKC_selV         )  pCMD_graphic   // V
   , IDX_EQ( EdKC_selW         )  pCMD_graphic   // W
   , IDX_EQ( EdKC_selX         )  pCMD_graphic   // X
   , IDX_EQ( EdKC_selY         )  pCMD_graphic   // Y
   , IDX_EQ( EdKC_selZ         )  pCMD_graphic   // Z
#endif // SEL_KEYMAP

   , IDX_EQ( EdKC_f1           )  pCMD_unassigned
   , IDX_EQ( EdKC_f2           )  pCMD_setfile
   , IDX_EQ( EdKC_f3           )  pCMD_psearch
   , IDX_EQ( EdKC_f4           )  pCMD_msearch
   , IDX_EQ( EdKC_f5           )  pCMD_unassigned
   , IDX_EQ( EdKC_f6           )  pCMD_window
   , IDX_EQ( EdKC_f7           )  pCMD_execute
   , IDX_EQ( EdKC_f8           )  pCMD_unassigned
   , IDX_EQ( EdKC_f9           )  pCMD_meta
   , IDX_EQ( EdKC_f10          )  pCMD_arg            //            LAPTOP
   , IDX_EQ( EdKC_f11          )  pCMD_mfreplace
   , IDX_EQ( EdKC_f12          )  pCMD_unassigned
   , IDX_EQ( EdKC_home         )  pCMD_begline
   , IDX_EQ( EdKC_end          )  pCMD_endline
   , IDX_EQ( EdKC_left         )  pCMD_left
   , IDX_EQ( EdKC_right        )  pCMD_right
   , IDX_EQ( EdKC_up           )  pCMD_up
   , IDX_EQ( EdKC_down         )  pCMD_down
   , IDX_EQ( EdKC_pgup         )  pCMD_mpage
   , IDX_EQ( EdKC_pgdn         )  pCMD_ppage
   , IDX_EQ( EdKC_ins          )  pCMD_paste
   , IDX_EQ( EdKC_del          )  pCMD_delete
   , IDX_EQ( EdKC_center       )  pCMD_arg
   , IDX_EQ( EdKC_num0         )  pCMD_graphic
   , IDX_EQ( EdKC_num1         )  pCMD_graphic
   , IDX_EQ( EdKC_num2         )  pCMD_graphic
   , IDX_EQ( EdKC_num3         )  pCMD_graphic
   , IDX_EQ( EdKC_num4         )  pCMD_graphic
   , IDX_EQ( EdKC_num5         )  pCMD_graphic
   , IDX_EQ( EdKC_num6         )  pCMD_graphic
   , IDX_EQ( EdKC_num7         )  pCMD_graphic
   , IDX_EQ( EdKC_num8         )  pCMD_graphic
   , IDX_EQ( EdKC_num9         )  pCMD_graphic
   , IDX_EQ( EdKC_numMinus     )  pCMD_udelete
   , IDX_EQ( EdKC_numPlus      )  pCMD_copy
   , IDX_EQ( EdKC_numStar      )  pCMD_graphic
   , IDX_EQ( EdKC_numSlash     )  pCMD_graphic
   , IDX_EQ( EdKC_numEnter     )  pCMD_emacsnewl
   , IDX_EQ( EdKC_space        )  pCMD_unassigned
   , IDX_EQ( EdKC_bksp         )  pCMD_emacscdel
   , IDX_EQ( EdKC_tab          )  pCMD_tab
   , IDX_EQ( EdKC_esc          )  pCMD_cancel
   , IDX_EQ( EdKC_enter        )  pCMD_emacsnewl
   , IDX_EQ( EdKC_a_0          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_1          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_2          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_3          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_4          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_5          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_6          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_7          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_8          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_9          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_a          )  pCMD_arg
   , IDX_EQ( EdKC_a_b          )  pCMD_boxstream
   , IDX_EQ( EdKC_a_c          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_d          )  pCMD_lasttext
   , IDX_EQ( EdKC_a_e          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_g          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_h          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_i          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_j          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_k          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_l          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_m          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_n          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_o          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_p          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_q          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_r          )  pCMD_unassigned     //        CMD_record
   , IDX_EQ( EdKC_a_s          )  pCMD_lastselect
   , IDX_EQ( EdKC_a_t          )  pCMD_tell
   , IDX_EQ( EdKC_a_u          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_v          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_w          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_x          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_y          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_z          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f1         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f2         )  pCMD_files          //       CMD_print
   , IDX_EQ( EdKC_a_f3         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f4         )  pCMD_exit
   , IDX_EQ( EdKC_a_f5         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f6         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f7         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f8         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f9         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f10        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f11        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f12        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_BACKTICK   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_MINUS      )  pCMD_unassigned
   , IDX_EQ( EdKC_a_EQUAL      )  pCMD_assign
   , IDX_EQ( EdKC_a_LEFT_SQ    )  pCMD_unassigned
   , IDX_EQ( EdKC_a_RIGHT_SQ   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_BACKSLASH  )  pCMD_unassigned
   , IDX_EQ( EdKC_a_SEMICOLON  )  pCMD_ldelete        //            LAPTOP
   , IDX_EQ( EdKC_a_TICK       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_COMMA      )  pCMD_unassigned
   , IDX_EQ( EdKC_a_DOT        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_SLASH      )  pCMD_copy           //            LAPTOP
   , IDX_EQ( EdKC_a_home       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_end        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_left       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_right      )  pCMD_unassigned
   , IDX_EQ( EdKC_a_up         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_down       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_pgup       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_pgdn       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_ins        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_del        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_center     )  pCMD_restcur
   , IDX_EQ( EdKC_a_num0       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num1       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num2       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num3       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num4       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num5       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num6       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num7       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num8       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num9       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numMinus   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numPlus    )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numStar    )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numSlash   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numEnter   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_space      )  pCMD_unassigned
   , IDX_EQ( EdKC_a_bksp       )  pCMD_undo
   , IDX_EQ( EdKC_a_tab        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_esc        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_enter      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_0          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_1          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_2          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_3          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_4          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_5          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_6          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_7          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_8          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_9          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_a          )  pCMD_mword
   , IDX_EQ( EdKC_c_b          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_c          )  WL( pCMD_towinclip, pCMD_unassigned )
   , IDX_EQ( EdKC_c_d          )  pCMD_right
   , IDX_EQ( EdKC_c_e          )  pCMD_up
   , IDX_EQ( EdKC_c_f          )  pCMD_pword
   , IDX_EQ( EdKC_c_g          )  pCMD_cdelete
   , IDX_EQ( EdKC_c_h          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_i          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_j          )  pCMD_sinsert
   , IDX_EQ( EdKC_c_k          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_l          )  pCMD_replace
   , IDX_EQ( EdKC_c_m          )  pCMD_mark
   , IDX_EQ( EdKC_c_n          )  pCMD_linsert
   , IDX_EQ( EdKC_c_o          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_p          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_q          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_r          )  pCMD_refresh
   , IDX_EQ( EdKC_c_s          )  pCMD_left
   , IDX_EQ( EdKC_c_t          )  pCMD_tell
   , IDX_EQ( EdKC_c_u          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_v          )  WL( pCMD_fromwinclip, pCMD_unassigned )
   , IDX_EQ( EdKC_c_w          )  pCMD_mlines
   , IDX_EQ( EdKC_c_x          )  pCMD_execute
   , IDX_EQ( EdKC_c_y          )  pCMD_ldelete
   , IDX_EQ( EdKC_c_z          )  pCMD_plines         //   // gap
   , IDX_EQ( EdKC_c_f1         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f2         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f3         )  pCMD_grep           //       CMD_compile
   , IDX_EQ( EdKC_c_f4         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f5         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f6         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f7         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f8         )  pCMD_unassigned     //       CMD_print
   , IDX_EQ( EdKC_c_f9         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f10        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f11        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f12        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_BACKTICK   )  pCMD_unassigned
   , IDX_EQ( EdKC_c_MINUS      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_EQUAL      )  pCMD_false
   , IDX_EQ( EdKC_c_LEFT_SQ    )  pCMD_unassigned     //       CMD_pbal
   , IDX_EQ( EdKC_c_RIGHT_SQ   )  pCMD_setwindow
   , IDX_EQ( EdKC_c_BACKSLASH  )  pCMD_qreplace
   , IDX_EQ( EdKC_c_SEMICOLON  )  pCMD_unassigned
   , IDX_EQ( EdKC_c_TICK       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_COMMA      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_DOT        )  pCMD_false
   , IDX_EQ( EdKC_c_SLASH      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_home       )  pCMD_home
   , IDX_EQ( EdKC_c_end        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_left       )  pCMD_mword
   , IDX_EQ( EdKC_c_right      )  pCMD_pword
   , IDX_EQ( EdKC_c_up         )  pCMD_mpara
   , IDX_EQ( EdKC_c_down       )  pCMD_ppara
   , IDX_EQ( EdKC_c_pgup       )  pCMD_begfile
   , IDX_EQ( EdKC_c_pgdn       )  pCMD_endfile
   , IDX_EQ( EdKC_c_ins        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_del        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_center     )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num0       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num1       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num2       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num3       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num4       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num5       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num6       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num7       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num8       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num9       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numMinus   )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numPlus    )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numStar    )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numSlash   )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numEnter   )  pCMD_unassigned
   , IDX_EQ( EdKC_c_space      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_bksp       )  pCMD_redo
   , IDX_EQ( EdKC_c_tab        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_esc        )  pCMD_unassigned     //   // WINDOWS SYSTEM KC
   , IDX_EQ( EdKC_c_enter      )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f1         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f2         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f3         )  pCMD_unassigned     //       CMD_nextmsg
   , IDX_EQ( EdKC_s_f4         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f5         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f6         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f7         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f8         )  pCMD_initialize
   , IDX_EQ( EdKC_s_f9         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f10        )  pCMD_files
   , IDX_EQ( EdKC_s_f11        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f12        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_home       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_end        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_left       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_right      )  pCMD_unassigned
   , IDX_EQ( EdKC_s_up         )  pCMD_mlines
   , IDX_EQ( EdKC_s_down       )  pCMD_plines
   , IDX_EQ( EdKC_s_pgup       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_pgdn       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_ins        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_del        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_center     )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num0       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num1       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num2       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num3       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num4       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num5       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num6       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num7       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num8       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num9       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numMinus   )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numPlus    )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numStar    )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numSlash   )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numEnter   )  pCMD_unassigned
   , IDX_EQ( EdKC_s_space      )  pCMD_graphic
   , IDX_EQ( EdKC_s_bksp       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_tab        )  pCMD_backtab
   , IDX_EQ( EdKC_s_esc        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_enter      )  pCMD_emacsnewl

   , IDX_EQ( EdKC_cs_bksp      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_tab       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_center    )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_enter     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_pause     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_space     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_pgup      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_pgdn      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_end       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_home      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_left      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_up        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_right     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_down      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_ins       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_0         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_1         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_2         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_3         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_4         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_5         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_6         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_7         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_8         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_9         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_a         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_b         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_c         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_d         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_e         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_g         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_h         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_i         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_j         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_k         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_l         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_m         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_n         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_o         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_p         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_q         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_r         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_s         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_t         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_u         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_v         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_w         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_x         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_y         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_z         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numStar   )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numPlus   )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numEnter  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numMinus  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numSlash  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f1        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f2        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f3        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f4        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f5        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f6        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f7        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f8        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f9        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f10       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f11       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f12       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_scroll    )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_SEMICOLON )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_EQUAL     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_COMMA     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_MINUS     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_DOT       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_SLASH     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_BACKTICK  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_TICK      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_LEFT_SQ   )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_BACKSLASH )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_RIGHT_SQ  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num0      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num1      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num2      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num3      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num4      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num5      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num6      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num7      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num8      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num9      )  pCMD_unassigned

   , IDX_EQ( EdKC_pause        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_pause      )  pCMD_unassigned
   , IDX_EQ( EdKC_s_pause      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numlk      )  pCMD_savecur
   , IDX_EQ( EdKC_scroll       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_scroll     )  pCMD_unassigned
   , IDX_EQ( EdKC_s_scroll     )  pCMD_unassigned
   };

static_assert( ELEMENTS( g_Key2CmdTbl ) == EdKC_COUNT, "ELEMENTS( g_Key2CmdTbl ) == EdKC_COUNT" );


// s_CmdIdxBuiltins is a const tree, never pruned or added to after CmdIdxInit()
// is called (contains only statically allocated C++ CMDs, which cannot be
// freed!!!)

STATIC_VAR PRbTree s_CmdIdxBuiltins;

// s_CmdIdxAddins is a dynamic tree which is searched prior to s_CmdIdxBuiltins
// during user-function-lookups: all CMDs indexed herein (for macros and Lua
// functions) are heap-allocated, and can thus be created and destroyed at will:

GLOBAL_VAR PRbTree s_CmdIdxAddins;

STIL PCMD IdxNodeToPCMD( PRbNode pNd ) { return static_cast<PCMD>( rb_val(pNd) ); }  // type-safe conversion function

int rb_strcmpi( PCVoid p1, PCVoid p2 ) {
   return Stricmp( static_cast<PCChar>(p1), static_cast<PCChar>(p2) );
   }

STATIC_VAR RbCtrl s_CmdIdxRbCtrl = { AllocNZ_, Free_, };

void CmdIdxInit() {
   s_CmdIdxAddins   = rb_alloc_tree( &s_CmdIdxRbCtrl );
   s_CmdIdxBuiltins = rb_alloc_tree( &s_CmdIdxRbCtrl );
   for( auto &cmd : g_CmdTable ) {
      rb_insert_gen( s_CmdIdxBuiltins, cmd.Name(), rb_strcmpi, &cmd );
      }
   }

STATIC_FXN void CMD_PlacementFree_SameName( PCMD pCmd ) {
   if( pCmd->IsRealMacro() )
      Free0( pCmd->d_argData.pszMacroDef );

   AHELP( Free0( pCmd->d_HelpStr ); )
   }

STATIC_FXN void DeleteCMD( void *pData, void *pExtra ) {
   auto pCmd( static_cast<PCMD>(pData) );
   CMD_PlacementFree_SameName( pCmd );
   Free0( pCmd->d_name );
   Free0( pCmd );
   }

void CmdIdxClose() {
   rb_dealloc_treev( s_CmdIdxAddins, nullptr, DeleteCMD );
   rb_dealloc_tree(  s_CmdIdxBuiltins );
   }

PCMD CmdFromName( PCChar name ) {
   auto pNd( rb_find_gen( s_CmdIdxAddins  , name, rb_strcmpi ) );
   if( !pNd )  pNd = rb_find_gen( s_CmdIdxBuiltins, name, rb_strcmpi );
   return pNd ? IdxNodeToPCMD( pNd ) : nullptr;
   }

STATIC_FXN PCMD CmdFromNameBuiltinOnly( PCChar name ) {
   auto pNd( rb_find_gen( s_CmdIdxBuiltins, name, rb_strcmpi ) );
   return pNd ? IdxNodeToPCMD( pNd ) : nullptr;
   }

STATIC_FXN void cmdIdxAdd( PCChar name, funcCmd pFxn, int argType, PCChar macroDef  _AHELP( PCChar helpStr ) ) {
   int equal;
   auto pNd( rb_find_gte_gen( &equal, s_CmdIdxAddins, name, rb_strcmpi ) );
   PCMD pCmd;
   if( equal ) {
      pCmd = IdxNodeToPCMD( pNd );
      CMD_PlacementFree_SameName( pCmd );
      }
   else {
      AllocArrayZ( pCmd );
      pCmd->d_name = Strdup( name );
      rb_insert_before( s_CmdIdxAddins, pNd, pCmd->Name(), pCmd );
      }

   pCmd->d_func    = pFxn;
   pCmd->d_argType = argType;
   if( macroDef )
      pCmd->d_argData.pszMacroDef = Strdup( macroDef );

   AHELP( pCmd->d_HelpStr = Strdup( helpStr ? helpStr : "" ); )

   // semi-hacky: replace any references to same-named builtin function
   const auto pCmdBuiltIn( CmdFromNameBuiltinOnly( name ) );
   if( pCmdBuiltIn )
      EventCmdSupercede( pCmdBuiltIn, pCmd );
   }

void CmdIdxAddLuaFunc( PCChar name, funcCmd pFxn, int argType  _AHELP( PCChar helpStr ) ) {
   return cmdIdxAdd( name, pFxn, argType, nullptr  _AHELP( helpStr ) );
   }

void CmdIdxAddMacro( PCChar name, PCChar macroDef ) {
   return cmdIdxAdd( name, fn_runmacro(), (KEEPMETA + MACROFUNC), macroDef  _AHELP( nullptr ) );
   }

STATIC_FXN void DeleteCmd( PCMD pCmd ) {
   { // revert to builtin CMD of same name, if any
   const auto pCmdBuiltIn( CmdFromNameBuiltinOnly( pCmd->d_name ) );
   EventCmdSupercede( pCmd, pCmdBuiltIn ? pCmdBuiltIn : pCMD_unassigned );
   }

   Free0( pCmd->d_name );
   if( pCmd->IsRealMacro() )
      Free0( pCmd->d_argData.pszMacroDef );

   AHELP( Free0( pCmd->d_HelpStr ); )
   Free0( pCmd );
   }

int CmdIdxRmvCmdsByFunction( funcCmd pFxn ) {
   auto rv(0);
   PCChar pPrevKey(nullptr);
   for( auto pNd=rb_first( s_CmdIdxAddins ) ; pNd != rb_last( s_CmdIdxAddins ); pNd = rb_next( pNd ) ) {
      {
      const auto pCmd( IdxNodeToPCMD( pNd ) );
      if( pCmd->d_func != pFxn ) {
         pPrevKey = pCmd->Name();
         continue;
         }
      DeleteCmd( pCmd );
      }
      ++rv;

      rb_delete_node( s_CmdIdxAddins, pNd );

      // now (since cur node has been deleted), set pNd to prev node
      // locate prev node by searching for pPrevKey
      if( !pPrevKey ) {
         pNd = rb_first( s_CmdIdxAddins );
         }
      else {
         int equal;
         pNd = rb_find_gte_gen( &equal, s_CmdIdxAddins, pPrevKey, rb_strcmpi );
         Assert( equal );
         }
      }

   return rv;
   }

// iterator APIs

PCmdIdxNd CmdIdxFirst()                 { return rb_first( s_CmdIdxBuiltins ); }
PCmdIdxNd CmdIdxLast()                  { return rb_last(  s_CmdIdxBuiltins ); }
PCmdIdxNd CmdIdxAddinFirst()            { return rb_first( s_CmdIdxAddins ); }
PCmdIdxNd CmdIdxAddinLast()             { return rb_last(  s_CmdIdxAddins ); }
PCmdIdxNd CmdIdxNext( PCmdIdxNd pNd )   { return rb_next(  pNd      ); }
PCCMD     CmdIdxToPCMD( PCmdIdxNd pNd ) { return IdxNodeToPCMD( pNd ); }

void WalkAllCMDs( void *pCtxt, CmdVisit visit ) { 0 && DBG( "%s+", __func__ );
   for( auto pNd=CmdIdxFirst() ; pNd != CmdIdxLast() ; pNd = CmdIdxNext( pNd ) )
      visit( CmdIdxToPCMD( pNd ), pCtxt );
   for( auto pNd=CmdIdxAddinFirst() ; pNd != CmdIdxAddinLast() ; pNd = CmdIdxNext( pNd ) )
      visit( CmdIdxToPCMD( pNd ), pCtxt );
   }

#include "my_fio.h"

enum { SEEN = 1<<31 };

STATIC_FXN void sv_idx( PCCMD pCmd, void *pCtxt ) {
   auto ofh( static_cast<FILE *>(pCtxt) );
   if( pCmd->d_callCount != SEEN ) {
      pCmd->d_gCallCount += pCmd->d_callCount;
      }
   pCmd->d_callCount = 0;
   if( pCmd->d_gCallCount > 0 ) {
      fprintf( ofh, "%10u %s\n", pCmd->d_gCallCount, pCmd->d_name );
      }
   }

void cmdusage_updt() {
   enum { SHOWDBG = 0 };
   SHOWDBG && DBG( "%s+", __func__ );
   pathbuf origfnm;
   StateFilename( BSOB(origfnm), "cmdusg" );
   const auto ifh( fopen( origfnm, "rt" ) );
   const auto fOrigFExists( ifh != nullptr );
   if( ifh ) {
      SHOWDBG && DBG( "%s: opened ifh = '%s'", __func__, origfnm );
      auto entries(0);
      for(;;) {
         char buf[300];
         auto p( fgets( BSOB(buf), ifh ) );
         if( !p )
            break;

         //DBG( "%s: '%s'", __func__, p );
         pathbuf  savedCmdnm;
         unsigned savedCount;
         if( (2 == sscanf( p, "%u %s", &savedCount, savedCmdnm )) && *savedCmdnm ) {
            ++entries;
            //DBG( "%s:>%u '%s'", __func__, savedCount, savedCmdnm );
            const PCCMD pCmd( CmdFromName( savedCmdnm ) );
            if( pCmd ) {
               pCmd->d_gCallCount = savedCount + pCmd->d_callCount;
               pCmd->d_callCount = SEEN;
               }
            }
         }
      fclose( ifh );
      SHOWDBG && DBG( "%s: closed ifh = '%s'; read %d entries", __func__, origfnm, entries );
      }

   pathbuf tmpfnm;
   StateFilename( BSOB(tmpfnm), "cmdusg_" );
   //DBG( "%s:%s", __func__, tmpfnm );
   const auto ofh = fopen( tmpfnm, "wt" );
   if( ofh ) {
      SHOWDBG && DBG( "%s: opened ofh = '%s'", __func__, tmpfnm );
      WalkAllCMDs( ofh, sv_idx );
      fclose( ofh );
      SHOWDBG && DBG( "%s: closed ofh = '%s'", __func__, tmpfnm );
      const auto doRename( !fOrigFExists || unlinkOk( origfnm ) );
      if( doRename ) {
         const auto renmOk( rename( tmpfnm, origfnm ) == 0 );
         SHOWDBG && DBG( "%s: did rename, %s", __func__, renmOk ? "ok" : "FAILED" );
         }
      }
   SHOWDBG && DBG( "%s-", __func__ );
   }
