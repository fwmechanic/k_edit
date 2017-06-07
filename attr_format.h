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

// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#define GCC_VERSION (  __GNUC__           * 10000 \
                     + __GNUC_MINOR__     * 100 \
                     + __GNUC_PATCHLEVEL__          )
// Test for GCC > 7.1.0 #if GCC_VERSION > 70100

#ifndef ATTR_FORMAT
#ifdef __GNUC__
#define ATTR_FORMAT(xx,yy) __attribute__ ((format (gnu_printf, xx, yy)))
#else
#define ATTR_FORMAT(xx,yy)
#endif
#endif

#if defined(__GNUC__) && (GCC_VERSION > 70000)
#define ATTR_FALLTHRU __attribute__ ((fallthrough))
#else
#define ATTR_FALLTHRU
#endif
