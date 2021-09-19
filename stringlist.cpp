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

StringListEl *StringList::NewStringListEl( stref src ) {
   const auto len( src.length() );
   StringListEl *rv;
   AllocBytesNZ( rv, sizeof( *rv ) + len + 1 );
   rv->dlink.clear();
   rv->len = len;
   memcpy( rv->string, src.data(), len );
   rv->string[len] = '\0';
   return rv;
   }
