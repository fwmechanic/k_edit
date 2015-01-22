
# EdKC -> ncurses Table

Data in following table gathered by hitting ctrl+t (ARG::tell), hitting the corresponding key, and reading the dialog line to obtain the key name.

## Legend for EdKC -> ncurses Table

 * `y`: working as expected
 * `hooked`: not seen by ncurses K because intercepted by WM, DE, or other
 * `no response`: no visible respose (systemwide) to key being struck.
 * `is X, s/b A`: when the key is struck, EdKC==X is received when ==A is expected
 * `is X`: when the key is struck, EdKC==X is received (implicitly: when another is expected)
 * `ascii 'X'`: when the key is struck, EdKC==EdKC_a (where a is an ASCII code) is received (implicitly: when another is expected);

| EdKC              | ncurses |
|-------------------|---------|
| EdKC_f1           | y |
| EdKC_f2           | y |
| EdKC_f3           | y |
| EdKC_f4           | y |
| EdKC_f5           | y |
| EdKC_f6           | y |
| EdKC_f7           | y |
| EdKC_f8           | y |
| EdKC_f9           | y |
| EdKC_f10          | y |
| EdKC_f11          | Hooked |
| EdKC_f12          | y |
| EdKC_home         | y |
| EdKC_end          | y |
| EdKC_left         | y |
| EdKC_right        | y |
| EdKC_up           | y |
| EdKC_down         | y |
| EdKC_pgup         | y |
| EdKC_pgdn         | y |
| EdKC_ins          | y |
| EdKC_del          | y |
| EdKC_center       | y |
| EdKC_num0         | is 0, s/b num0 |
| EdKC_num1         | is 1, s/b num1 |
| EdKC_num2         | is 2, s/b num2 |
| EdKC_num3         | is 3, s/b num3 |
| EdKC_num4         | is 4, s/b num4 |
| EdKC_num5         | is 5, s/b num5 |
| EdKC_num6         | is 6, s/b num6 |
| EdKC_num7         | is 7, s/b num7 |
| EdKC_num8         | is 8, s/b num8 |
| EdKC_num9         | is 9, s/b num9 |
| EdKC_numMinus     | y |
| EdKC_numPlus      | y |
| EdKC_numStar      | y |
| EdKC_numSlash     | y |
| EdKC_numEnter     | y |
| EdKC_space        | ascii ' ' (?) |
| EdKC_bksp         | y |
| EdKC_tab          | y |
| EdKC_esc          | y |
| EdKC_enter        | y |
| EdKC_a_0          | y |
| EdKC_a_1          | y |
| EdKC_a_2          | y |
| EdKC_a_3          | y |
| EdKC_a_4          | y |
| EdKC_a_5          | y |
| EdKC_a_6          | y |
| EdKC_a_7          | y |
| EdKC_a_8          | y |
| EdKC_a_9          | y |
| EdKC_a_a          | y |
| EdKC_a_b          | y |
| EdKC_a_c          | y |
| EdKC_a_d          | y |
| EdKC_a_e          | y |
| EdKC_a_f          | y |
| EdKC_a_g          | y |
| EdKC_a_h          | y |
| EdKC_a_i          | y |
| EdKC_a_j          | y |
| EdKC_a_k          | y |
| EdKC_a_l          | y |
| EdKC_a_m          | y |
| EdKC_a_n          | y |
| EdKC_a_o          | y |
| EdKC_a_p          | y |
| EdKC_a_q          | y |
| EdKC_a_r          | y |
| EdKC_a_s          | y |
| EdKC_a_t          | y |
| EdKC_a_u          | y |
| EdKC_a_v          | y |
| EdKC_a_w          | y |
| EdKC_a_x          | y |
| EdKC_a_y          | y |
| EdKC_a_z          | y |
| EdKC_a_f1         | hooked: Lubuntu start menu? |
| EdKC_a_f2         | hooked: run dialog box |
| EdKC_a_f3         | hooked: unknown; seems to freeze K (?) |
| EdKC_a_f4         | hooked: unknown; seems to freeze K (?) - should work with edited openbox config |
| EdKC_a_f5         | hooked: unknown; seems to freeze K  |
| EdKC_a_f6         | hooked: unknown; seems to freeze K  |
| EdKC_a_f7         | hooked: unknown; seems to freeze K  |
| EdKC_a_f8         | hooked: unknown; seems to freeze K  |
| EdKC_a_f9         | hooked: unknown; seems to freeze K  |
| EdKC_a_f10        | hooked: unknown; seems to freeze K  |
| EdKC_a_f11        | hooked: unknown; seems to freeze K  |
| EdKC_a_f12        | hooked: unknown; seems to freeze K  |
| EdKC_a_BACKTICK   | is ctrl+f (?) |
| EdKC_a_MINUS      | is alt+; (?) |
| EdKC_a_EQUAL      | is alt+num0 |
| EdKC_a_LEFT_SQ    | is ctrl+a (?) |
| EdKC_a_RIGHT_SQ   | is ctrl+c (?) |
| EdKC_a_BACKSLASH  | is ctrl+b (?) |
| EdKC_a_SEMICOLON  | is alt+del (?) |
| EdKC_a_TICK       | is alt+` (backtick) (?) |
| EdKC_a_COMMA      | is alt+\ (?) |
| EdKC_a_DOT        | is alt+' (tick) (?) |
| EdKC_a_SLASH      | is alt+, (?) |
| EdKC_a_home       | is home (IMPOSSIBLE) |
| EdKC_a_end        | is end (IMPOSSIBLE) |
| EdKC_a_left       | y |
| EdKC_a_right      | y |
| EdKC_a_up         | no response (?) |
| EdKC_a_down       | no response (?) |
| EdKC_a_pgup       | no response (?) |
| EdKC_a_pgdn       | no response (?) |
| EdKC_a_ins        | no response (?) |
| EdKC_a_del        | no response (?) |
| EdKC_a_center     | is center (IMPOSSIBLE) |
| EdKC_a_num0       |  |
| EdKC_a_num1       |  |
| EdKC_a_num2       |  |
| EdKC_a_num3       |  |
| EdKC_a_num4       |  |
| EdKC_a_num5       |  |
| EdKC_a_num6       |  |
| EdKC_a_num7       |  |
| EdKC_a_num8       |  |
| EdKC_a_num9       |  |
| EdKC_a_numMinus   |  |
| EdKC_a_numPlus    |  |
| EdKC_a_numStar    |  |
| EdKC_a_numSlash   |  |
| EdKC_a_numEnter   |  |
| EdKC_a_space      |  |
| EdKC_a_bksp       | y |
| EdKC_a_tab        |  |
| EdKC_a_esc        |  |
| EdKC_a_enter      |  |
| EdKC_c_0          | is ascii '0' |
| EdKC_c_1          | is ascii '1' |
| EdKC_c_2          | is ctrl+9 |
| EdKC_c_3          | is esc |
| EdKC_c_4          | is ctrl+f2 |
| EdKC_c_5          | is ctrl+f3 |
| EdKC_c_6          | is ctrl+f4 |
| EdKC_c_7          | is ctrl+f5 |
| EdKC_c_8          | is bkspc |
| EdKC_c_9          | is ascii '9' |
| EdKC_c_a          | y |
| EdKC_c_b          | y |
| EdKC_c_c          | y |
| EdKC_c_d          | y |
| EdKC_c_e          | y |
| EdKC_c_f          | y |
| EdKC_c_g          | y |
| EdKC_c_h          | y |
| EdKC_c_i          | is tab |
| EdKC_c_j          | is enter |
| EdKC_c_k          | y |
| EdKC_c_l          | y |
| EdKC_c_m          | is enter |
| EdKC_c_n          | y |
| EdKC_c_o          | y |
| EdKC_c_p          | y |
| EdKC_c_q          | y |
| EdKC_c_r          | y |
| EdKC_c_s          | y |
| EdKC_c_t          | y |
| EdKC_c_u          | y |
| EdKC_c_v          | y |
| EdKC_c_w          | y |
| EdKC_c_x          | y |
| EdKC_c_y          | y |
| EdKC_c_z          | y |
| EdKC_c_f1         | no response |
| EdKC_c_f2         | no response |
| EdKC_c_f3         | no response (?) |
| EdKC_c_f4         | no response (?) - should work with edited openbox config |
| EdKC_c_f5         | no response |
| EdKC_c_f6         | no response |
| EdKC_c_f7         | hooked: blanked the entire screen |
| EdKC_c_f8         | no response |
| EdKC_c_f9         | no response |
| EdKC_c_f10        | no response |
| EdKC_c_f11        | no response |
| EdKC_c_f12        | no response |
| EdKC_c_BACKTICK   | is ctrl+9 |
| EdKC_c_MINUS      | ascii '-' |
| EdKC_c_EQUAL      | ascii '-' |
| EdKC_c_LEFT_SQ    | is esc |
| EdKC_c_RIGHT_SQ   | is ctrl+f3 |
| EdKC_c_BACKSLASH  | is ctrl+f2 |
| EdKC_c_SEMICOLON  | ascii ';' |
| EdKC_c_TICK       | ascii ''' |
| EdKC_c_COMMA      | ascii ',' |
| EdKC_c_DOT        | ascii '.' |
| EdKC_c_SLASH      | is ctrl+f5 |
| EdKC_c_home       | is home |
| EdKC_c_end        | is end  |
| EdKC_c_left       | y |
| EdKC_c_right      | y |
| EdKC_c_up         | y |
| EdKC_c_down       | y |
| EdKC_c_pgup       | no response |
| EdKC_c_pgdn       | no response |
| EdKC_c_ins        | no response |
| EdKC_c_del        | no response |
| EdKC_c_center     | is center |
| EdKC_c_num0       | is '0' |
| EdKC_c_num1       | is '1' |
| EdKC_c_num2       | is '2' |
| EdKC_c_num3       | is '3' |
| EdKC_c_num4       | is '4' |
| EdKC_c_num5       | is '5' |
| EdKC_c_num6       | is '6' |
| EdKC_c_num7       | is '7' |
| EdKC_c_num8       | is '8' |
| EdKC_c_num9       | is '9' |
| EdKC_c_numMinus   |  |
| EdKC_c_numPlus    |  |
| EdKC_c_numStar    |  |
| EdKC_c_numSlash   |  |
| EdKC_c_numEnter   |  |
| EdKC_c_space      | is space |
| EdKC_c_bksp       | is bksp |
| EdKC_c_tab        |  |
| EdKC_c_esc        |  |
| EdKC_c_enter      |  |
| EdKC_s_f1         | NR |
| EdKC_s_f2         | NR |
| EdKC_s_f3         | y |
| EdKC_s_f4         | y |
| EdKC_s_f5         | NR |
| EdKC_s_f6         | NR |
| EdKC_s_f7         | NR |
| EdKC_s_f8         | NR |
| EdKC_s_f9         | NR |
| EdKC_s_f10        | NR |
| EdKC_s_f11        | NR |
| EdKC_s_f12        | NR |
| EdKC_s_home       | is '7' |
| EdKC_s_end        | is '1' |
| EdKC_s_left       | is '2' |
| EdKC_s_right      | is '4' |
| EdKC_s_up         | is '8' |
| EdKC_s_down       | is '2' |
| EdKC_s_pgup       | is '9' |
| EdKC_s_pgdn       | is '3' |
| EdKC_s_ins        | is '0' |
| EdKC_s_del        | is '.' |
| EdKC_s_center     | is '5' |
| EdKC_s_num0       |  |
| EdKC_s_num1       |  |
| EdKC_s_num2       |  |
| EdKC_s_num3       |  |
| EdKC_s_num4       |  |
| EdKC_s_num5       |  |
| EdKC_s_num6       |  |
| EdKC_s_num7       |  |
| EdKC_s_num8       |  |
| EdKC_s_num9       |  |
| EdKC_s_numMinus   | no response |
| EdKC_s_numPlus    | no response |
| EdKC_s_numStar    | no response |
| EdKC_s_numSlash   | no response |
| EdKC_s_numEnter   | is enter |
| EdKC_s_space      |  |
| EdKC_s_bksp       | is bksp |
| EdKC_s_tab        | no response |
| EdKC_s_esc        | is esc |
| EdKC_s_enter      |  |
| EdKC_cs_bksp      |  |
| EdKC_cs_tab       |  |
| EdKC_cs_center    |  |
| EdKC_cs_enter     |  |
| EdKC_cs_pause     |  |
| EdKC_cs_space     |  |
| EdKC_cs_pgup      |  |
| EdKC_cs_pgdn      |  |
| EdKC_cs_end       |  |
| EdKC_cs_home      |  |
| EdKC_cs_left      |  |
| EdKC_cs_up        |  |
| EdKC_cs_right     |  |
| EdKC_cs_down      |  |
| EdKC_cs_ins       |  |
| EdKC_cs_0         |  |
| EdKC_cs_1         |  |
| EdKC_cs_2         |  |
| EdKC_cs_3         |  |
| EdKC_cs_4         |  |
| EdKC_cs_5         |  |
| EdKC_cs_6         |  |
| EdKC_cs_7         |  |
| EdKC_cs_8         |  |
| EdKC_cs_9         |  |
| EdKC_cs_a         |  |
| EdKC_cs_b         |  |
| EdKC_cs_c         |  |
| EdKC_cs_d         |  |
| EdKC_cs_e         |  |
| EdKC_cs_f         |  |
| EdKC_cs_g         |  |
| EdKC_cs_h         |  |
| EdKC_cs_i         |  |
| EdKC_cs_j         |  |
| EdKC_cs_k         |  |
| EdKC_cs_l         |  |
| EdKC_cs_m         |  |
| EdKC_cs_n         |  |
| EdKC_cs_o         |  |
| EdKC_cs_p         |  |
| EdKC_cs_q         |  |
| EdKC_cs_r         |  |
| EdKC_cs_s         |  |
| EdKC_cs_t         |  |
| EdKC_cs_u         |  |
| EdKC_cs_v         |  |
| EdKC_cs_w         |  |
| EdKC_cs_x         |  |
| EdKC_cs_y         |  |
| EdKC_cs_z         |  |
| EdKC_cs_numStar   |  |
| EdKC_cs_numPlus   |  |
| EdKC_cs_numEnter  |  |
| EdKC_cs_numMinus  |  |
| EdKC_cs_numSlash  |  |
| EdKC_cs_f1        |  |
| EdKC_cs_f2        |  |
| EdKC_cs_f3        |  |
| EdKC_cs_f4        |  |
| EdKC_cs_f5        |  |
| EdKC_cs_f6        |  |
| EdKC_cs_f7        |  |
| EdKC_cs_f8        |  |
| EdKC_cs_f9        |  |
| EdKC_cs_f10       |  |
| EdKC_cs_f11       |  |
| EdKC_cs_f12       |  |
| EdKC_cs_scroll    |  |
| EdKC_cs_SEMICOLON |  |
| EdKC_cs_EQUAL     |  |
| EdKC_cs_COMMA     |  |
| EdKC_cs_MINUS     |  |
| EdKC_cs_DOT       |  |
| EdKC_cs_SLASH     |  |
| EdKC_cs_BACKTICK  |  |
| EdKC_cs_TICK      |  |
| EdKC_cs_LEFT_SQ   |  |
| EdKC_cs_BACKSLASH |  |
| EdKC_cs_RIGHT_SQ  |  |
| EdKC_cs_num0      |  |
| EdKC_cs_num1      |  |
| EdKC_cs_num2      |  |
| EdKC_cs_num3      |  |
| EdKC_cs_num4      |  |
| EdKC_cs_num5      |  |
| EdKC_cs_num6      |  |
| EdKC_cs_num7      |  |
| EdKC_cs_num8      |  |
| EdKC_cs_num9      |  |
| EdKC_pause        |  |
| EdKC_a_pause      |  |
| EdKC_s_pause      |  |
| EdKC_c_numlk      |  |
| EdKC_scroll       |  |
| EdKC_a_scroll     |  |
| EdKC_s_scroll     |  |


# Top 60 editor functions used, mapped to EdKC

20150115_0742 X rate=21/59=36%

| count  | ARG::name        |?| EdKC             |
|--------|------------------|-|------------------|
| 774704 | down             |y| EdKC_down        |
| 582550 | up               |y| EdKC_up          |
| 366102 | graphic          |y| (many)           |
| 352863 | right            |y| EdKC_right       |
| 173013 | left             |y| EdKC_left        |
| 108228 | pword            |y| EdKC_c_right     |
| 106733 | arg              |y| EdKC_center      |
|  70229 | mword            |y| EdKC_c_left      |
|  59268 | mpage            |y| EdKC_pgup        |
|  51719 | setfile          |y| EdKC_f2          |
|  42663 | endline          |y| EdKC_end         |
|  41748 | msearch          |y| EdKC_f4          |
|  38256 | begline          |y| EdKC_home        |
|  36816 | udelete          |y| EdKC_numMINUS    |
|  31729 | ppage            |y| EdKC_pgdn        |
|  29793 | psearch          |y| EdKC_f3          |
|  ????? | selword          |X| EdKC_c_center    |
|  24858 | paste            |y| EdKC_ins         |
|  16908 | copy             |y| EdKC_numPLUS     |
|  16852 | exit             |X| EdKC_a_f4        |
|  14419 | emacscdel        |y| EdKC_bksp        |
|  13473 | undo             |y| EdKC_a_bksp      |
|  13343 | emacsnewl        |y| EdKC_numEnter    |
|   8615 | cancel           |y| EdKC_esc         |
|   6145 | ppara            |y| EdKC_c_down      |
|   5982 | files            |X| EdKC_a_f2        |
|   4727 | endfile          |X| EdKC_c_pgdn      |
|   4610 | execute          |y| EdKC_c_x         |
|   3914 | window           |y| EdKC_f6          |
|   3108 | meta             |y| EdKC_f9          |
|   2602 | begfile          |X| EdKC_c_pgup      |
|   2556 | nextmsg          |y| EdKC_a_n         |
|   2435 | redo             |X| EdKC_c_bksp      |
|   2177 | mfgrep           |y| EdKC_s_f4        |
|   2167 | vrepeat          |y| EdKC_a_v         |
|   1778 | flipcase         |y| EdKC_a_c         |
|   1354 | home             |X| EdKC_c_home      |
|   1187 | replace          |y| EdKC_c_l         |
|   1132 | assign           |X| EdKC_a_EQUAL     |
|    883 | qreplace         |X| EdKC_c_BACKSLASH |
|    858 | fromwinclip      |y| EdKC_c_v         |
|    731 | grep             |X| EdKC_c_f3        |
|    693 | towinclip        |y| EdKC_c_c         |
|    668 | longline         |X| EdKC_a_BACKSLASH |
|    527 | mpara            |y| EdKC_c_up        |
|    519 | tags             |y| EdKC_a_u         |
|    474 | selword          |X| EdKC_c_center    |
|    425 | searchlog        |X| EdKC_a_f3        |
|    406 | del_no_clip      |y| EdKC_del         |
|    323 | justify          |y| EdKC_c_b         |
|    309 | lua              |y| EdKC_a_l         |
|    266 | lastselect       |y| EdKC_a_s         |
|    251 | gotofileline     |y| EdKC_a_g         |
|    246 | lasttext         |y| EdKC_a_d         |
|    221 | mfreplace        |X| EdKC_f11         |
|    159 | aligncol         |y| EdKC_a_a         |
|    141 | backtravlocnlist |y| EdKC_a_left      |
|    137 | traverselocnlist |y| EdKC_a_right     |
|    105 | boxstream        |y| EdKC_a_b         |
|    100 | tell             |y| EdKC_a_t         |
