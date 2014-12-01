//
//  Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

#ifndef ATTR_FORMAT
#ifdef __GNUC__
#define ATTR_FORMAT(xx,yy) __attribute__ ((format (printf, xx, yy)))
#else
#define ATTR_FORMAT(xx,yy)
#endif
#endif
