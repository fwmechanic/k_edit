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

#include "my_types.h"
#include "dlink.h"
#include <cstring>

struct StringListEl {
   DLinkEntry<StringListEl> dlink;
   char string[0];
   };

typedef  DLinkHead<StringListEl> StringListHead;

extern StringListEl *NewStringListEl( stref src );
extern void          InsStringListEl( StringListHead &slhd, stref src );
extern void          DeleteStringList( StringListHead &slhd );

#define  FreeStringListEl( pSLE )  Free0( pSLE )

struct StringList {
   StringListHead d_head; // _public_ so DLINKC_FIRST_TO_LASTA can be used to efficiently walk the list
   StringList() {}
   StringList( PCChar str ) { InsStringListEl( d_head, str ); }
   void clear() { DeleteStringList( d_head ); }
   ~StringList() { clear(); }
   void push_back( stref src ) { InsStringListEl( d_head, src ); }
   unsigned length() const { return d_head.length(); }
   StringListEl *remove_first() { StringListEl *rv; DLINK_REMOVE_FIRST( d_head, rv, dlink ); return rv; }
   bool empty() const { return d_head.empty(); }
   };
