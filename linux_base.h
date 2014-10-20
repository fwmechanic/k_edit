//
// Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

//##############################################################################

STIL void DebugLog( PCChar string ) { /* Win32::OutputDebugString( string ); */ }

extern bool IsFileReadonly( PCChar pFBufName );
extern bool FileOrDirExists( PCChar lpFBufName );
