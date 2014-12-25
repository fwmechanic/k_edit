//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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

extern StringListEl *NewStringListEl( boost::string_ref src );
extern void          InsStringListEl( StringListHead &slhd, boost::string_ref src );
extern void          DeleteStringList( StringListHead &slhd );

#define  FreeStringListEl( pSLE )  Free0( pSLE )

struct StringList {
   StringListHead d_head; // _public_ so DLINKC_FIRST_TO_LASTA can be used to efficiently walk the list
   StringList() {}
   StringList( PCChar str ) { InsStringListEl( d_head, str ); }
   void clear() { DeleteStringList( d_head ); }
   ~StringList() { clear(); }
   void push_front( boost::string_ref src ) { InsStringListEl( d_head, src ); }
   unsigned length() const { return d_head.length(); }
   StringListHead &Head() { return d_head; }
   bool empty() const { return d_head.empty(); }
   };
