//
// Copyright 2015-2016 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

typedef uint16_t EdKC;

enum { EdKC_Count = 256+272+91
#if SEL_KEYMAP
                              +(26*2)
#endif
   };

enum EdKeyCodes
   {
   // first 256 [0x00..0xFF] are for ASCII codes
     EdKC_a    = 'a'
   , EdKC_z    = 'z'
   , EdKC_A    = 'A'
   , EdKC_Z    = 'Z'
#if SEL_KEYMAP
   , EdKC_sela = 0x100
   , EdKC_selb
   , EdKC_selc
   , EdKC_seld
   , EdKC_sele
   , EdKC_self
   , EdKC_selg
   , EdKC_selh
   , EdKC_seli
   , EdKC_selj
   , EdKC_selk
   , EdKC_sell
   , EdKC_selm
   , EdKC_seln
   , EdKC_selo
   , EdKC_selp
   , EdKC_selq
   , EdKC_selr
   , EdKC_sels
   , EdKC_selt
   , EdKC_selu
   , EdKC_selv
   , EdKC_selw
   , EdKC_selx
   , EdKC_sely
   , EdKC_selz
   , EdKC_selA
   , EdKC_selB
   , EdKC_selC
   , EdKC_selD
   , EdKC_selE
   , EdKC_selF
   , EdKC_selG
   , EdKC_selH
   , EdKC_selI
   , EdKC_selJ
   , EdKC_selK
   , EdKC_selL
   , EdKC_selM
   , EdKC_selN
   , EdKC_selO
   , EdKC_selP
   , EdKC_selQ
   , EdKC_selR
   , EdKC_selS
   , EdKC_selT
   , EdKC_selU
   , EdKC_selV
   , EdKC_selW
   , EdKC_selX
   , EdKC_selY
   , EdKC_selZ
   , EdKC_f1
#else
   , EdKC_f1   = 0x100
#endif // SEL_KEYMAP
   , EdKC_f2
   , EdKC_f3
   , EdKC_f4
   , EdKC_f5
   , EdKC_f6
   , EdKC_f7
   , EdKC_f8
   , EdKC_f9
   , EdKC_f10
   , EdKC_f11
   , EdKC_f12
   , EdKC_home
   , EdKC_end
   , EdKC_left
   , EdKC_right
   , EdKC_up
   , EdKC_down
   , EdKC_pgup
   , EdKC_pgdn
   , EdKC_ins
   , EdKC_del
   , EdKC_center
   , EdKC_num0
   , EdKC_num1
   , EdKC_num2
   , EdKC_num3
   , EdKC_num4
   , EdKC_num5
   , EdKC_num6
   , EdKC_num7
   , EdKC_num8
   , EdKC_num9
   , EdKC_numMinus
   , EdKC_numPlus
   , EdKC_numStar
   , EdKC_numSlash
   , EdKC_numEnter
   , EdKC_bksp
   , EdKC_tab
   , EdKC_esc
   , EdKC_enter
   , EdKC_a_0
   , EdKC_a_1
   , EdKC_a_2
   , EdKC_a_3
   , EdKC_a_4
   , EdKC_a_5
   , EdKC_a_6
   , EdKC_a_7
   , EdKC_a_8
   , EdKC_a_9
   , EdKC_a_a
   , EdKC_a_b
   , EdKC_a_c
   , EdKC_a_d
   , EdKC_a_e
   , EdKC_a_f
   , EdKC_a_g
   , EdKC_a_h
   , EdKC_a_i
   , EdKC_a_j
   , EdKC_a_k
   , EdKC_a_l
   , EdKC_a_m
   , EdKC_a_n
   , EdKC_a_o
   , EdKC_a_p
   , EdKC_a_q
   , EdKC_a_r
   , EdKC_a_s
   , EdKC_a_t
   , EdKC_a_u
   , EdKC_a_v
   , EdKC_a_w
   , EdKC_a_x
   , EdKC_a_y
   , EdKC_a_z
   , EdKC_a_f1
   , EdKC_a_f2
   , EdKC_a_f3
   , EdKC_a_f4
   , EdKC_a_f5
   , EdKC_a_f6
   , EdKC_a_f7
   , EdKC_a_f8
   , EdKC_a_f9
   , EdKC_a_f10
   , EdKC_a_f11
   , EdKC_a_f12
   , EdKC_a_BACKTICK
   , EdKC_a_MINUS
   , EdKC_a_EQUAL
   , EdKC_a_LEFT_SQ
   , EdKC_a_RIGHT_SQ
   , EdKC_a_BACKSLASH
   , EdKC_a_SEMICOLON
   , EdKC_a_TICK
   , EdKC_a_COMMA
   , EdKC_a_DOT
   , EdKC_a_SLASH
   , EdKC_a_home
   , EdKC_a_end
   , EdKC_a_left
   , EdKC_a_right
   , EdKC_a_up
   , EdKC_a_down
   , EdKC_a_pgup
   , EdKC_a_pgdn
   , EdKC_a_ins
   , EdKC_a_del
   , EdKC_a_center
   , EdKC_a_num0
   , EdKC_a_num1
   , EdKC_a_num2
   , EdKC_a_num3
   , EdKC_a_num4
   , EdKC_a_num5
   , EdKC_a_num6
   , EdKC_a_num7
   , EdKC_a_num8
   , EdKC_a_num9
   , EdKC_a_numMinus
   , EdKC_a_numPlus
   , EdKC_a_numStar
   , EdKC_a_numSlash
   , EdKC_a_numEnter
   , EdKC_a_space
   , EdKC_a_bksp
   , EdKC_a_tab
   , EdKC_a_esc        // not supported on Win32
   , EdKC_a_enter
   , EdKC_c_0
   , EdKC_c_1
   , EdKC_c_2
   , EdKC_c_3
   , EdKC_c_4
   , EdKC_c_5
   , EdKC_c_6
   , EdKC_c_7
   , EdKC_c_8
   , EdKC_c_9
   , EdKC_c_a
   , EdKC_c_b
   , EdKC_c_c
   , EdKC_c_d
   , EdKC_c_e
   , EdKC_c_f
   , EdKC_c_g
   , EdKC_c_h
   , EdKC_c_i
   , EdKC_c_j
   , EdKC_c_k
   , EdKC_c_l
   , EdKC_c_m
   , EdKC_c_n
   , EdKC_c_o
   , EdKC_c_p
   , EdKC_c_q
   , EdKC_c_r
   , EdKC_c_s
   , EdKC_c_t
   , EdKC_c_u
   , EdKC_c_v
   , EdKC_c_w
   , EdKC_c_x
   , EdKC_c_y
   , EdKC_c_z
   , EdKC_c_f1
   , EdKC_c_f2
   , EdKC_c_f3
   , EdKC_c_f4
   , EdKC_c_f5
   , EdKC_c_f6
   , EdKC_c_f7
   , EdKC_c_f8
   , EdKC_c_f9
   , EdKC_c_f10
   , EdKC_c_f11
   , EdKC_c_f12
   , EdKC_c_BACKTICK
   , EdKC_c_MINUS
   , EdKC_c_EQUAL
   , EdKC_c_LEFT_SQ
   , EdKC_c_RIGHT_SQ
   , EdKC_c_BACKSLASH
   , EdKC_c_SEMICOLON
   , EdKC_c_TICK
   , EdKC_c_COMMA
   , EdKC_c_DOT
   , EdKC_c_SLASH
   , EdKC_c_home
   , EdKC_c_end
   , EdKC_c_left
   , EdKC_c_right
   , EdKC_c_up
   , EdKC_c_down
   , EdKC_c_pgup
   , EdKC_c_pgdn
   , EdKC_c_ins
   , EdKC_c_del
   , EdKC_c_center
   , EdKC_c_num0
   , EdKC_c_num1
   , EdKC_c_num2
   , EdKC_c_num3
   , EdKC_c_num4
   , EdKC_c_num5
   , EdKC_c_num6
   , EdKC_c_num7
   , EdKC_c_num8
   , EdKC_c_num9
   , EdKC_c_numMinus
   , EdKC_c_numPlus
   , EdKC_c_numStar
   , EdKC_c_numSlash
   , EdKC_c_numEnter
   , EdKC_c_space
   , EdKC_c_bksp
   , EdKC_c_tab
   , EdKC_c_esc
   , EdKC_c_enter
   , EdKC_s_f1
   , EdKC_s_f2
   , EdKC_s_f3
   , EdKC_s_f4
   , EdKC_s_f5
   , EdKC_s_f6
   , EdKC_s_f7
   , EdKC_s_f8
   , EdKC_s_f9
   , EdKC_s_f10
   , EdKC_s_f11
   , EdKC_s_f12
   , EdKC_s_home
   , EdKC_s_end
   , EdKC_s_left
   , EdKC_s_right
   , EdKC_s_up
   , EdKC_s_down
   , EdKC_s_pgup
   , EdKC_s_pgdn
   , EdKC_s_ins
   , EdKC_s_del
   , EdKC_s_center
   , EdKC_s_num0
   , EdKC_s_num1
   , EdKC_s_num2
   , EdKC_s_num3
   , EdKC_s_num4
   , EdKC_s_num5
   , EdKC_s_num6
   , EdKC_s_num7
   , EdKC_s_num8
   , EdKC_s_num9
   , EdKC_s_numMinus
   , EdKC_s_numPlus
   , EdKC_s_numStar
   , EdKC_s_numSlash
   , EdKC_s_numEnter
   , EdKC_s_space
   , EdKC_s_bksp
   , EdKC_s_tab
   , EdKC_s_esc
   , EdKC_s_enter

   , EdKC_cs_bksp          //------------------ +EdKC_cs_...
   , EdKC_cs_tab
   , EdKC_cs_center
   , EdKC_cs_enter
   , EdKC_cs_pause
   , EdKC_cs_space
   , EdKC_cs_pgup
   , EdKC_cs_pgdn
   , EdKC_cs_end
   , EdKC_cs_home
   , EdKC_cs_left
   , EdKC_cs_up
   , EdKC_cs_right
   , EdKC_cs_down
   , EdKC_cs_ins
   , EdKC_cs_del
   , EdKC_cs_0
   , EdKC_cs_1
   , EdKC_cs_2
   , EdKC_cs_3
   , EdKC_cs_4
   , EdKC_cs_5
   , EdKC_cs_6
   , EdKC_cs_7
   , EdKC_cs_8
   , EdKC_cs_9
   , EdKC_cs_a
   , EdKC_cs_b
   , EdKC_cs_c
   , EdKC_cs_d
   , EdKC_cs_e
   , EdKC_cs_f
   , EdKC_cs_g
   , EdKC_cs_h
   , EdKC_cs_i
   , EdKC_cs_j
   , EdKC_cs_k
   , EdKC_cs_l
   , EdKC_cs_m
   , EdKC_cs_n
   , EdKC_cs_o
   , EdKC_cs_p
   , EdKC_cs_q
   , EdKC_cs_r
   , EdKC_cs_s
   , EdKC_cs_t
   , EdKC_cs_u
   , EdKC_cs_v
   , EdKC_cs_w
   , EdKC_cs_x
   , EdKC_cs_y
   , EdKC_cs_z
   , EdKC_cs_numStar
   , EdKC_cs_numPlus
   , EdKC_cs_numEnter
   , EdKC_cs_numMinus
   , EdKC_cs_numSlash
   , EdKC_cs_f1
   , EdKC_cs_f2
   , EdKC_cs_f3
   , EdKC_cs_f4
   , EdKC_cs_f5
   , EdKC_cs_f6
   , EdKC_cs_f7
   , EdKC_cs_f8
   , EdKC_cs_f9
   , EdKC_cs_f10
   , EdKC_cs_f11
   , EdKC_cs_f12
   , EdKC_cs_scroll
   , EdKC_cs_SEMICOLON
   , EdKC_cs_EQUAL
   , EdKC_cs_COMMA
   , EdKC_cs_MINUS
   , EdKC_cs_DOT
   , EdKC_cs_SLASH
   , EdKC_cs_BACKTICK
   , EdKC_cs_TICK
   , EdKC_cs_LEFT_SQ
   , EdKC_cs_BACKSLASH
   , EdKC_cs_RIGHT_SQ
   , EdKC_cs_num0
   , EdKC_cs_num1
   , EdKC_cs_num2
   , EdKC_cs_num3
   , EdKC_cs_num4
   , EdKC_cs_num5
   , EdKC_cs_num6
   , EdKC_cs_num7
   , EdKC_cs_num8
   , EdKC_cs_num9          //------------------ -EdKC_cs_...

   , EdKC_pause      // VK 0x13
   , EdKC_a_pause    // VK 0x13
   , EdKC_s_pause    // VK 0x13
   , EdKC_c_numlk    // VK 0x13
   , EdKC_scroll     // VK 0x91
   , EdKC_a_scroll   // VK 0x91
   , EdKC_s_scroll   // VK 0x91

   , EdKC_COUNT
   };

enum { correct_edKC_f1_val =  0x100
#if SEL_KEYMAP
     + (26*2)
#endif
   };

static_assert( EdKC_f1 == correct_edKC_f1_val, "EdKeyCodes::EdKC_f1 == correct_edKC_f1_val" );
static_assert( EdKC_Count == EdKC_COUNT, "EdKC_Count == enum EdKeyCodes" );
