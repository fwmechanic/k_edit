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

#include "ed_main.h"
#include "krbtree.h"

GLOBAL_VAR CMD g_CmdTable[] = { // *** THIS IS GLOBAL, NOT STATIC, BECAUSE #defines of pCMD_xxx in cmdtbl.kh refer to it
//-----------------------------------
#define CMDTBL_H_CMD_TBL_INITIALIZERS
#include "cmdtbl.h" // <-- Lua-generated array initializers and #defines
#undef  CMDTBL_H_CMD_TBL_INITIALIZERS
//-----------------------------------
   };

#ifdef __GNUC__

// Use GCC-only "[out-of-order] designated initialization of arrays" feature if available.
//
// NB: even though C++20 added "designated initializers",
//
//    "Note: out-of-order designated initialization, nested designated
//    initialization, mixing of designated initializers and regular initializers,
//    and designated initialization of arrays are all supported in the C
//    programming language, but are not allowed in C++20."
//
//    Source: https://en.cppreference.com/w/cpp/language/aggregate_initialization#Designated_initializers

#define  IDX_EQ( idx_val )  [ idx_val ] =
#else
#error
#define  IDX_EQ( idx_val )
#endif

// nearly identical declarations:
STATIC_VAR PCMD s_Key2CmdTbl[] = // use this so assert @ end of initializer will work
   {
   // first 256 [0x00..0xFF] are for ASCII codes
   // C++20 requires that if ANY designated initializers are used, ALL have to have them, which is why all the (hex) numbers...
   #define  gfc(nn)  [nn]=pCMD_graphic
     gfc(0x00),gfc(0x01),gfc(0x02),gfc(0x03),gfc(0x04),gfc(0x05),gfc(0x06),gfc(0x07),gfc(0x08),gfc(0x09),gfc(0x0a),gfc(0x0b),gfc(0x0c),gfc(0x0d),gfc(0x0e),gfc(0x0f)
   , gfc(0x10),gfc(0x11),gfc(0x12),gfc(0x13),gfc(0x14),gfc(0x15),gfc(0x16),gfc(0x17),gfc(0x18),gfc(0x19),gfc(0x1a),gfc(0x1b),gfc(0x1c),gfc(0x1d),gfc(0x1e),gfc(0x1f)
   , gfc(0x20),gfc(0x21),gfc(0x22),gfc(0x23),gfc(0x24),gfc(0x25),gfc(0x26),gfc(0x27),gfc(0x28),gfc(0x29),gfc(0x2a),gfc(0x2b),gfc(0x2c),gfc(0x2d),gfc(0x2e),gfc(0x2f)
   , gfc(0x30),gfc(0x31),gfc(0x32),gfc(0x33),gfc(0x34),gfc(0x35),gfc(0x36),gfc(0x37),gfc(0x38),gfc(0x39),gfc(0x3a),gfc(0x3b),gfc(0x3c),gfc(0x3d),gfc(0x3e),gfc(0x3f)
   , gfc(0x40),gfc(0x41),gfc(0x42),gfc(0x43),gfc(0x44),gfc(0x45),gfc(0x46),gfc(0x47),gfc(0x48),gfc(0x49),gfc(0x4a),gfc(0x4b),gfc(0x4c),gfc(0x4d),gfc(0x4e),gfc(0x4f)
   , gfc(0x50),gfc(0x51),gfc(0x52),gfc(0x53),gfc(0x54),gfc(0x55),gfc(0x56),gfc(0x57),gfc(0x58),gfc(0x59),gfc(0x5a),gfc(0x5b),gfc(0x5c),gfc(0x5d),gfc(0x5e),gfc(0x5f)
   , gfc(0x60),gfc(0x61),gfc(0x62),gfc(0x63),gfc(0x64),gfc(0x65),gfc(0x66),gfc(0x67),gfc(0x68),gfc(0x69),gfc(0x6a),gfc(0x6b),gfc(0x6c),gfc(0x6d),gfc(0x6e),gfc(0x6f)
   , gfc(0x70),gfc(0x71),gfc(0x72),gfc(0x73),gfc(0x74),gfc(0x75),gfc(0x76),gfc(0x77),gfc(0x78),gfc(0x79),gfc(0x7a),gfc(0x7b),gfc(0x7c),gfc(0x7d),gfc(0x7e),gfc(0x7f)
   , gfc(0x80),gfc(0x81),gfc(0x82),gfc(0x83),gfc(0x84),gfc(0x85),gfc(0x86),gfc(0x87),gfc(0x88),gfc(0x89),gfc(0x8a),gfc(0x8b),gfc(0x8c),gfc(0x8d),gfc(0x8e),gfc(0x8f)
   , gfc(0x90),gfc(0x91),gfc(0x92),gfc(0x93),gfc(0x94),gfc(0x95),gfc(0x96),gfc(0x97),gfc(0x98),gfc(0x99),gfc(0x9a),gfc(0x9b),gfc(0x9c),gfc(0x9d),gfc(0x9e),gfc(0x9f)
   , gfc(0xA0),gfc(0xA1),gfc(0xA2),gfc(0xA3),gfc(0xA4),gfc(0xA5),gfc(0xA6),gfc(0xA7),gfc(0xA8),gfc(0xA9),gfc(0xAa),gfc(0xAb),gfc(0xAc),gfc(0xAd),gfc(0xAe),gfc(0xAf)
   , gfc(0xB0),gfc(0xB1),gfc(0xB2),gfc(0xB3),gfc(0xB4),gfc(0xB5),gfc(0xB6),gfc(0xB7),gfc(0xB8),gfc(0xB9),gfc(0xBa),gfc(0xBb),gfc(0xBc),gfc(0xBd),gfc(0xBe),gfc(0xBf)
   , gfc(0xC0),gfc(0xC1),gfc(0xC2),gfc(0xC3),gfc(0xC4),gfc(0xC5),gfc(0xC6),gfc(0xC7),gfc(0xC8),gfc(0xC9),gfc(0xCa),gfc(0xCb),gfc(0xCc),gfc(0xCd),gfc(0xCe),gfc(0xCf)
   , gfc(0xD0),gfc(0xD1),gfc(0xD2),gfc(0xD3),gfc(0xD4),gfc(0xD5),gfc(0xD6),gfc(0xD7),gfc(0xD8),gfc(0xD9),gfc(0xDa),gfc(0xDb),gfc(0xDc),gfc(0xDd),gfc(0xDe),gfc(0xDf)
   , gfc(0xE0),gfc(0xE1),gfc(0xE2),gfc(0xE3),gfc(0xE4),gfc(0xE5),gfc(0xE6),gfc(0xE7),gfc(0xE8),gfc(0xE9),gfc(0xEa),gfc(0xEb),gfc(0xEc),gfc(0xEd),gfc(0xEe),gfc(0xEf)
   , gfc(0xF0),gfc(0xF1),gfc(0xF2),gfc(0xF3),gfc(0xF4),gfc(0xF5),gfc(0xF6),gfc(0xF7),gfc(0xF8),gfc(0xF9),gfc(0xFa),gfc(0xFb),gfc(0xFc),gfc(0xFd),gfc(0xFe),gfc(0xFf)
   #undef  gfc

#ifdef fn_argselkeymap
   , IDX_EQ( EdKC_sela         )  pCMD_selkeymaptogl // a
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
   , IDX_EQ( EdKC_sel0         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel1         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel2         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel3         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel4         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel5         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel6         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel7         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel8         )  pCMD_graphic   //
   , IDX_EQ( EdKC_sel9         )  pCMD_graphic   //
   , IDX_EQ( EdKC_selSPACE     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selLPAREN    )  pCMD_graphic   //
   , IDX_EQ( EdKC_selRPAREN    )  pCMD_graphic   //
   , IDX_EQ( EdKC_selLCURLY    )  pCMD_graphic   //
   , IDX_EQ( EdKC_selRCURLY    )  pCMD_graphic   //
   , IDX_EQ( EdKC_selLEFT_SQ   )  pCMD_graphic   //
   , IDX_EQ( EdKC_selRIGHT_SQ  )  pCMD_graphic   //
   , IDX_EQ( EdKC_selLT        )  pCMD_graphic   //
   , IDX_EQ( EdKC_selGT        )  pCMD_graphic   //
   , IDX_EQ( EdKC_selPIPE      )  pCMD_graphic   //
   , IDX_EQ( EdKC_selBACKSLASH )  pCMD_graphic   //
   , IDX_EQ( EdKC_selSLASH     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selQMARK     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selEQUAL     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selPLUS      )  pCMD_graphic   //
   , IDX_EQ( EdKC_selMINUS     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selUNDERSCORE)  pCMD_graphic   //
   , IDX_EQ( EdKC_selSEMICOLON )  pCMD_graphic   //
   , IDX_EQ( EdKC_selCOLON     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selCOMMA     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selDOT       )  pCMD_graphic   //
   , IDX_EQ( EdKC_selBACKTICK  )  pCMD_graphic   //
   , IDX_EQ( EdKC_selTILDE     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selEX        )  pCMD_graphic   //
   , IDX_EQ( EdKC_selAT        )  pCMD_graphic   //
   , IDX_EQ( EdKC_selPOUND     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selDOLLAR    )  pCMD_graphic   //
   , IDX_EQ( EdKC_selPERCENT   )  pCMD_graphic   //
   , IDX_EQ( EdKC_selCARET     )  pCMD_graphic   //
   , IDX_EQ( EdKC_selAND       )  pCMD_graphic   //
   , IDX_EQ( EdKC_selSTAR      )  pCMD_graphic   //
   , IDX_EQ( EdKC_selDQUOTE    )  pCMD_graphic   //

#endif // #ifdef fn_argselkeymap

   , IDX_EQ( EdKC_f1           )  pCMD_selcmd
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
   , IDX_EQ( EdKC_c_c          )  WL( pCMD_towinclip, pCMD_toxclip )
   , IDX_EQ( EdKC_c_d          )  pCMD_right
   , IDX_EQ( EdKC_c_e          )  pCMD_up
   , IDX_EQ( EdKC_c_f          )  pCMD_pword
   , IDX_EQ( EdKC_c_g          )  pCMD_cdelete
   , IDX_EQ( EdKC_c_h          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_i          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_j          )  pCMD_sinsert
   , IDX_EQ( EdKC_c_k          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_l          )  pCMD_replace
   , IDX_EQ( EdKC_c_m          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_n          )  pCMD_linsert
   , IDX_EQ( EdKC_c_o          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_p          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_q          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_r          )  pCMD_refresh
   , IDX_EQ( EdKC_c_s          )  pCMD_left
   , IDX_EQ( EdKC_c_t          )  pCMD_tell
   , IDX_EQ( EdKC_c_u          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_v          )  WL( pCMD_fromwinclip, pCMD_fromxclip )
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

   , IDX_EQ( EdKC_ca_f1        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f2        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f3        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f4        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f5        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f6        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f7        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f8        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f9        )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f10       )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f11       )  pCMD_unassigned
   , IDX_EQ( EdKC_ca_f12       )  pCMD_unassigned

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
   , IDX_EQ( EdKC_cs_del       )  pCMD_unassigned
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

