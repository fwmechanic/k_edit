//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"
#include "stringlist.h"

StringListEl *NewStringListEl( boost::string_ref src ) {
   const auto sbytes( src.length() + 1 );
   StringListEl *rv;
   AllocBytesNZ( rv, sizeof( *rv ) + sbytes, __func__ );
   rv->dlink.Clear();
   memcpy( rv->string, src.data(), sbytes-1 );
   rv->string[sbytes-1] = '\0';
   return rv;
   }

void InsStringListEl( StringListHead &slhd, boost::string_ref src ) { 0 && DBG("%s: %" PR_BSR, __func__, BSR(src) );
   auto pIns( NewStringListEl( src ) );
   DLINK_INSERT_LAST( slhd, pIns, dlink );
   }

void DeleteStringList( StringListHead &slhd ) {
   while( auto pEl = slhd.First() ) {
      DLINK_REMOVE_FIRST( slhd, pEl, dlink );
      FreeStringListEl( pEl );
      }
   }
