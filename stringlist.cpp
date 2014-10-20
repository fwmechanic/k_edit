//
//  Copyright 1991 - 2012 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"
#include "stringlist.h"

PStringListEl NewStringListEl( PCChar str, PCChar pEos ) { // pEos points past last valid char in str
   if( !pEos ) pEos = Eos( str );                          // if str is a complete asciz, *pEos == 0
   const auto sbytes( pEos - str + 1 );
   PStringListEl rv;
   AllocBytesNZ( rv, sizeof( *rv ) + sbytes, __func__ );
   rv->dlink.Clear();
   memcpy( rv->string, str, sbytes-1 );
   rv->string[sbytes-1] = '\0';
   return rv;
   }

void InsStringListEl( PStringListHead pSlhd, PCChar str, PCChar pEos ) {
   DBG("%s: %s", __func__, str );
   auto pIns( NewStringListEl( str, pEos ) );
   DLINK_INSERT_LAST( *pSlhd, pIns, dlink );
   }

void DeleteStringList( PStringListHead pSlhd ) {
   while( auto pEl = pSlhd->First() ) {
      DLINK_REMOVE_FIRST( *pSlhd, pEl, dlink );
      FreeStringListEl( pEl );
      }
   }