static_assert( ELEMENTS( s_Key2CmdTbl ) == EdKC_COUNT, "ELEMENTS( s_Key2CmdTbl ) == EdKC_COUNT" );

SetKeyRV BindKeyToCMD( stref cmdName, stref keyName ) {
   const auto edKC( EdkcOfKeyNm( keyName ) ); if( !edKC ) { return SetKeyRV::SetKeyRV_BADKEY; }
   const auto pCmd( CmdFromName( cmdName ) ); if( !pCmd ) { return SetKeyRV::SetKeyRV_BADCMD; }
   s_Key2CmdTbl[ edKC ] = pCmd;
   return SetKeyRV::SetKeyRV_OK;
   }

void AssignSubstituteCmd( PCMD pOldCmd, PCMD pNewCmd ) {
   for( auto &pCmd : s_Key2CmdTbl ) {
      if( pCmd == pOldCmd ) {
          pCmd =  pNewCmd;
          }
      }
   }

std::string KeyNmAssignedToCmd_first( const CMD &CmdToFind ) {
   for( const auto &pCmd : s_Key2CmdTbl ) {
      if( pCmd == &CmdToFind ) {
         return KeyNmOfEdkc( &pCmd - s_Key2CmdTbl );
         }
      }
   return "";
   }

std::string KeyNmAssignedToCmd_all( PCCMD pCmdToFind, PCChar nmSep ) {
   if( pCmdToFind == pCMD_graphic ) {
      return "";
      }
   std::string dest, kyNm;
   BoolOneShot first;
   for( const auto &pCmd : s_Key2CmdTbl ) {
      if( pCmd == pCmdToFind ) {
         if( !first ) {
            dest.append( nmSep );
            }
         dest.append( KeyNmOfEdkc( kyNm, &pCmd - s_Key2CmdTbl ) );
         }
      }
   return dest;
   }

