//
// Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
#include <ncurses.h>
#include "ed_main.h"

void AssertDialog_( PCChar function, int line ) {
   fprintf( stderr, "Assertion failed, %s L %d", function, line );
   abort();
   }

Path::str_t Path::GetCwd() { // quick and dirty AND relies on GLIBC getcwd( nullptr, 0 ) semantics which are NONPORTABLE
   PChar mallocd_cwd = getcwd( nullptr, 0 );
   Path::str_t rv( mallocd_cwd );
   free( mallocd_cwd );
   return rv;
   }

PCChar OsVerStr() { return "Linux"; }

Path::str_t Path::Absolutize( PCChar pszFilename ) {  enum { DEBUG_FXN = 0 };
   if( IsPathSepCh( pszFilename[0] ) ) {
      return pszFilename;
      }
   return GetCwd_ps() + pszFilename;
   }


STIL bool KeepMatch_( const WildCardMatchMode want, const struct stat &sbuf ) {
   const auto fFileOk( ToBOOL(want & ONLY_FILES) );
   const auto fDirOk ( ToBOOL(want & ONLY_DIRS ) );
   const auto fIsDir ( ToBOOL(sbuf.st_mode & S_IFDIR) );
   const auto fIsFile( !fIsDir && (sbuf.st_mode & (S_IFREG | S_IFLNK)) );

   if( fDirOk  && fIsDir  ) return true;
   if( fFileOk && fIsFile ) return true;
   return                          false;
   }

bool DirMatches::KeepMatch() {
   if( stat( d_dirent->d_name, &d_sbuf ) != 0 ) {
      return false;
      }

   const auto rv( KeepMatch_( d_wcMode, d_sbuf ) );

   1 && DBG( "want %c%c, have %X '%s' rv=%d"
           , (d_wcMode & ONLY_DIRS ) ? 'D':'d'
           , (d_wcMode & ONLY_FILES) ? 'F':'f'
           , d_sbuf.st_mode
           , d_dirent->d_name
           , rv
           );

   return rv;
   }

bool DirMatches::FoundNext() {
   d_dirent = readdir( d_dirp );
   if( !d_dirent ) {
      d_ixDest = std::string::npos; // flag no more matches
      return false; // failed!
      }

   return true;
   }

const Path::str_t DirMatches::GetNext() {
   if( d_ixDest == std::string::npos ) // already hit no-more-matches condition?
      return Path::str_t("");

   while( FoundNext() && !KeepMatch() )
      continue;

   if( d_ixDest == std::string::npos ) // already hit no-more-matches condition?
      return Path::str_t("");

   d_buf.replace( d_ixDest, std::string::npos, d_dirent->d_name );
   if( ToBOOL(d_sbuf.st_mode & S_IFDIR) && !Path::IsDotOrDotDot( d_buf.c_str() ) )
      d_buf.append( PATH_SEP_STR );

   0 && DBG( "DirMatches::GetNext: '%s' (%X)", d_buf.c_str(), d_sbuf.st_mode );
   return d_buf;
   }

DirMatches::~DirMatches() {
   if( d_dirp ) {
      closedir( d_dirp );
      }
   }

DirMatches::DirMatches( PCChar pszPrefix, PCChar pszSuffix, WildCardMatchMode wcMode, bool fAbsolutize )
   : d_wcMode( wcMode )
   , d_buf( Path::str_t(pszPrefix ? pszPrefix : "") + Path::str_t(pszSuffix ? pszSuffix : "") )
   { // prep the buffer
   if( fAbsolutize ) {
      d_buf = Path::Absolutize( d_buf.c_str() );
      }

   d_dirp = opendir( d_buf.c_str() );
   if( !d_dirp ) {
      d_ixDest = std::string::npos; // already hit no-more-matches condition?
      }
   else {
      d_dirent = readdir( d_dirp );
      if( !d_dirent ) {
         d_ixDest = std::string::npos; // already hit no-more-matches condition?
         }
      else {
         // hackaround
         const auto pDest( Path::StrToPrevPathSepOrNull( d_buf.c_str(), PCChar(nullptr) ) );
         if( pDest )
            d_ixDest = (pDest - d_buf.c_str()) + 1; // point past pathsep
         else
            d_ixDest = 0;
         }
      }
   }


