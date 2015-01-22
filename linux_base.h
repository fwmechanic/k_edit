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

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

//##############################################################################

STIL void DebugLog( PCChar string ) { /* Win32::OutputDebugString( string ); */ }

extern bool IsFileReadonly( PCChar pFBufName );
extern bool FileOrDirExists( PCChar lpFBufName );

struct WhileHoldingGlobalVariableLock
   {
   WhileHoldingGlobalVariableLock()  ;
   ~WhileHoldingGlobalVariableLock() ;
   };

extern void MainThreadGiveUpGlobalVariableLock()  ;
extern void MainThreadWaitForGlobalVariableLock() ;
