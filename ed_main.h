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

#pragma once

#define  DEBUG_LOGGING  1
#define  OLDGREP  0

// porting abbreviation tool
#if defined(_WIN32)
#   define WL(ww,ll)  ww
#else
#   define WL(ww,ll)  ll
#endif

#if defined(_WIN32)
   #define  MOUSE_SUPPORT  1
#else
   #define  MOUSE_SUPPORT  0
#endif

#define USE_STAT64 0


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

// std C++ headers
#include <memory>
#include <utility>  // for std::move,std::swap
#include <algorithm>
#include <string>
#if !defined(_WIN32)
   // Linux-specific string fxns: strcasecmp, strncasecmp, etc.
   #include "strings.h"
#endif
#include <vector>

// standard-ish C headers
#include <malloc.h>    // for alloca()
#include <sys/stat.h>  // for struct _stat

#if defined(_WIN32)
#else
   #define _MAX_PATH 513
#endif

// typedef  char                         pathbuf[_MAX_PATH+1];

#include "my_types.h"
#include "ed_mem.h"

// for my_strutils.h:
extern void chkdVsnprintf( PChar buf, size_t bufBytes, PCChar format, va_list val );
#define   use_vsnprintf  chkdVsnprintf

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

#include "ed_features.h"
#include "ed_vars.h"
#include "ed_protos.h"

#include "my_log.h"
