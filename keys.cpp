//
// Copyright 2015-2017 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

struct TKyCd2KyNameTbl {
   EdKC   EdKC_;
   PCChar name;
   };

STATIC_CONST TKyCd2KyNameTbl KyCd2KyNameTbl[] = {
#ifdef fn_argselkeymap
      { EdKC_sela            , "sel_a"          },
      { EdKC_selb            , "sel_b"          },
      { EdKC_selc            , "sel_c"          },
      { EdKC_seld            , "sel_d"          },
      { EdKC_sele            , "sel_e"          },
      { EdKC_self            , "sel_f"          },
      { EdKC_selg            , "sel_g"          },
      { EdKC_selh            , "sel_h"          },
      { EdKC_seli            , "sel_i"          },
      { EdKC_selj            , "sel_j"          },
      { EdKC_selk            , "sel_k"          },
      { EdKC_sell            , "sel_l"          },
      { EdKC_selm            , "sel_m"          },
      { EdKC_seln            , "sel_n"          },
      { EdKC_selo            , "sel_o"          },
      { EdKC_selp            , "sel_p"          },
      { EdKC_selq            , "sel_q"          },
      { EdKC_selr            , "sel_r"          },
      { EdKC_sels            , "sel_s"          },
      { EdKC_selt            , "sel_t"          },
      { EdKC_selu            , "sel_u"          },
      { EdKC_selv            , "sel_v"          },
      { EdKC_selw            , "sel_w"          },
      { EdKC_selx            , "sel_x"          },
      { EdKC_sely            , "sel_y"          },
      { EdKC_selz            , "sel_z"          },
      { EdKC_selA            , "shift+sel_a"    },
      { EdKC_selB            , "shift+sel_b"    },
      { EdKC_selC            , "shift+sel_c"    },
      { EdKC_selD            , "shift+sel_d"    },
      { EdKC_selE            , "shift+sel_e"    },
      { EdKC_selF            , "shift+sel_f"    },
      { EdKC_selG            , "shift+sel_g"    },
      { EdKC_selH            , "shift+sel_h"    },
      { EdKC_selI            , "shift+sel_i"    },
      { EdKC_selJ            , "shift+sel_j"    },
      { EdKC_selK            , "shift+sel_k"    },
      { EdKC_selL            , "shift+sel_l"    },
      { EdKC_selM            , "shift+sel_m"    },
      { EdKC_selN            , "shift+sel_n"    },
      { EdKC_selO            , "shift+sel_o"    },
      { EdKC_selP            , "shift+sel_p"    },
      { EdKC_selQ            , "shift+sel_q"    },
      { EdKC_selR            , "shift+sel_r"    },
      { EdKC_selS            , "shift+sel_s"    },
      { EdKC_selT            , "shift+sel_t"    },
      { EdKC_selU            , "shift+sel_u"    },
      { EdKC_selV            , "shift+sel_v"    },
      { EdKC_selW            , "shift+sel_w"    },
      { EdKC_selX            , "shift+sel_x"    },
      { EdKC_selY            , "shift+sel_y"    },
      { EdKC_selZ            , "shift+sel_z"    },
      { EdKC_sel0            , "sel_0"          },
      { EdKC_sel1            , "sel_1"          },
      { EdKC_sel2            , "sel_2"          },
      { EdKC_sel3            , "sel_3"          },
      { EdKC_sel4            , "sel_4"          },
      { EdKC_sel5            , "sel_5"          },
      { EdKC_sel6            , "sel_6"          },
      { EdKC_sel7            , "sel_7"          },
      { EdKC_sel8            , "sel_8"          },
      { EdKC_sel9            , "sel_9"          },
      { EdKC_selSPACE        , "sel_space"      },
      { EdKC_selLPAREN       , "sel_("          },
      { EdKC_selRPAREN       , "sel_)"          },
      { EdKC_selLCURLY       , "sel_{"          },
      { EdKC_selRCURLY       , "sel_}"          },
      { EdKC_selLEFT_SQ      , "sel_["          },
      { EdKC_selRIGHT_SQ     , "sel_]"          },
      { EdKC_selLT           , "sel_<"          },
      { EdKC_selGT           , "sel_>"          },
      { EdKC_selPIPE         , "sel_|"          },
      { EdKC_selBACKSLASH    , "sel_\\"         },
      { EdKC_selSLASH        , "sel_/"          },
      { EdKC_selQMARK        , "sel_?"          },
      { EdKC_selEQUAL        , "sel_="          },
      { EdKC_selPLUS         , "sel_+"          },
      { EdKC_selMINUS        , "sel_-"          },
      { EdKC_selUNDERSCORE   , "sel__"          },
      { EdKC_selSEMICOLON    , "sel_;"          },
      { EdKC_selCOLON        , "sel_:"          },
      { EdKC_selCOMMA        , "sel_,"          },
      { EdKC_selDOT          , "sel_."          },
      { EdKC_selBACKTICK     , "sel_`"          },
      { EdKC_selTILDE        , "sel_~"          },
      { EdKC_selEX           , "sel_!"          },
      { EdKC_selAT           , "sel_@"          },
      { EdKC_selPOUND        , "sel_#"          },
      { EdKC_selDOLLAR       , "sel_$"          },
      { EdKC_selPERCENT      , "sel_%"          },
      { EdKC_selCARET        , "sel_^"          },
      { EdKC_selAND          , "sel_&"          },
      { EdKC_selSTAR         , "sel_*"          },
      { EdKC_selDQUOTE       , "sel_\""         },
#endif // #ifdef fn_argselkeymap
      { EdKC_f1              , "f1"             },
      { EdKC_f2              , "f2"             },
      { EdKC_f3              , "f3"             },
      { EdKC_f4              , "f4"             },
      { EdKC_f5              , "f5"             },
      { EdKC_f6              , "f6"             },
      { EdKC_f7              , "f7"             },
      { EdKC_f8              , "f8"             },
      { EdKC_f9              , "f9"             },
      { EdKC_f10             , "f10"            },
      { EdKC_f11             , "f11"            },
      { EdKC_f12             , "f12"            },
      { EdKC_home            , "home"           },
      { EdKC_end             , "end"            },
      { EdKC_left            , "left"           },
      { EdKC_right           , "right"          },
      { EdKC_up              , "up"             },
      { EdKC_down            , "down"           },
      { EdKC_pgup            , "pgup"           },
      { EdKC_pgdn            , "pgdn"           },
      { EdKC_ins             , "ins"            },
      { EdKC_del             , "del"            },
      { EdKC_center          , "center"         },
      { EdKC_num0            , "num+0"          },
      { EdKC_num1            , "num+1"          },
      { EdKC_num2            , "num+2"          },
      { EdKC_num3            , "num+3"          },
      { EdKC_num4            , "num+4"          },
      { EdKC_num5            , "num+5"          },
      { EdKC_num6            , "num+6"          },
      { EdKC_num7            , "num+7"          },
      { EdKC_num8            , "num+8"          },
      { EdKC_num9            , "num+9"          },
      { EdKC_numMinus        , "num-"           },
      { EdKC_numPlus         , "num+"           },
      { EdKC_numStar         , "num*"           },
      { EdKC_numSlash        , "num/"           },
      { EdKC_numEnter        , "num+enter"      },
      { EdKC_bksp            , "bksp"           },
      { EdKC_tab             , "tab"            },
      { EdKC_esc             , "esc"            },
      { EdKC_enter           , "enter"          },
      { EdKC_a_0             , "alt+0"          },
      { EdKC_a_1             , "alt+1"          },
      { EdKC_a_2             , "alt+2"          },
      { EdKC_a_3             , "alt+3"          },
      { EdKC_a_4             , "alt+4"          },
      { EdKC_a_5             , "alt+5"          },
      { EdKC_a_6             , "alt+6"          },
      { EdKC_a_7             , "alt+7"          },
      { EdKC_a_8             , "alt+8"          },
      { EdKC_a_9             , "alt+9"          },
      { EdKC_a_a             , "alt+a"          },
      { EdKC_a_b             , "alt+b"          },
      { EdKC_a_c             , "alt+c"          },
      { EdKC_a_d             , "alt+d"          },
      { EdKC_a_e             , "alt+e"          },
      { EdKC_a_f             , "alt+f"          },
      { EdKC_a_g             , "alt+g"          },
      { EdKC_a_h             , "alt+h"          },
      { EdKC_a_i             , "alt+i"          },
      { EdKC_a_j             , "alt+j"          },
      { EdKC_a_k             , "alt+k"          },
      { EdKC_a_l             , "alt+l"          },
      { EdKC_a_m             , "alt+m"          },
      { EdKC_a_n             , "alt+n"          },
      { EdKC_a_o             , "alt+o"          },
      { EdKC_a_p             , "alt+p"          },
      { EdKC_a_q             , "alt+q"          },
      { EdKC_a_r             , "alt+r"          },
      { EdKC_a_s             , "alt+s"          },
      { EdKC_a_t             , "alt+t"          },
      { EdKC_a_u             , "alt+u"          },
      { EdKC_a_v             , "alt+v"          },
      { EdKC_a_w             , "alt+w"          },
      { EdKC_a_x             , "alt+x"          },
      { EdKC_a_y             , "alt+y"          },
      { EdKC_a_z             , "alt+z"          },
      { EdKC_a_f1            , "alt+f1"         },
      { EdKC_a_f2            , "alt+f2"         },
      { EdKC_a_f3            , "alt+f3"         },
      { EdKC_a_f4            , "alt+f4"         },
      { EdKC_a_f5            , "alt+f5"         },
      { EdKC_a_f6            , "alt+f6"         },
      { EdKC_a_f7            , "alt+f7"         },
      { EdKC_a_f8            , "alt+f8"         },
      { EdKC_a_f9            , "alt+f9"         },
      { EdKC_a_f10           , "alt+f10"        },
      { EdKC_a_f11           , "alt+f11"        },
      { EdKC_a_f12           , "alt+f12"        },
      { EdKC_a_BACKTICK      , "alt+`"          },
      { EdKC_a_MINUS         , "alt+-"          },
      { EdKC_a_EQUAL         , "alt+="          },
      { EdKC_a_LEFT_SQ       , "alt+["          },
      { EdKC_a_RIGHT_SQ      , "alt+]"          },
      { EdKC_a_BACKSLASH     , "alt+\\"         },
      { EdKC_a_SEMICOLON     , "alt+;"          },
      { EdKC_a_TICK          , "alt+'"          },
      { EdKC_a_COMMA         , "alt+,"          },
      { EdKC_a_DOT           , "alt+."          },
      { EdKC_a_SLASH         , "alt+/"          },
      { EdKC_a_home          , "alt+home"       },
      { EdKC_a_end           , "alt+end"        },
      { EdKC_a_left          , "alt+left"       },
      { EdKC_a_right         , "alt+right"      },
      { EdKC_a_up            , "alt+up"         },
      { EdKC_a_down          , "alt+down"       },
      { EdKC_a_pgup          , "alt+pgup"       },
      { EdKC_a_pgdn          , "alt+pgdn"       },
      { EdKC_a_ins           , "alt+ins"        },
      { EdKC_a_del           , "alt+del"        },
      { EdKC_a_center        , "alt+center"     },
      { EdKC_a_num0          , "alt+num0"       },
      { EdKC_a_num1          , "alt+num1"       },
      { EdKC_a_num2          , "alt+num2"       },
      { EdKC_a_num3          , "alt+num3"       },
      { EdKC_a_num4          , "alt+num4"       },
      { EdKC_a_num5          , "alt+num5"       },
      { EdKC_a_num6          , "alt+num6"       },
      { EdKC_a_num7          , "alt+num7"       },
      { EdKC_a_num8          , "alt+num8"       },
      { EdKC_a_num9          , "alt+num9"       },
      { EdKC_a_numMinus      , "alt+num-"       },
      { EdKC_a_numPlus       , "alt+num+"       },
      { EdKC_a_numStar       , "alt+num*"       },
      { EdKC_a_numSlash      , "alt+num/"       },
      { EdKC_a_numEnter      , "alt+numenter"   },
      { EdKC_a_space         , "alt+space"      },
      { EdKC_a_bksp          , "alt+bksp"       },
      { EdKC_a_tab           , "alt+tab"        },
      { EdKC_a_esc           , "alt+esc"        },
      { EdKC_a_enter         , "alt+enter"      },
      { EdKC_c_0             , "ctrl+0"         },
      { EdKC_c_1             , "ctrl+1"         },
      { EdKC_c_2             , "ctrl+2"         },
      { EdKC_c_3             , "ctrl+3"         },
      { EdKC_c_4             , "ctrl+4"         },
      { EdKC_c_5             , "ctrl+5"         },
      { EdKC_c_6             , "ctrl+6"         },
      { EdKC_c_7             , "ctrl+7"         },
      { EdKC_c_8             , "ctrl+8"         },
      { EdKC_c_9             , "ctrl+9"         },
      { EdKC_c_a             , "ctrl+a"         },
      { EdKC_c_b             , "ctrl+b"         },
      { EdKC_c_c             , "ctrl+c"         },
      { EdKC_c_d             , "ctrl+d"         },
      { EdKC_c_e             , "ctrl+e"         },
      { EdKC_c_f             , "ctrl+f"         },
      { EdKC_c_g             , "ctrl+g"         },
      { EdKC_c_h             , "ctrl+h"         },
      { EdKC_c_i             , "ctrl+i"         },
      { EdKC_c_j             , "ctrl+j"         },
      { EdKC_c_k             , "ctrl+k"         },
      { EdKC_c_l             , "ctrl+l"         },
      { EdKC_c_m             , "ctrl+m"         },
      { EdKC_c_n             , "ctrl+n"         },
      { EdKC_c_o             , "ctrl+o"         },
      { EdKC_c_p             , "ctrl+p"         },
      { EdKC_c_q             , "ctrl+q"         },
      { EdKC_c_r             , "ctrl+r"         },
      { EdKC_c_s             , "ctrl+s"         },
      { EdKC_c_t             , "ctrl+t"         },
      { EdKC_c_u             , "ctrl+u"         },
      { EdKC_c_v             , "ctrl+v"         },
      { EdKC_c_w             , "ctrl+w"         },
      { EdKC_c_x             , "ctrl+x"         },
      { EdKC_c_y             , "ctrl+y"         },
      { EdKC_c_z             , "ctrl+z"         },
      { EdKC_c_f1            , "ctrl+f1"        },
      { EdKC_c_f2            , "ctrl+f2"        },
      { EdKC_c_f3            , "ctrl+f3"        },
      { EdKC_c_f4            , "ctrl+f4"        },
      { EdKC_c_f5            , "ctrl+f5"        },
      { EdKC_c_f6            , "ctrl+f6"        },
      { EdKC_c_f7            , "ctrl+f7"        },
      { EdKC_c_f8            , "ctrl+f8"        },
      { EdKC_c_f9            , "ctrl+f9"        },
      { EdKC_c_f10           , "ctrl+f10"       },
      { EdKC_c_f11           , "ctrl+f11"       },
      { EdKC_c_f12           , "ctrl+f12"       },
      { EdKC_c_BACKTICK      , "ctrl+`"         },
      { EdKC_c_MINUS         , "ctrl+-"         },
      { EdKC_c_EQUAL         , "ctrl+="         },
      { EdKC_c_LEFT_SQ       , "ctrl+["         },
      { EdKC_c_RIGHT_SQ      , "ctrl+]"         },
      { EdKC_c_BACKSLASH     , "ctrl+\\"        },
      { EdKC_c_SEMICOLON     , "ctrl+;"         },
      { EdKC_c_TICK          , "ctrl+'"         },
      { EdKC_c_COMMA         , "ctrl+,"         },
      { EdKC_c_DOT           , "ctrl+."         },
      { EdKC_c_SLASH         , "ctrl+/"         },
      { EdKC_c_home          , "ctrl+home"      },
      { EdKC_c_end           , "ctrl+end"       },
      { EdKC_c_left          , "ctrl+left"      },
      { EdKC_c_right         , "ctrl+right"     },
      { EdKC_c_up            , "ctrl+up"        },
      { EdKC_c_down          , "ctrl+down"      },
      { EdKC_c_pgup          , "ctrl+pgup"      },
      { EdKC_c_pgdn          , "ctrl+pgdn"      },
      { EdKC_c_ins           , "ctrl+ins"       },
      { EdKC_c_del           , "ctrl+del"       },
      { EdKC_c_center        , "ctrl+center"    },
      { EdKC_c_num0          , "ctrl+num0"      },
      { EdKC_c_num1          , "ctrl+num1"      },
      { EdKC_c_num2          , "ctrl+num2"      },
      { EdKC_c_num3          , "ctrl+num3"      },
      { EdKC_c_num4          , "ctrl+num4"      },
      { EdKC_c_num5          , "ctrl+num5"      },
      { EdKC_c_num6          , "ctrl+num6"      },
      { EdKC_c_num7          , "ctrl+num7"      },
      { EdKC_c_num8          , "ctrl+num8"      },
      { EdKC_c_num9          , "ctrl+num9"      },
      { EdKC_c_numMinus      , "ctrl+num-"      },
      { EdKC_c_numPlus       , "ctrl+num+"      },
      { EdKC_c_numStar       , "ctrl+num*"      },
      { EdKC_c_numSlash      , "ctrl+num/"      },
      { EdKC_c_numEnter      , "ctrl+numenter"  },
      { EdKC_c_space         , "ctrl+space"     },
      { EdKC_c_bksp          , "ctrl+bksp"      },
      { EdKC_c_tab           , "ctrl+tab"       },
      { EdKC_c_esc           , "ctrl+esc"       },
      { EdKC_c_enter         , "ctrl+enter"     },
      { EdKC_s_f1            , "shift+f1"       },
      { EdKC_s_f2            , "shift+f2"       },
      { EdKC_s_f3            , "shift+f3"       },
      { EdKC_s_f4            , "shift+f4"       },
      { EdKC_s_f5            , "shift+f5"       },
      { EdKC_s_f6            , "shift+f6"       },
      { EdKC_s_f7            , "shift+f7"       },
      { EdKC_s_f8            , "shift+f8"       },
      { EdKC_s_f9            , "shift+f9"       },
      { EdKC_s_f10           , "shift+f10"      },
      { EdKC_s_f11           , "shift+f11"      },
      { EdKC_s_f12           , "shift+f12"      },
      { EdKC_s_home          , "shift+home"     },
      { EdKC_s_end           , "shift+end"      },
      { EdKC_s_left          , "shift+left"     },
      { EdKC_s_right         , "shift+right"    },
      { EdKC_s_up            , "shift+up"       },
      { EdKC_s_down          , "shift+down"     },
      { EdKC_s_pgup          , "shift+pgup"     },
      { EdKC_s_pgdn          , "shift+pgdn"     },
      { EdKC_s_ins           , "shift+ins"      },
      { EdKC_s_del           , "shift+del"      },
      { EdKC_s_center        , "shift+center"   },
      { EdKC_s_num0          , "shift+num0"     },
      { EdKC_s_num1          , "shift+num1"     },
      { EdKC_s_num2          , "shift+num2"     },
      { EdKC_s_num3          , "shift+num3"     },
      { EdKC_s_num4          , "shift+num4"     },
      { EdKC_s_num5          , "shift+num5"     },
      { EdKC_s_num6          , "shift+num6"     },
      { EdKC_s_num7          , "shift+num7"     },
      { EdKC_s_num8          , "shift+num8"     },
      { EdKC_s_num9          , "shift+num9"     },
      { EdKC_s_numMinus      , "shift+num-"     },
      { EdKC_s_numPlus       , "shift+num+"     },
      { EdKC_s_numStar       , "shift+num*"     },
      { EdKC_s_numSlash      , "shift+num/"     },
      { EdKC_s_numEnter      , "shift+numenter" },
      { EdKC_s_space         , "shift+space"    },
      { EdKC_s_bksp          , "shift+bksp"     },
      { EdKC_s_tab           , "shift+tab"      },
      { EdKC_s_esc           , "shift+esc"      },
      { EdKC_s_enter         , "shift+enter"    },

      { EdKC_cs_bksp         , "ctrl+shift+bksp"     },
      { EdKC_cs_tab          , "ctrl+shift+tab"      },
      { EdKC_cs_center       , "ctrl+shift+center"   },
      { EdKC_cs_enter        , "ctrl+shift+enter"    },
      { EdKC_cs_pause        , "ctrl+shift+pause"    },
      { EdKC_cs_space        , "ctrl+shift+space"    },
      { EdKC_cs_pgup         , "ctrl+shift+pgup"     },
      { EdKC_cs_pgdn         , "ctrl+shift+pgdn"     },
      { EdKC_cs_end          , "ctrl+shift+end"      },
      { EdKC_cs_home         , "ctrl+shift+home"     },
      { EdKC_cs_left         , "ctrl+shift+left"     },
      { EdKC_cs_up           , "ctrl+shift+up"       },
      { EdKC_cs_right        , "ctrl+shift+right"    },
      { EdKC_cs_down         , "ctrl+shift+down"     },
      { EdKC_cs_ins          , "ctrl+shift+ins"      },
      { EdKC_cs_del          , "ctrl+shift+del"      },
      { EdKC_cs_0            , "ctrl+shift+0"        },
      { EdKC_cs_1            , "ctrl+shift+1"        },
      { EdKC_cs_2            , "ctrl+shift+2"        },
      { EdKC_cs_3            , "ctrl+shift+3"        },
      { EdKC_cs_4            , "ctrl+shift+4"        },
      { EdKC_cs_5            , "ctrl+shift+5"        },
      { EdKC_cs_6            , "ctrl+shift+6"        },
      { EdKC_cs_7            , "ctrl+shift+7"        },
      { EdKC_cs_8            , "ctrl+shift+8"        },
      { EdKC_cs_9            , "ctrl+shift+9"        },
      { EdKC_cs_a            , "ctrl+shift+a"        },
      { EdKC_cs_b            , "ctrl+shift+b"        },
      { EdKC_cs_c            , "ctrl+shift+c"        },
      { EdKC_cs_d            , "ctrl+shift+d"        },
      { EdKC_cs_e            , "ctrl+shift+e"        },
      { EdKC_cs_f            , "ctrl+shift+f"        },
      { EdKC_cs_g            , "ctrl+shift+g"        },
      { EdKC_cs_h            , "ctrl+shift+h"        },
      { EdKC_cs_i            , "ctrl+shift+i"        },
      { EdKC_cs_j            , "ctrl+shift+j"        },
      { EdKC_cs_k            , "ctrl+shift+k"        },
      { EdKC_cs_l            , "ctrl+shift+l"        },
      { EdKC_cs_m            , "ctrl+shift+m"        },
      { EdKC_cs_n            , "ctrl+shift+n"        },
      { EdKC_cs_o            , "ctrl+shift+o"        },
      { EdKC_cs_p            , "ctrl+shift+p"        },
      { EdKC_cs_q            , "ctrl+shift+q"        },
      { EdKC_cs_r            , "ctrl+shift+r"        },
      { EdKC_cs_s            , "ctrl+shift+s"        },
      { EdKC_cs_t            , "ctrl+shift+t"        },
      { EdKC_cs_u            , "ctrl+shift+u"        },
      { EdKC_cs_v            , "ctrl+shift+v"        },
      { EdKC_cs_w            , "ctrl+shift+w"        },
      { EdKC_cs_x            , "ctrl+shift+x"        },
      { EdKC_cs_y            , "ctrl+shift+y"        },
      { EdKC_cs_z            , "ctrl+shift+z"        },
      { EdKC_cs_numStar      , "ctrl+shift+num*"     },
      { EdKC_cs_numPlus      , "ctrl+shift+num+"     },
      { EdKC_cs_numEnter     , "ctrl+shift+numenter" },
      { EdKC_cs_numMinus     , "ctrl+shift+num-"     },
      { EdKC_cs_numSlash     , "ctrl+shift+num/"     },
      { EdKC_cs_f1           , "ctrl+shift+f1"       },
      { EdKC_cs_f2           , "ctrl+shift+f2"       },
      { EdKC_cs_f3           , "ctrl+shift+f3"       },
      { EdKC_cs_f4           , "ctrl+shift+f4"       },
      { EdKC_cs_f5           , "ctrl+shift+f5"       },
      { EdKC_cs_f6           , "ctrl+shift+f6"       },
      { EdKC_cs_f7           , "ctrl+shift+f7"       },
      { EdKC_cs_f8           , "ctrl+shift+f8"       },
      { EdKC_cs_f9           , "ctrl+shift+f9"       },
      { EdKC_cs_f10          , "ctrl+shift+f10"      },
      { EdKC_cs_f11          , "ctrl+shift+f11"      },
      { EdKC_cs_f12          , "ctrl+shift+f12"      },
      { EdKC_cs_scroll       , "ctrl+shift+scroll"   },
      { EdKC_cs_SEMICOLON    , "ctrl+shift+;"        },
      { EdKC_cs_EQUAL        , "ctrl+shift+="        },
      { EdKC_cs_COMMA        , "ctrl+shift+,"        },
      { EdKC_cs_MINUS        , "ctrl+shift+-"        },
      { EdKC_cs_DOT          , "ctrl+shift+."        },
      { EdKC_cs_SLASH        , "ctrl+shift+/"        },
      { EdKC_cs_BACKTICK     , "ctrl+shift+`"        },
      { EdKC_cs_TICK         , "ctrl+shift+'"        },
      { EdKC_cs_LEFT_SQ      , "ctrl+shift+["        },
      { EdKC_cs_BACKSLASH    , "ctrl+shift+\\"       },
      { EdKC_cs_RIGHT_SQ     , "ctrl+shift+]"        },
      { EdKC_cs_num0         , "ctrl+shift+num0"     },
      { EdKC_cs_num1         , "ctrl+shift+num1"     },
      { EdKC_cs_num2         , "ctrl+shift+num2"     },
      { EdKC_cs_num3         , "ctrl+shift+num3"     },
      { EdKC_cs_num4         , "ctrl+shift+num4"     },
      { EdKC_cs_num5         , "ctrl+shift+num5"     },
      { EdKC_cs_num6         , "ctrl+shift+num6"     },
      { EdKC_cs_num7         , "ctrl+shift+num7"     },
      { EdKC_cs_num8         , "ctrl+shift+num8"     },
      { EdKC_cs_num9         , "ctrl+shift+num9"     },

      { EdKC_pause           , "pause"               },
      { EdKC_a_pause         , "alt+pause"           },
      { EdKC_s_pause         , "shift+pause"         },
      { EdKC_c_numlk         , "ctrl+numlk"          },
      { EdKC_scroll          , "scroll"              },
      { EdKC_a_scroll        , "alt+scroll"          },
      { EdKC_s_scroll        , "shift+scroll"        },
   };