void AssignShowKeyAssignment( const CMD &Cmd, PFBUF pFBufToWrite, std::string &tmp1, std::string &tmp2 ) {
   if( Cmd.IsFnUnassigned() || Cmd.IsFnGraphic() ) {
      return;
      }
   FmtStr<50> cmdNm( "%-20s: ", Cmd.Name() );
   const PCChar pText( Cmd.IsRealMacro() ? Cmd.MacroText() : (Cmd.d_HelpStr && *Cmd.d_HelpStr ?  Cmd.d_HelpStr : "") );
   auto fFoundAssignment(false);
   auto &keyNm( tmp1 );
   auto &accum( tmp2 );
   for( const auto &pCmd : s_Key2CmdTbl ) {
      if( pCmd == &Cmd ) {
         accum.clear();
         accum.append( cmdNm.c_str() );
         KeyNmOfEdkc( keyNm, &pCmd - s_Key2CmdTbl ); // trail-pad with spaces to width
         accum.append( PadRight( keyNm, g_MaxKeyNameLen ) );
         accum.append( " # " );
         accum.append( !fFoundAssignment ? pText : "|" );
         pFBufToWrite->PutLastLineRaw( accum );
         fFoundAssignment = true;
         }
      }
   if( !fFoundAssignment ) {
      keyNm.clear();
      accum.clear();
      accum.append( cmdNm.c_str() );
      accum.append( PadRight( keyNm, g_MaxKeyNameLen ) );
      accum.append( " # " );
      accum.append( pText );
      pFBufToWrite->PutLastLineRaw( accum );
      }
   }

