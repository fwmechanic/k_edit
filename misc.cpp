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