static_assert( ELEMENTS( KyCd2KyNameTbl ) == (EdKC_COUNT - 256), "KyCd2KyNameTbl ) == (EdKC_COUNT - 256)" );

STATIC_FXN int MaxKeyNameLen_() {
   auto maxLen(0);
   for( const auto &ky2Nm : KyCd2KyNameTbl ) {
      NoLessThan( &maxLen, Strlen( ky2Nm.name ) );
      }
   return maxLen;
   }

// GLOBAL_CONST int g_MaxKeyNameLen( MaxKeyNameLen_() ); // <-- Exuberant Ctags DOES NOT tag g_MaxKeyNameLen
//        const int g_MaxKeyNameLen( MaxKeyNameLen_() ); // <-- Exuberant Ctags DOES NOT tag g_MaxKeyNameLen
   GLOBAL_CONST int g_MaxKeyNameLen = MaxKeyNameLen_();  // <-- Exuberant Ctags DOES     tag g_MaxKeyNameLen

int EdkcOfKeyNm( stref keyNm ) {
   if( keyNm.length() == 1 ) {
      return keyNm[0];
      }
   for( const auto &ky2Nm : KyCd2KyNameTbl ) {
      if( 0==cmpi( ky2Nm.name, keyNm ) ) {
         return ky2Nm.EdKC_;
         }
      }
   return 0;
   }

