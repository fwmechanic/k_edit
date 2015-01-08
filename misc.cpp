//
// Copyright 1991 - 2015 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

char Xbuf::ds_empty = 0;

// takes counted-string param so *pStart doesn't have to be forcibly
// NUL-terminated (it may be a const string)
//
PChar Getenv( PCChar pStart, int len ) {
   ALLOCA_STRDUP( buf, slen, pStart, len )    0 && DBG("Getenv '%s'", buf );
   return getenv( buf );
   }

PChar GetenvStrdup( PCChar pStart, size_t len ) {
   ALLOCA_STRDUP( buf, slen, pStart, len )    0 && DBG("GetenvStrdup '%s'", buf );
   return GetenvStrdup( buf );
   }

PChar GetenvStrdup( PCChar pszEnvName ) {
   auto penv( getenv( pszEnvName ) );
   if( penv )
       penv = Strdup( penv );

   return penv;
   }

STATIC_VAR struct {
   PCChar name;
   int    counter;
   } pseudoBufInfo[] = {
     {"grep"},
     {"sel"},
   };

PFBUF PseudoBuf( ePseudoBufType pseudoBufType, int fNew ) {
   auto &selNum( pseudoBufInfo[ pseudoBufType ].counter );
   if( fNew )
      ++selNum;

   return OpenFileNotDir_NoCreate( FmtStr<20>( "<%s.%u>", pseudoBufInfo[ pseudoBufType ].name, selNum ) );
   }

bool ARG::nextselbuf() {
   ++pseudoBufInfo[ SEL_BUF ].counter;
   return true;
   }
