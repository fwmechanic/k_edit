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

#pragma once

#if !defined(WL)
// porting abbreviation tool
#if defined(_WIN32)
#   define WL(ww,ll)  ww
#else
#   define WL(ww,ll)  ll
#endif
#endif

#if defined(_WIN32)
   // in order to use _stat64
   // http://stackoverflow.com/questions/12539488/determine-64-bit-file-size-in-c-on-mingw-32-bit
   // MUST be defined before any MINGW .h file is #included
   #define  __MSVCRT_VERSION__ 0x0601
#endif

// std C/C++ headers
#include <cstddef>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <ctime>
#include <climits>
// standard-ish C headers
#include <malloc.h>    // for alloca()
#include <sys/stat.h>  // for struct _stat
#if !defined(_WIN32)
   // Linux-specific string fxns: strcasecmp, strncasecmp, etc.
   #include "strings.h"
#endif

// std C++ headers
#include <memory>
#include <utility>  // for std::move,std::swap
#include <algorithm>
#include <string>
#include <vector>

//*****************                                 *****************
//*****************  begin project-header includes  *****************
//*****************                                 *****************

// "feature-flags" for project-header includes
#define  DEBUG_LOGGING  1
#define  OLDGREP  0
#define  DBGHILITE  0
#define  MOUSE_SUPPORT  WL(1,0)
#define  VARIABLE_WINBORDER  WL(1,0)
#define  USE_STAT64 0

#if defined(_WIN32)
#else
   #define _MAX_PATH 513
#endif

#include "my_types.h"
#include "ed_mem.h"
#include "my_strutils.h"
#include "filename.h"
#include "dlink.h"
#include "stringlist.h"
#include "krbtree.h"
#include "ed_core.h"
#include "ed_os_generic.h"

//---------------------------------
#define CMDTBL_H_CMD_TBL_PTR_MACROS
#include "cmdtbl.h"
#undef  CMDTBL_H_CMD_TBL_PTR_MACROS
//---------------------------------

#include "ed_vars.h"
#include "ed_edkc.h"
#include "ed_protos.h"
#include "my_log.h"
