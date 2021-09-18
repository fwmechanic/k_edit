//
// Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include "ed_main.h"
#include "stringlist.h"

StringListEl *NewStringListEl( stref src ) {
   const auto sbytes( src.length() + 1 );
   StringListEl *rv;
   AllocBytesNZ( rv, sizeof( *rv ) + sbytes );
   rv->dlink.clear();
   memcpy( rv->string, src.data(), sbytes-1 );
   rv->string[sbytes-1] = '\0';
   return rv;
   }
