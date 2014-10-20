//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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


enum { SFMG_OK=0, SFMG_NO_EXISTING=1, SFMG_CANT_MV_ORIG= 2, SFMG_IDX_WR_ERR=3, SFMG_CANT_MK_BAKDIR };

extern int SaveFileMultiGenerationBackup( PCChar pszFileName );
