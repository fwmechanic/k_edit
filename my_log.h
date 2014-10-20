//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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
#else
    extern int DBG( char const *kszFormat, ... ) ATTR_FORMAT(1, 2);
#endif
