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

extern PStringListEl NewStringListEl(                        PCChar str, PCChar pEos=nullptr );
extern void          InsStringListEl( PStringListHead pSlhd, PCChar str, PCChar pEos=nullptr );
extern void          DeleteStringList( PStringListHead pSlhd );

#define  FreeStringListEl( pSLE )  Free0( pSLE )

struct StringList {
   StringListHead d_head;
   StringList() {}
   StringList( PCChar str ) { InsStringListEl( &d_head, str ); }
   void Clear() { DeleteStringList( &d_head ); }
   ~StringList() { Clear(); }
   void AddStr( PCChar str, PCChar pEos=nullptr ) { InsStringListEl( &d_head, str, pEos ); }
   unsigned Count() const { return d_head.Count(); }
   StringListHead &Head() { return d_head; }
   bool IsEmpty() const { return d_head.IsEmpty(); }

   }; STD_TYPEDEFS( StringList )
