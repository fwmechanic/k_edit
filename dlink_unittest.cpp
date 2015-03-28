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

#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include "dlink.h"

#define  ELEMENTS(array)  (sizeof(array)/sizeof(array[0]))
#define  AEOB( array )   array , ELEMENTS(array), #array

struct inta {
   const int a;
   DLinkEntry<inta> dlink;
   inta( int a_ ) : a( a_ ) {}
   };

typedef DLinkHead<inta> head_t;

void assert_inta( const head_t &hd, const int arr[], const size_t els, const char *nm ) {
   if( els != hd.length() ) {
      std::cerr << nm << ": count miscmp: " << els << " != " << hd.length() << "\n";
      exit(1);
      }
   auto ix = 0u;
   DLINKC_FIRST_TO_LASTA( hd, dlink, pEl ) {
      if( pEl->a != arr[ix] ) {
         std::cerr << nm << "[" << ix << "]: miscmp: " << pEl->a << " != " << arr[ix] << "\n";
         exit(1);
         }
      ++ix;
      }
   if( els != ix ) {
      std::cerr << nm << ": overrun: " << els << " != " << ix << "\n";
      exit(1);
      }
   }

int main() {
   head_t  Hd;

   DLINK_INSERT_FIRST(Hd, new inta( 1 ), dlink);              { const int ref1[] = { 1 }; assert_inta( Hd, AEOB( ref1 ) ); }
   DLINK_INSERT_LAST(Hd, new inta( 2 ), dlink);               { const int ref2[] = { 1,2 }; assert_inta( Hd, AEOB( ref2 ) ); }
   DLINK_INSERT_LAST(Hd, new inta( 3 ), dlink);               { const int ref3[] = { 1,2,3 }; assert_inta( Hd, AEOB( ref3 ) ); }
   DLINK_INSERT_FIRST( Hd, new inta( 12 ), dlink );           { const int ref3[] = { 12,1,2,3 }; assert_inta( Hd, AEOB( ref3 ) ); }
   auto pNew = new inta( 0 );
   DLINK_INSERT_BEFORE( Hd, Hd.front(), pNew, dlink );        { const int ref4[] = { 0,12,1,2,3 }; assert_inta( Hd, AEOB( ref4 ) ); }
                                                              assert( Hd.front() == pNew );

   DLINK_INSERT_BEFORE( Hd, pNew, new inta( -1 ), dlink );    { const int ref5[] = { -1,0,12,1,2,3 }; assert_inta( Hd, AEOB( ref5 ) ); }
   DLINK_INSERT_AFTER(  Hd, pNew, new inta( -2 ), dlink );    { const int ref8[] = { -1,0,-2,12,1,2,3 }; assert_inta( Hd, AEOB( ref8 ) ); }
   DLINK_REMOVE(Hd, pNew, dlink);                             { const int ref6[] = { -1,-2,12,1,2,3 }; assert_inta( Hd, AEOB( ref6 ) ); }
   delete pNew;
   DLINK_INSERT_BEFORE( Hd, Hd.back(),new inta( 0 ),dlink );
   head_t  Hd2;
         { const int ref7[] = { -1,-2,12,1,2,0,3 };           assert_inta( Hd, AEOB( ref7 ) );
         DLINK_MOVE_HD( Hd2, Hd );                            assert_inta( Hd2, AEOB( ref7 ) );
                                                              assert( Hd.empty() );
         }
   DLINK_INSERT_FIRST( Hd, new inta( 20 ), dlink );
   DLINK_INSERT_FIRST( Hd, new inta( 21 ), dlink );
   DLINK_INSERT_FIRST( Hd, new inta( 22 ), dlink );           { const int ref9[] = { 22,21,20 }; assert_inta( Hd, AEOB( ref9 ) ); }
   DLINK_JOIN(Hd, Hd2, dlink);                                { const int refA[] = { 22,21,20,-1,-2,12,1,2,0,3 }; assert_inta( Hd, AEOB( refA ) ); }
                                                              assert( Hd2.empty() );
   printf( "PASS\n");
   return 0;
   }
