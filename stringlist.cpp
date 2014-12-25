//
//  Copyright 1991 - 2012 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"
#include "stringlist.h"

STATIC_FXN PStringListEl NewStringListEl( boost::string_ref src ) { // pEos points past last valid char in str
   const auto sbytes( src.length() + 1 );
   PStringListEl rv;
   AllocBytesNZ( rv, sizeof( *rv ) + sbytes, __func__ );
   rv->dlink.Clear();
   memcpy( rv->string, src.data(), sbytes-1 );
   rv->string[sbytes-1] = '\0';
   return rv;
   }

void InsStringListEl( PStringListHead pSlhd, boost::string_ref src ) { 0 && DBG("%s: %" PR_BSR, __func__, BSR(src) );
   auto pIns( NewStringListEl( src ) );
   DLINK_INSERT_LAST( *pSlhd, pIns, dlink );
   }

void DeleteStringList( PStringListHead pSlhd ) {
   while( auto pEl = pSlhd->First() ) {
      DLINK_REMOVE_FIRST( *pSlhd, pEl, dlink );
      FreeStringListEl( pEl );
      }
   }