bool FileOrDirExists( PCChar pszFileName ) {
   struct stat sbuf;
   if( stat( pszFileName, &sbuf ) != 0 ) {
      return false;
      }
   return true; // basically, anything by this name gets in the way of a file write
   }

PCChar OsErrStr( PChar dest, size_t sizeofDest ) {
   return strerror_r( errno, dest, sizeofDest );
   }

//******************************************************************************
extern GLOBAL_VAR CMD g_CmdTable[];

#define  IDX_EQ( idx_val )

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
//TODO
typedef U16 EdKC;
typedef const EdKC XlatForKey[5];
STATIC_CONST XlatForKey normalXlatTbl[ 0xE0 ] = {

//   NORM           ALT               CTRL              SHIFT            CTRL+SHIFT                       VK_xxx
//   -------------  ----------------  ----------------  ---------------- ----------------               ----------------
   { 0,             0,                0,                0,               0,                 }, //    0
   { 0,             0,                0,                0,               0,                 }, // 0x01  VK_LBUTTON              Left mouse button
   { 0,             0,                0,                0,               0,                 }, // 0x02  VK_RBUTTON              Right mouse button
   { 0,             0,                0,                0,               0,                 }, // 0x03  VK_CANCEL               Control-break processing
   { 0,             0,                0,                0,               0,                 }, // 0x04  VK_MBUTTON              Middle mouse button (three-button mouse)
   { 0,             0,                0,                0,               0,                 }, // 0x05  VK_XBUTTON1             Windows 2000/XP: X1 mouse button
   { 0,             0,                0,                0,               0,                 }, // 0x06  VK_XBUTTON2             Windows 2000/XP: X2 mouse button
   { 0,             0,                0,                0,               0,                 }, //    7  - (07)                  Undefined
   { EdKC_bksp,     EdKC_a_bksp,      EdKC_c_bksp,      EdKC_s_bksp,     EdKC_cs_bksp,      }, // 0x08  VK_BACK                 BACKSPACE key
   { EdKC_tab,      EdKC_a_tab,       EdKC_c_tab,       EdKC_s_tab,      EdKC_cs_tab,       }, // 0x09  VK_TAB                  TAB key
   { 0,             0,                0,                0,               0,                 }, //    A  - (0A-0B)               Reserved
   { 0,             0,                0,                0,               0,                 }, //    B  - (0A-0B)               Reserved
   { EdKC_center,   EdKC_a_center,    EdKC_c_center,    EdKC_s_center,   EdKC_cs_center,    }, // 0x0C  VK_CLEAR                CLEAR key
   { EdKC_enter,    EdKC_a_enter,     EdKC_c_enter,     EdKC_s_enter,    EdKC_cs_enter,     }, // 0x0D  VK_RETURN               ENTER key
   { 0,             0,                0,                0,               0,                 }, //    E  - (0E-0F)               Undefined
   { 0,             0,                0,                0,               0,                 }, //    F  - (0E-0F)               Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x10  VK_SHIFT                SHIFT key
   { 0,             0,                0,                0,               0,                 }, // 0x11  VK_CONTROL              CTRL key
   { 0,             0,                0,                0,               0,                 }, // 0x12  VK_MENU                 ALT key
   { EdKC_pause,    EdKC_a_pause,     EdKC_c_numlk,     EdKC_s_pause,    EdKC_cs_pause,     }, // 0x13  VK_PAUSE                PAUSE key
   { 0,             0,                0,                0,               0,                 }, // 0x14  VK_CAPITAL              CAPS LOCK key
   { 0,             0,                0,                0,               0,                 }, // 0x15  VK_HANGUL               IME Hangul mode
   { 0,             0,                0,                0,               0,                 }, //    6  - (16)                  Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x17  VK_JUNJA                IME Junja mode
   { 0,             0,                0,                0,               0,                 }, // 0x18  VK_FINAL                IME final mode
   { 0,             0,                0,                0,               0,                 }, // 0x19  VK_KANJI                IME Kanji mode
   { 0,             0,                0,                0,               0,                 }, //    A  - (1A)                  Undefined
   { EdKC_esc,      0,                EdKC_c_esc,       EdKC_s_esc,      0,                 }, // 0x1B  VK_ESCAPE               ESC key
   { 0,             0,                0,                0,               0,                 }, // 0x1C  VK_CONVERT              IME convert
   { 0,             0,                0,                0,               0,                 }, // 0x1D  VK_NONCONVERT           IME nonconvert
   { 0,             0,                0,                0,               0,                 }, // 0x1E  VK_ACCEPT               IME accept
   { 0,             0,                0,                0,               0,                 }, // 0x1F  VK_MODECHANGE           IME mode change request
   { 0,             EdKC_a_space,     EdKC_c_space,     0,               EdKC_cs_space,     }, // 0x20  VK_SPACE                SPACEBAR
   { EdKC_pgup,     EdKC_a_pgup,      EdKC_c_pgup,      EdKC_s_pgup,     EdKC_cs_pgup,      }, // 0x21  VK_PRIOR                PAGE UP key
   { EdKC_pgdn,     EdKC_a_pgdn,      EdKC_c_pgdn,      EdKC_s_pgdn,     EdKC_cs_pgdn,      }, // 0x22  VK_NEXT                 PAGE DOWN key
   { EdKC_end,      EdKC_a_end,       EdKC_c_end,       EdKC_s_end,      EdKC_cs_end,       }, // 0x23  VK_END                  END key
   { EdKC_home,     EdKC_a_home,      EdKC_c_home,      EdKC_s_home,     EdKC_cs_home,      }, // 0x24  VK_HOME                 HOME key
   { EdKC_left,     EdKC_a_left,      EdKC_c_left,      EdKC_s_left,     EdKC_cs_left,      }, // 0x25  VK_LEFT                 LEFT ARROW key
   { EdKC_up,       EdKC_a_up,        EdKC_c_up,        EdKC_s_up,       EdKC_cs_up,        }, // 0x26  VK_UP                   UP ARROW key
   { EdKC_right,    EdKC_a_right,     EdKC_c_right,     EdKC_s_right,    EdKC_cs_right,     }, // 0x27  VK_RIGHT                RIGHT ARROW key
   { EdKC_down,     EdKC_a_down,      EdKC_c_down,      EdKC_s_down,     EdKC_cs_down,      }, // 0x28  VK_DOWN                 DOWN ARROW key
   { 0,             0,                0,                0,               0,                 }, // 0x29  VK_SELECT               SELECT key
   { 0,             0,                0,                0,               0,                 }, // 0x2A  VK_PRINT                PRINT key
   { 0,             0,                0,                0,               0,                 }, // 0x2B  VK_EXECUTE              EXECUTE key
   { 0,             0,                0,                0,               0,                 }, // 0x2C  VK_SNAPSHOT             PRINT SCREEN key
   { EdKC_ins,      EdKC_a_ins,       EdKC_c_ins,       EdKC_s_ins,      EdKC_cs_ins,       }, // 0x2D  VK_INSERT               INS key
   { EdKC_del,      EdKC_a_del,       EdKC_c_del,       EdKC_s_del,      0,                 }, // 0x2E  VK_DELETE               DEL key
   { 0,             0,                0,                0,               0,                 }, // 0x2F  VK_HELP                 HELP key
   { 0,             EdKC_a_0,         EdKC_c_0,         0,               EdKC_cs_0,         }, // 0x30  VK_0                    0 key
   { 0,             EdKC_a_1,         EdKC_c_1,         0,               EdKC_cs_1,         }, // 0x31  VK_1                    1 key
   { 0,             EdKC_a_2,         EdKC_c_2,         0,               EdKC_cs_2,         }, // 0x32  VK_2                    2 key
   { 0,             EdKC_a_3,         EdKC_c_3,         0,               EdKC_cs_3,         }, // 0x33  VK_3                    3 key
   { 0,             EdKC_a_4,         EdKC_c_4,         0,               EdKC_cs_4,         }, // 0x34  VK_4                    4 key
   { 0,             EdKC_a_5,         EdKC_c_5,         0,               EdKC_cs_5,         }, // 0x35  VK_5                    5 key
   { 0,             EdKC_a_6,         EdKC_c_6,         0,               EdKC_cs_6,         }, // 0x36  VK_6                    6 key
   { 0,             EdKC_a_7,         EdKC_c_7,         0,               EdKC_cs_7,         }, // 0x37  VK_7                    7 key
   { 0,             EdKC_a_8,         EdKC_c_8,         0,               EdKC_cs_8,         }, // 0x38  VK_8                    8 key
   { 0,             EdKC_a_9,         EdKC_c_9,         0,               EdKC_cs_9,         }, // 0x39  VK_9                    9 key
   { 0,             0,                0,                0,               0,                 }, // 0x3A          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3B          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3C          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3D          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3E          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3F          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x40          Undefined
   { 0,             EdKC_a_a,         EdKC_c_a,         0,               EdKC_cs_a,         }, // 0x41  VK_A
   { 0,             EdKC_a_b,         EdKC_c_b,         0,               EdKC_cs_b,         }, // 0x42  VK_B
   { 0,             EdKC_a_c,         EdKC_c_c,         0,               EdKC_cs_c,         }, // 0x43  VK_C
   { 0,             EdKC_a_d,         EdKC_c_d,         0,               EdKC_cs_d,         }, // 0x44  VK_D
   { 0,             EdKC_a_e,         EdKC_c_e,         0,               EdKC_cs_e,         }, // 0x45  VK_E
   { 0,             EdKC_a_f,         EdKC_c_f,         0,               EdKC_cs_f,         }, // 0x46  VK_F
   { 0,             EdKC_a_g,         EdKC_c_g,         0,               EdKC_cs_g,         }, // 0x47  VK_G
   { 0,             EdKC_a_h,         EdKC_c_h,         0,               EdKC_cs_h,         }, // 0x48  VK_H
   { 0,             EdKC_a_i,         EdKC_c_i,         0,               EdKC_cs_i,         }, // 0x49  VK_I
   { 0,             EdKC_a_j,         EdKC_c_j,         0,               EdKC_cs_j,         }, // 0x4A  VK_J
   { 0,             EdKC_a_k,         EdKC_c_k,         0,               EdKC_cs_k,         }, // 0x4B  VK_K
   { 0,             EdKC_a_l,         EdKC_c_l,         0,               EdKC_cs_l,         }, // 0x4C  VK_L
   { 0,             EdKC_a_m,         EdKC_c_m,         0,               EdKC_cs_m,         }, // 0x4D  VK_M
   { 0,             EdKC_a_n,         EdKC_c_n,         0,               EdKC_cs_n,         }, // 0x4E  VK_N
   { 0,             EdKC_a_o,         EdKC_c_o,         0,               EdKC_cs_o,         }, // 0x4F  VK_O
   { 0,             EdKC_a_p,         EdKC_c_p,         0,               EdKC_cs_p,         }, // 0x50  VK_P
   { 0,             EdKC_a_q,         EdKC_c_q,         0,               EdKC_cs_q,         }, // 0x51  VK_Q
   { 0,             EdKC_a_r,         EdKC_c_r,         0,               EdKC_cs_r,         }, // 0x52  VK_R
   { 0,             EdKC_a_s,         EdKC_c_s,         0,               EdKC_cs_s,         }, // 0x53  VK_S
   { 0,             EdKC_a_t,         EdKC_c_t,         0,               EdKC_cs_t,         }, // 0x54  VK_T
   { 0,             EdKC_a_u,         EdKC_c_u,         0,               EdKC_cs_u,         }, // 0x55  VK_U
   { 0,             EdKC_a_v,         EdKC_c_v,         0,               EdKC_cs_v,         }, // 0x56  VK_V
   { 0,             EdKC_a_w,         EdKC_c_w,         0,               EdKC_cs_w,         }, // 0x57  VK_W
   { 0,             EdKC_a_x,         EdKC_c_x,         0,               EdKC_cs_x,         }, // 0x58  VK_X
   { 0,             EdKC_a_y,         EdKC_c_y,         0,               EdKC_cs_y,         }, // 0x59  VK_Y
   { 0,             EdKC_a_z,         EdKC_c_z,         0,               EdKC_cs_z,         }, // 0x5A  VK_Z
   { 0,             0,                0,                0,               0,                 }, // 0x5B  VK_LWIN                 Left Windows key (Microsoft Natural keyboard)
   { 0,             0,                0,                0,               0,                 }, // 0x5C  VK_RWIN                 Right Windows key (Natural keyboard)
   { 0,             0,                0,                0,               0,                 }, // 0x5D  VK_APPS                 Applications key (Natural keyboard)
   { 0,             0,                0,                0,               0,                 }, // 0x5E         Reserved
   { 0,             0,                0,                0,               0,                 }, // 0x5F  VK_SLEEP                Computer Sleep key
   { 0,             0,                0,                0,               0,                 }, // 0x60  VK_NUMPAD0              Numeric keypad 0 key
   { 0,             0,                0,                0,               0,                 }, // 0x61  VK_NUMPAD1              Numeric keypad 1 key
   { 0,             0,                0,                0,               0,                 }, // 0x62  VK_NUMPAD2              Numeric keypad 2 key
   { 0,             0,                0,                0,               0,                 }, // 0x63  VK_NUMPAD3              Numeric keypad 3 key
   { 0,             0,                0,                0,               0,                 }, // 0x64  VK_NUMPAD4              Numeric keypad 4 key
   { 0,             0,                0,                0,               0,                 }, // 0x65  VK_NUMPAD5              Numeric keypad 5 key
   { 0,             0,                0,                0,               0,                 }, // 0x66  VK_NUMPAD6              Numeric keypad 6 key
   { 0,             0,                0,                0,               0,                 }, // 0x67  VK_NUMPAD7              Numeric keypad 7 key
   { 0,             0,                0,                0,               0,                 }, // 0x68  VK_NUMPAD8              Numeric keypad 8 key
   { 0,             0,                0,                0,               0,                 }, // 0x69  VK_NUMPAD9              Numeric keypad 9 key
   { EdKC_numStar , EdKC_a_numStar ,  EdKC_c_numStar ,  EdKC_s_numStar , EdKC_cs_numStar ,  }, // 0x6A  VK_MULTIPLY             Multiply key
   { EdKC_numPlus , EdKC_a_numPlus ,  EdKC_c_numPlus ,  EdKC_s_numPlus , EdKC_cs_numPlus ,  }, // 0x6B  VK_ADD                  Add key
   { EdKC_numEnter, EdKC_a_numEnter,  EdKC_c_numEnter,  EdKC_s_numEnter, EdKC_cs_numEnter,  }, // 0x6C  VK_SEPARATOR            Separator key
   { EdKC_numMinus, EdKC_a_numMinus,  EdKC_c_numMinus,  EdKC_s_numMinus, EdKC_cs_numMinus,  }, // 0x6D  VK_SUBTRACT             Subtract key
   { 0,             0,                0,                0,               0,                 }, // 0x6E  VK_DECIMAL              Decimal key
   { EdKC_numSlash, EdKC_a_numSlash,  EdKC_c_numSlash,  EdKC_s_numSlash, EdKC_cs_numSlash,  }, // 0x6F  VK_DIVIDE               Divide key
   { EdKC_f1,       EdKC_a_f1,        EdKC_c_f1,        EdKC_s_f1,       EdKC_cs_f1,        }, // 0x70  VK_F1                   F1 key
   { EdKC_f2,       EdKC_a_f2,        EdKC_c_f2,        EdKC_s_f2,       EdKC_cs_f2,        }, // 0x71  VK_F2                   F2 key
   { EdKC_f3,       EdKC_a_f3,        EdKC_c_f3,        EdKC_s_f3,       EdKC_cs_f3,        }, // 0x72  VK_F3                   F3 key
   { EdKC_f4,       EdKC_a_f4,        EdKC_c_f4,        EdKC_s_f4,       EdKC_cs_f4,        }, // 0x73  VK_F4                   F4 key
   { EdKC_f5,       EdKC_a_f5,        EdKC_c_f5,        EdKC_s_f5,       EdKC_cs_f5,        }, // 0x74  VK_F5                   F5 key
   { EdKC_f6,       EdKC_a_f6,        EdKC_c_f6,        EdKC_s_f6,       EdKC_cs_f6,        }, // 0x75  VK_F6                   F6 key
   { EdKC_f7,       EdKC_a_f7,        EdKC_c_f7,        EdKC_s_f7,       EdKC_cs_f7,        }, // 0x76  VK_F7                   F7 key
   { EdKC_f8,       EdKC_a_f8,        EdKC_c_f8,        EdKC_s_f8,       EdKC_cs_f8,        }, // 0x77  VK_F8                   F8 key
   { EdKC_f9,       EdKC_a_f9,        EdKC_c_f9,        EdKC_s_f9,       EdKC_cs_f9,        }, // 0x78  VK_F9                   F9 key
   { EdKC_f10,      EdKC_a_f10,       EdKC_c_f10,       EdKC_s_f10,      EdKC_cs_f10,       }, // 0x79  VK_F10                  F10 key
   { EdKC_f11,      EdKC_a_f11,       EdKC_c_f11,       EdKC_s_f11,      EdKC_cs_f11,       }, // 0x7A  VK_F11                  F11 key
   { EdKC_f12,      EdKC_a_f12,       EdKC_c_f12,       EdKC_s_f12,      EdKC_cs_f12,       }, // 0x7B  VK_F12                  F12 key
   { 0,             0,                0,                0,               0,                 }, // 0x7C  VK_F13                  F13 key
   { 0,             0,                0,                0,               0,                 }, // 0x7D  VK_F14                  F14 key
   { 0,             0,                0,                0,               0,                 }, // 0x7E  VK_F15                  F15 key
   { 0,             0,                0,                0,               0,                 }, // 0x7F  VK_F16                  F16 key
   { 0,             0,                0,                0,               0,                 }, // 0x80  VK_F17                  F17 key
   { 0,             0,                0,                0,               0,                 }, // 0x81  VK_F18                  F18 key
   { 0,             0,                0,                0,               0,                 }, // 0x82  VK_F19                  F19 key
   { 0,             0,                0,                0,               0,                 }, // 0x83  VK_F20                  F20 key
   { 0,             0,                0,                0,               0,                 }, // 0x84  VK_F21                  F21 key
   { 0,             0,                0,                0,               0,                 }, // 0x85  VK_F22                  F22 key
   { 0,             0,                0,                0,               0,                 }, // 0x86  VK_F23                  F23 key
   { 0,             0,                0,                0,               0,                 }, // 0x87  VK_F24                  F24 key
   { 0,             0,                0,                0,               0,                 }, // 0x88          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x89          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8A          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8B          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8C          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8D          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8E          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8F          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x90  VK_NUMLOCK              NUM LOCK key
   { EdKC_scroll,   EdKC_a_scroll,    0,                EdKC_s_scroll,   EdKC_cs_scroll,    }, // 0x91  VK_SCROLL               SCROLL LOCK key
   { 0,             0,                0,                0,               0,                 }, // 0x92   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x93   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x94   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x95   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x96   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x97  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x98  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x99  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9A  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9B  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9C  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9D  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9E  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9F  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0xA0  VK_LSHIFT               Left SHIFT key
   { 0,             0,                0,                0,               0,                 }, // 0xA1  VK_RSHIFT               Right SHIFT key
   { 0,             0,                0,                0,               0,                 }, // 0xA2  VK_LCONTROL             Left CONTROL key
   { 0,             0,                0,                0,               0,                 }, // 0xA3  VK_RCONTROL             Right CONTROL key
   { 0,             0,                0,                0,               0,                 }, // 0xA4  VK_LMENU                Left MENU key
   { 0,             0,                0,                0,               0,                 }, // 0xA5  VK_RMENU                Right MENU key
   { 0,             0,                0,                0,               0,                 }, // 0xA6  VK_BROWSER_BACK         Windows 2000/XP: Browser Back key
   { 0,             0,                0,                0,               0,                 }, // 0xA7  VK_BROWSER_FORWARD      Windows 2000/XP: Browser Forward key
   { 0,             0,                0,                0,               0,                 }, // 0xA8  VK_BROWSER_REFRESH      Windows 2000/XP: Browser Refresh key
   { 0,             0,                0,                0,               0,                 }, // 0xA9  VK_BROWSER_STOP         Windows 2000/XP: Browser Stop key
   { 0,             0,                0,                0,               0,                 }, // 0xAA  VK_BROWSER_SEARCH       Windows 2000/XP: Browser Search key
   { 0,             0,                0,                0,               0,                 }, // 0xAB  VK_BROWSER_FAVORITES    Windows 2000/XP: Browser Favorites key
   { 0,             0,                0,                0,               0,                 }, // 0xAC  VK_BROWSER_HOME         Windows 2000/XP: Browser Start and Home key
   { 0,             0,                0,                0,               0,                 }, // 0xAD  VK_VOLUME_MUTE          Windows 2000/XP: Volume Mute key
   { 0,             0,                0,                0,               0,                 }, // 0xAE  VK_VOLUME_DOWN          Windows 2000/XP: Volume Down key
   { 0,             0,                0,                0,               0,                 }, // 0xAF  VK_VOLUME_UP            Windows 2000/XP: Volume Up key
   { 0,             0,                0,                0,               0,                 }, // 0xB0  VK_MEDIA_NEXT_TRACK     Windows 2000/XP: Next Track key
   { 0,             0,                0,                0,               0,                 }, // 0xB1  VK_MEDIA_PREV_TRACK     Windows 2000/XP: Previous Track key
   { 0,             0,                0,                0,               0,                 }, // 0xB2  VK_MEDIA_STOP           Windows 2000/XP: Stop Media key
   { 0,             0,                0,                0,               0,                 }, // 0xB3  VK_MEDIA_PLAY_PAUSE     Windows 2000/XP: Play/Pause Media key
   { 0,             0,                0,                0,               0,                 }, // 0xB4  VK_LAUNCH_MAIL          Windows 2000/XP: Start Mail key
   { 0,             0,                0,                0,               0,                 }, // 0xB5  VK_LAUNCH_MEDIA_SELECT  Windows 2000/XP: Select Media key
   { 0,             0,                0,                0,               0,                 }, // 0xB6  VK_LAUNCH_APP1          Windows 2000/XP: Start Application 1 key
   { 0,             0,                0,                0,               0,                 }, // 0xB7  VK_LAUNCH_APP2          Windows 2000/XP: Start Application 2 key
   { 0,             0,                0,                0,               0,                 }, // 0xB8         Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xB9         Reserved
   { 0,             EdKC_a_SEMICOLON, EdKC_c_SEMICOLON, 0,               EdKC_cs_SEMICOLON, }, // 0xBA  VK_OEM_1                Windows 2000/XP: For the US standard keyboard, the ';:' key
   { 0,             EdKC_a_EQUAL,     EdKC_c_EQUAL,     0,               EdKC_cs_EQUAL,     }, // 0xBB  VK_OEM_PLUS             Windows 2000/XP: For any country/region, the '+' key
   { 0,             EdKC_a_COMMA,     EdKC_c_COMMA,     0,               EdKC_cs_COMMA,     }, // 0xBC  VK_OEM_COMMA            Windows 2000/XP: For any country/region, the ',' key
   { 0,             EdKC_a_MINUS,     EdKC_c_MINUS,     0,               EdKC_cs_MINUS,     }, // 0xBD  VK_OEM_MINUS            Windows 2000/XP: For any country/region, the '-' key
   { 0,             EdKC_a_DOT,       EdKC_c_DOT,       0,               EdKC_cs_DOT,       }, // 0xBE  VK_OEM_PERIOD           Windows 2000/XP: For any country/region, the '.' key
   { 0,             EdKC_a_SLASH,     EdKC_c_SLASH,     0,               EdKC_cs_SLASH,     }, // 0xBF  VK_OEM_2                Windows 2000/XP: For the US standard keyboard, the '/?' key
   { 0,             EdKC_a_BACKTICK,  EdKC_c_BACKTICK,  0,               EdKC_cs_BACKTICK,  }, // 0xC0  VK_OEM_3                Windows 2000/XP: For the US standard keyboard, the '`~' key
   { 0,             EdKC_a_TICK,      EdKC_c_TICK,      0,               EdKC_cs_TICK,      }, // 0xC1          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC2          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC3          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC4          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC5          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC6          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC7          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC8          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC9          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCA          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCB          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCC          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCD          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCE          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCF          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD0          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD1          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD2          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD3          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD4          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD5          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD6          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD7          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD8          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0xD9          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0xDA          Unassigned
   { 0,             EdKC_a_LEFT_SQ,   EdKC_c_LEFT_SQ,   0,               EdKC_cs_LEFT_SQ,   }, // 0xDB VK_OEM_4                 Windows 2000/XP: For the US standard keyboard, the '[{' key
   { 0,             EdKC_a_BACKSLASH, EdKC_c_BACKSLASH, 0,               EdKC_cs_BACKSLASH, }, // 0xDC VK_OEM_5                 Windows 2000/XP: For the US standard keyboard, the '\|' key
   { 0,             EdKC_a_RIGHT_SQ,  EdKC_c_RIGHT_SQ,  0,               EdKC_cs_RIGHT_SQ,  }, // 0xDD VK_OEM_6                 Windows 2000/XP: For the US standard keyboard, the ']}' key
   { 0,             EdKC_a_TICK,      EdKC_c_TICK,      0,               EdKC_cs_TICK,      }, // 0xDE VK_OEM_7                 Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
   { 0,             EdKC_a_EQUAL,     EdKC_c_EQUAL,     0,               EdKC_cs_EQUAL,     }, // 0xDF VK_OEM_8                 Used for miscellaneous characters; it can vary by keyboard.
                                                                                               // 0xE0          Reserved
                                                                                               // 0xE1          OEM specific
                                                                                               // 0xE2 VK_OEM_102               Windows 2000/XP: Either the angle bracket key or the backslash key on the RT 102-key keyboard
                                                                                               // 0xE3          OEM specific
                                                                                               // 0xE4          OEM specific
                                                                                               // 0xE5 VK_PROCESSKEY            Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCESS key
                                                                                               // 0xE6          OEM specific
                                                                                               // 0xE7 VK_PACKET                Windows 2000/XP: Used to pass Unicode characters ... (see below)
                                                                                               // 0xE8          Unassigned
                                                                                               // 0xE9          OEM specific
                                                                                               // 0xEA          OEM specific
                                                                                               // 0xEB          OEM specific
                                                                                               // 0xEC          OEM specific
                                                                                               // 0xED          OEM specific
                                                                                               // 0xEE          OEM specific
                                                                                               // 0xEF          OEM specific
                                                                                               // 0xF0          OEM specific
                                                                                               // 0xF1          OEM specific
                                                                                               // 0xF2          OEM specific
                                                                                               // 0xF3          OEM specific
                                                                                               // 0xF4          OEM specific
                                                                                               // 0xF5          OEM specific
                                                                                               // 0xF6 VK_ATTN                  Attn key
                                                                                               // 0xF7 VK_CRSEL                 CrSel key
                                                                                               // 0xF8 VK_EXSEL                 ExSel key
                                                                                               // 0xF9 VK_EREOF                 Erase EOF key
                                                                                               // 0xFA VK_PLAY                  Play key
                                                                                               // 0xFB VK_ZOOM                  Zoom key
                                                                                               // 0xFC VK_NONAME                Reserved
                                                                                               // 0xFD VK_PA1                   PA1 key
                                                                                               // 0xFE VK_OEM_CLEAR             Clear key

   };

bool FlushKeyQueueAnythingFlushed(){ return flushinp(); }

void StrFromEdkc( PChar pKeyStringBuf, size_t pKeyStringBufBytes, int edKC ){}

bool ConIO::Confirm( PCChar pszPrompt, ... ){ return false;}

int ConIO::DbgPopf( PCChar fmt, ... ){ return 0;}

void WaitForKey(){}

int KeyStr_full( PPChar ppDestBuf, size_t *bufBytesLeft, int keyNum_word ){return 0;}

GLOBAL_CONST int g_MaxKeyNameLen = 0;

CmdData CmdDataFromNextKey_Keystr( PChar pKeyStringBuffer, size_t pKeyStringBufferBytes ){}