int ShowAllUnassignedKeys( PFBUF pFBuf ) { // pFBuf may be 0 if caller is only interested in # of avail keys
   auto count(0);
   auto tblCol(0);
   const auto col_width( g_MaxKeyNameLen + 1 );
   std::string lbuf, keyNm;
   for( const auto &pCmd : s_Key2CmdTbl ) {
      if( pCmd->IsFnUnassigned() ) {
         KeyNmOfEdkc( keyNm, &pCmd - s_Key2CmdTbl );
         if( !keyNm.empty() ) {
            ++count;
            if( pFBuf ) {
               lbuf.append( PadRight( keyNm, col_width ) );
               if( tblCol++ == (g_CurWin()->d_Size.col / col_width) - 1 ) {
                  pFBuf->PutLastLineRaw( lbuf );
                  tblCol = 0;
                  lbuf.clear();
                  }
               }
            }
         }
      }
   if( pFBuf && tblCol > 0 ) {
      pFBuf->PutLastLineRaw( lbuf );
      }
   return count;
   }

PCCMD CmdFromKbdForInfo( std::string &dest ) {
   const auto cd( ConIn::EdKC_Ascii_FromNextKey_Keystr( dest ) );
   return cd.EdKcEnum == 0 ? pCMD_unassigned : s_Key2CmdTbl[ cd.EdKcEnum ];
   }

