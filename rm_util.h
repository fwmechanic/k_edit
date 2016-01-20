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


#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cctype>
#include <climits>
#if defined(_WIN32)
#include <io.h>
#include <direct.h>
#endif
#include <malloc.h> // for alloca()
#include <sys/stat.h>  // for struct _stat

#include "my_types.h"

enum { SFMG_OK, SFMG_NO_SRCFILE, SFMG_CANT_MV_ORIG, SFMG_CANT_MK_BAKDIR };
extern int SaveFileMultiGenerationBackup( PCChar pszFileName );
