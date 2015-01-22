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

//
//  Logging classes/APIs
//

#pragma once

#define NO_LOG 0
#if NO_LOG
    // #define version does make smaller code, but causes almost 300 of "warning C4002: too many actual parameters for macro 'DBG'"
    // #define DBG()  (1)
    // STIL version results in almost NO code size change since the params are all evaluated even if the function does nothing
    STIL int DBG( char const *kszFormat, ... ) ATTR_FORMAT(1, 2) { return 1; }
    STIL void DBG_init() {}
#else
    extern int DBG( char const *kszFormat, ... ) ATTR_FORMAT(1, 2);
    extern void DBG_init();
#endif