PCCMD CmdFromKbdForExec() {
   const auto cmddata( ConIn::EdKC_Ascii_FromNextKey() );
   if( 0 == cmddata.EdKcEnum && ExecutionHaltRequested() ) { 0 && DBG( "CmdFromKbdForExec sending pCMD_cancel" );
      return pCMD_cancel;
      }
   if( cmddata.EdKcEnum >= EdKC_Count ) {                    DBG( "!!! KC=0x%X", cmddata.EdKcEnum );
      return pCMD_unassigned;
      }
#if 0 // to get every (valid) keystroke to display
   else {
      char kystr[50];
      KeyNmOfEdkc( BSOB(kystr), cmddata.EdKcEnum );          DBG( "EdKc=%s", kystr );
      }
#endif
   const auto pCmd( s_Key2CmdTbl[ cmddata.EdKcEnum ] );
   if( pCmd && !pCmd->IsRealMacro() ) {
       pCmd->d_argData.eka = cmddata;
       }
   return pCmd;
   }

void UnbindMacrosFromKeys() {
   for( auto &pCmd : s_Key2CmdTbl ) {
      if( pCmd->IsRealMacro() ) {
         pCmd = pCMD_unassigned;
         }
      }
   }

// s_CmdIdxAddins is a dynamic tree which is searched prior to g_CmdTable
// during user-function-lookups: all CMDs indexed herein (for macros and Lua
// functions) are heap-allocated, and can thus be created and destroyed at will:
STATIC_VAR RbTree *s_CmdIdxAddins;

STIL PCMD IdxNodeToPCMD( PCmdIdxNd pNd ) { return static_cast<PCMD>( rb_val(pNd) ); }  // type-safe conversion function

STATIC_VAR RbCtrl s_CmdIdxRbCtrl = { AllocNZ_, Free_, };

STATIC_FXN void CMD_PlacementFree_SameName( PCMD pCmd ) {
   if( pCmd->IsRealMacro() ) {
      Free0( pCmd->d_argData.pszMacroDef );
      }
   AHELP( Free0( pCmd->d_HelpStr ); )
   }

STATIC_FXN void DeleteCMD( void *pData, void *pExtra ) {
   auto pCmd( static_cast<PCMD>(pData) );                0 && pCmd->IsRealMacro() && DBG( "%s MACRO %s", __func__, pCmd->d_name );
   CMD_PlacementFree_SameName( pCmd );
   Free0( pCmd->d_name );
   Free0( pCmd );
   }

void CmdIdxInit() {  s_CmdIdxAddins = rb_alloc_tree( &s_CmdIdxRbCtrl ); }
void CmdIdxClose() { rb_dealloc_treev( s_CmdIdxAddins, nullptr, DeleteCMD ); }

STATIC_FXN PCMD CmdFromNameBuiltinOnly( stref src ) {    0 && DBG( "%" PR_BSR "?", BSR(src) );
   // binary search
   size_t yMin( 0 );
   auto   yMax( ELEMENTS( g_CmdTable ) - 1 );
   while( yMin <= yMax ) {
      //                ( (yMax + yMin) / 2 );           // old overflow-susceptible version
      const auto cmpLine( yMin + ((yMax - yMin) / 2) );  // new overflow-proof version
      const auto &cand( g_CmdTable[ cmpLine ] );
      const auto rslt( cmpi( src, cand.Name() ) );
      if( rslt == 0 ) {                                  0 && DBG( "%s=[%" PR_SIZET "]: %" PR_BSR "|", __func__, cmpLine, BSR(src) );
         return CAST_AWAY_CONST(PCMD)( &cand );
         }
      if( rslt <  0 ) { /* handle unsigned underflow/wraparound */
         if( cmpLine==0 ) {
            break;
            }
         yMax = cmpLine - 1;
         }
      if( rslt >  0 ) { yMin = cmpLine + 1; }            0 && DBG( "%s=[%" PR_SIZET ",%" PR_SIZET "]: %" PR_BSR "|", __func__, yMin, yMax, BSR(src) );
      }                                                  0 && DBG( "%s:[-1]: %" PR_BSR "|", __func__, BSR(src) );
   return nullptr;
   }