std::string &KeyNmOfEdkc( std::string &dest, int edKC ) {
   for( const auto &ky2Nm : KyCd2KyNameTbl ) {
      if( ky2Nm.EdKC_ == edKC ) {
         dest.assign( ky2Nm.name );
         return dest;
         }
      }
   if( edKC < 0x100 && isprint( edKC ) ) {
      dest.assign( 1, static_cast<char>(edKC) );
      return dest;
      }
   else {
      dest.assign( FmtStr<24>( "edKC=0x%X", edKC ) );
      return dest;
      }
   }

std::string KeyNmOfEdkc( int edKC ) {
   std::string dest;
   KeyNmOfEdkc( dest, edKC );
   return dest;
   }

char CharAsciiFromKybd() {
   return ConIn::EdKC_Ascii_FromNextKey().Ascii;
   }

STATIC_CONST char spinners[] = { '-', '\\', '|', '/' };

class SpinChar {
   int d_spindex;
public:
   SpinChar() : d_spindex(0) {}
   char next() {
      const auto rv( spinners[d_spindex++] );
      d_spindex %= sizeof(spinners);
      return rv;
      }
   };

EdKc_t ModalKeyRemap( EdKc_t edKC ) {
  #ifdef fn_argselkeymap
   STATIC_VAR int s_KC2selKC[EdKC_f1];
   if( !s_KC2selKC['0'] ) {
      // attempts to do this in the s_KC2selKC initializer, e.g. "['0'] = EdKC_sel0," yielded
      // 'sorry, unimplemented: non-trivial designated initializers not supported'
      // yet s_Key2CmdTbl[] works; the difference is that s_Key2CmdTbl[] initialized every array
      // slot (and probably in numeric order), while here we are intending to perform sparse init,
      // per https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html (but apparently GCC impl
      // this differently for C vs. C++) also not caring about order.
      s_KC2selKC['0'] = EdKC_sel0           ;
      s_KC2selKC['1'] = EdKC_sel1           ;
      s_KC2selKC['2'] = EdKC_sel2           ;
      s_KC2selKC['3'] = EdKC_sel3           ;
      s_KC2selKC['4'] = EdKC_sel4           ;
      s_KC2selKC['5'] = EdKC_sel5           ;
      s_KC2selKC['6'] = EdKC_sel6           ;
      s_KC2selKC['7'] = EdKC_sel7           ;
      s_KC2selKC['8'] = EdKC_sel8           ;
      s_KC2selKC['9'] = EdKC_sel9           ;
      s_KC2selKC['A'] = EdKC_selA           ;
      s_KC2selKC['B'] = EdKC_selB           ;
      s_KC2selKC['C'] = EdKC_selC           ;
      s_KC2selKC['D'] = EdKC_selD           ;
      s_KC2selKC['E'] = EdKC_selE           ;
      s_KC2selKC['F'] = EdKC_selF           ;
      s_KC2selKC['G'] = EdKC_selG           ;
      s_KC2selKC['H'] = EdKC_selH           ;
      s_KC2selKC['I'] = EdKC_selI           ;
      s_KC2selKC['J'] = EdKC_selJ           ;
      s_KC2selKC['K'] = EdKC_selK           ;
      s_KC2selKC['L'] = EdKC_selL           ;
      s_KC2selKC['M'] = EdKC_selM           ;
      s_KC2selKC['N'] = EdKC_selN           ;
      s_KC2selKC['O'] = EdKC_selO           ;
      s_KC2selKC['P'] = EdKC_selP           ;
      s_KC2selKC['Q'] = EdKC_selQ           ;
      s_KC2selKC['R'] = EdKC_selR           ;
      s_KC2selKC['S'] = EdKC_selS           ;
      s_KC2selKC['T'] = EdKC_selT           ;
      s_KC2selKC['U'] = EdKC_selU           ;
      s_KC2selKC['V'] = EdKC_selV           ;
      s_KC2selKC['W'] = EdKC_selW           ;
      s_KC2selKC['X'] = EdKC_selX           ;
      s_KC2selKC['Y'] = EdKC_selY           ;
      s_KC2selKC['Z'] = EdKC_selZ           ;
      s_KC2selKC['a'] = EdKC_sela           ;
      s_KC2selKC['b'] = EdKC_selb           ;
      s_KC2selKC['c'] = EdKC_selc           ;
      s_KC2selKC['d'] = EdKC_seld           ;
      s_KC2selKC['e'] = EdKC_sele           ;
      s_KC2selKC['f'] = EdKC_self           ;
      s_KC2selKC['g'] = EdKC_selg           ;
      s_KC2selKC['h'] = EdKC_selh           ;
      s_KC2selKC['i'] = EdKC_seli           ;
      s_KC2selKC['j'] = EdKC_selj           ;
      s_KC2selKC['k'] = EdKC_selk           ;
      s_KC2selKC['l'] = EdKC_sell           ;
      s_KC2selKC['m'] = EdKC_selm           ;
      s_KC2selKC['n'] = EdKC_seln           ;
      s_KC2selKC['o'] = EdKC_selo           ;
      s_KC2selKC['p'] = EdKC_selp           ;
      s_KC2selKC['q'] = EdKC_selq           ;
      s_KC2selKC['r'] = EdKC_selr           ;
      s_KC2selKC['s'] = EdKC_sels           ;
      s_KC2selKC['t'] = EdKC_selt           ;
      s_KC2selKC['u'] = EdKC_selu           ;
      s_KC2selKC['v'] = EdKC_selv           ;
      s_KC2selKC['w'] = EdKC_selw           ;
      s_KC2selKC['x'] = EdKC_selx           ;
      s_KC2selKC['y'] = EdKC_sely           ;
      s_KC2selKC['z'] = EdKC_selz           ;
      s_KC2selKC[' '] = EdKC_selSPACE       ;
      s_KC2selKC['('] = EdKC_selLPAREN      ;
      s_KC2selKC[')'] = EdKC_selRPAREN      ;
      s_KC2selKC['{'] = EdKC_selLCURLY      ;
      s_KC2selKC['}'] = EdKC_selRCURLY      ;
      s_KC2selKC['['] = EdKC_selLEFT_SQ     ;
      s_KC2selKC[']'] = EdKC_selRIGHT_SQ    ;
      s_KC2selKC['<'] = EdKC_selLT          ;
      s_KC2selKC['>'] = EdKC_selGT          ;
      s_KC2selKC['|'] = EdKC_selPIPE        ;
      s_KC2selKC['\\']= EdKC_selBACKSLASH   ;
      s_KC2selKC['/'] = EdKC_selSLASH       ;
      s_KC2selKC['?'] = EdKC_selQMARK       ;
      s_KC2selKC['='] = EdKC_selEQUAL       ;
      s_KC2selKC['+'] = EdKC_selPLUS        ;
      s_KC2selKC['-'] = EdKC_selMINUS       ;
      s_KC2selKC['_'] = EdKC_selUNDERSCORE  ;
      s_KC2selKC[';'] = EdKC_selSEMICOLON   ;
      s_KC2selKC[':'] = EdKC_selCOLON       ;
      s_KC2selKC[','] = EdKC_selCOMMA       ;
      s_KC2selKC['.'] = EdKC_selDOT         ;
      s_KC2selKC['`'] = EdKC_selBACKTICK    ;
      s_KC2selKC['~'] = EdKC_selTILDE       ;
      s_KC2selKC['!'] = EdKC_selEX          ;
      s_KC2selKC['@'] = EdKC_selAT          ;
      s_KC2selKC['#'] = EdKC_selPOUND       ;
      s_KC2selKC['$'] = EdKC_selDOLLAR      ;
      s_KC2selKC['%'] = EdKC_selPERCENT     ;
      s_KC2selKC['^'] = EdKC_selCARET       ;
      s_KC2selKC['&'] = EdKC_selAND         ;
      s_KC2selKC['*'] = EdKC_selSTAR        ;
      s_KC2selKC['"'] = EdKC_selDQUOTE      ;
      s_KC2selKC['\'']= EdKC_selSLASH       ;
      }
   if( SelKeymapEnabled() && edKC < ELEMENTS(s_KC2selKC) && s_KC2selKC[edKC] ) {
      return s_KC2selKC[edKC];
      }
  #endif
   return edKC;
   }

