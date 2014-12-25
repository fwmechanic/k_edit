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
   };  STD_TYPEDEFS( StringListEl )

typedef  DLinkHead<StringListEl> StringListHead;  STD_TYPEDEFS( StringListHead )

extern void          InsStringListEl( PStringListHead pSlhd, boost::string_ref src );
extern void          DeleteStringList( PStringListHead pSlhd );

#define  FreeStringListEl( pSLE )  Free0( pSLE )

struct StringList {
   StringListHead d_head; // _public_ so DLINKC_FIRST_TO_LASTA can be used to efficiently walk the list
   StringList() {}
   StringList( PCChar str ) { InsStringListEl( &d_head, str ); }
   void Clear() { DeleteStringList( &d_head ); }
   ~StringList() { Clear(); }
   void AddStr( boost::string_ref src ) { InsStringListEl( &d_head, src ); }
   unsigned Count() const { return d_head.Count(); }
   StringListHead &Head() { return d_head; }
   bool IsEmpty() const { return d_head.IsEmpty(); }

   }; STD_TYPEDEFS( StringList )