PCMD CmdFromName( stref src ) {
   auto pNd( rb_find_sri( s_CmdIdxAddins, src ) );
   if( pNd ) { return IdxNodeToPCMD( pNd ); }
   // if( !pNd )  DBG( "%s '%" PR_BSR "'", __func__, BSR(src) );
   return CmdFromNameBuiltinOnly( src );
   }

STATIC_FXN void cmdIdxAdd( stref name, funcCmd pFxn, int argType, stref macroDef  _AHELP( PCChar helpStr ) ) {
   int equal;
   auto pNd( rb_find_gte_sri( &equal, s_CmdIdxAddins, name ) );
   PCMD pCmd;
   if( equal ) {
      pCmd = IdxNodeToPCMD( pNd );
      CMD_PlacementFree_SameName( pCmd );
      }
   else {
      AllocArrayZ( pCmd, 1 );
      pCmd->d_name = Strdup( name );
      rb_insert_before( s_CmdIdxAddins, pNd, pCmd->Name(), pCmd );
      }
   pCmd->d_func    = pFxn;
   pCmd->d_argType = argType;
   if( pCmd->IsRealMacro() ) {
      pCmd->d_argData.pszMacroDef = Strdup( macroDef );
      }
   AHELP( pCmd->d_HelpStr = Strdup( helpStr ? helpStr : "" ); )
   // semi-hacky: replace any references to same-named builtin function
   const auto pCmdBuiltIn( CmdFromNameBuiltinOnly( name ) );
   if( pCmdBuiltIn ) {
      AssignSubstituteCmd( pCmdBuiltIn, pCmd );
      }
   }

void CmdIdxAddLuaFunc( PCChar name, funcCmd pFxn, int argType  _AHELP( PCChar helpStr ) ) {
   return cmdIdxAdd( name, pFxn, argType, ""  _AHELP( helpStr ) );
   }

void CmdIdxAddMacro( stref name, stref macroDef ) { 0 && DBG( "%s: '%" PR_BSR "'<-'%" PR_BSR "'", __func__, BSR(name), BSR(macroDef) );
   return cmdIdxAdd( name, fn_runmacro(), (KEEPMETA + MACROFUNC), macroDef  _AHELP( nullptr ) );
   }

STATIC_FXN void DeleteCmd( PCMD pCmd ) {
   { // revert to builtin CMD of same name, if any
   const auto pCmdBuiltIn( CmdFromNameBuiltinOnly( pCmd->d_name ) );
   AssignSubstituteCmd( pCmd, pCmdBuiltIn ? pCmdBuiltIn : pCMD_unassigned );
   }
   Free0( pCmd->d_name );
   if( pCmd->IsRealMacro() ) {
      Free0( pCmd->d_argData.pszMacroDef );
      }
   AHELP( Free0( pCmd->d_HelpStr ); )
   Free0( pCmd );
   }

int CmdIdxRmvCmdsByFunction( funcCmd pFxn ) {
   auto rv(0);
   stref pPrevKey;
   rb_traverse( pNd, s_CmdIdxAddins ) {
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
      if( pPrevKey.empty() ) {
         pNd = rb_first( s_CmdIdxAddins );
         }
      else {
         int equal;
         pNd = rb_find_gte_sri( &equal, s_CmdIdxAddins, pPrevKey );
         Assert( equal );
         }
      }
   return rv;
   }