void WaitForKey( int secondsToWait ) {
// if( Interpreter::Interpreting() )
//    return;
   secondsToWait = std::min( secondsToWait, 1 );
   ConIn::FlushKeyQueueAnythingFlushed();
   SpinChar sc;
   auto timeNow( time( nullptr ) );
   const auto timeEnd( timeNow + secondsToWait + 1 );
   auto maxWidth(0);
   do {
      if( ConIn::FlushKeyQueueAnythingFlushed() ) {
         break;
         }
      SleepMs( 50 );
      timeNow = time( nullptr );
      const auto spinner( sc.next() );
      FmtStr<71> msg( " You have %c%2" PR_TIMET "%c seconds to press any key ", spinner, (timeEnd - timeNow), spinner );
      const auto mlen( msg.Len() );
      NoLessThan( &maxWidth, mlen );
      VidWrStrColorFlush( DialogLine(), EditScreenCols() - mlen, msg.k_str(), mlen, g_colorError, true );
      } while( timeNow < timeEnd );
   VidWrStrColorFlush( DialogLine(), EditScreenCols() - maxWidth, "", 0, g_colorInfo, true );
   }

#ifdef fn_waitkey15

bool ARG::waitkey15() {
   WaitForKey( 15 );
   return true;
   }

#endif