// iterator APIs

PCmdIdxNd CmdIdxNext( PCmdIdxNd pNd )      { return rb_next(  pNd      ); }
PCmdIdxNd CmdIdxAddinFirst()               { return rb_first( s_CmdIdxAddins ); }
PCmdIdxNd CmdIdxAddinNil()                 { return rb_nil( s_CmdIdxAddins ); }
PCmdIdxNd CmdIdxAddinNext( PCmdIdxNd pNd ) { return rb_next(  pNd      ); }
PCCMD     CmdIdxToPCMD( PCmdIdxNd pNd )    { return IdxNodeToPCMD( pNd ); }

void FBufRead_Assign_intrinsicCmds( PFBUF pFBuf, std::string &tmp1, std::string &tmp2 ) {
   FBufRead_Assign_SubHd( pFBuf, "Intrinsic Functions", ELEMENTS( g_CmdTable ) );
   for( auto &cand : g_CmdTable ) {
      AssignShowKeyAssignment( cand, pFBuf, tmp1, tmp2 );
      }
   }

void WalkAllCMDs( void *pCtxt, CmdVisit visit ) { 0 && DBG( "%s+", __func__ );
   for( auto &cand : g_CmdTable ) {
      visit( &cand, pCtxt );
      }
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinNil() ; pNd = CmdIdxNext( pNd ) ) {
      visit( CmdIdxToPCMD( pNd ), pCtxt );
      }
   }

#include "my_fio.h"

constexpr uint32_t SEEN = 1 << 31;

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
   enum { SD = 0 };                                                            SD && DBG( "%s+", __func__ );
   auto origfnm( StateFilename( "cmdusg" ) );
   const auto ifh( fopen( origfnm.c_str(), "rt" ) );
   const auto fOrigFExists( ifh != nullptr );
   if( ifh ) {                                                                 SD && DBG( "%s: opened ifh = '%s'", __func__, origfnm.c_str() );
      auto entries(0);
      for(;;) {
         char buf[300];
         auto p( fgets( BSOB(buf), ifh ) );
         if( !p ) {
            break;
            }                                                                  SD && DBG( "%s: '%s'", __func__, p );
         pathbuf  savedCmdnm;
         unsigned savedCount;
         if( (2 == sscanf( p, "%u %s", &savedCount, savedCmdnm )) && *savedCmdnm ) {
            ++entries;                                                         SD && DBG( "%s:>%u '%s'", __func__, savedCount, savedCmdnm );
            const PCCMD pCmd( CmdFromName( savedCmdnm ) );
            if( pCmd ) {
               pCmd->d_gCallCount = savedCount + pCmd->d_callCount;
               pCmd->d_callCount = SEEN;
               }
            }
         }
      fclose( ifh );                                                           SD && DBG( "%s: closed ifh = '%s'; read %d entries", __func__, origfnm.c_str(), entries );
      }
   auto tmpfnm( StateFilename( "cmdusg_" ) );                                  SD && DBG( "%s:%s", __func__, tmpfnm.c_str() );
   const auto ofh = fopen( tmpfnm.c_str(), "wt" );
   if( ofh ) {                                                                 SD && DBG( "%s: opened ofh = '%s'", __func__, tmpfnm.c_str() );
      WalkAllCMDs( ofh, sv_idx );
      fclose( ofh );                                                           SD && DBG( "%s: closed ofh = '%s'", __func__, tmpfnm.c_str() );
      const auto doRename( !fOrigFExists || unlinkOk( origfnm.c_str() ) );
      if( doRename ) {
         const auto renmOk( rename( tmpfnm.c_str(), origfnm.c_str() ) == 0 );  SD && DBG( "%s: did rename, %s", __func__, renmOk ? "ok" : "FAILED" );
         }
      }                                                                        SD && DBG( "%s-", __func__ );
   }
