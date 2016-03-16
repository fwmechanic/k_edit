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

// since I seem to be having trouble with boost::string_ref on CentOS 7 (20160312) ...

typedef       char *         PChar;
typedef const char *        PCChar;
typedef      PChar *        PPChar;
typedef char const * const CPCChar;
typedef     PCChar *       PPCChar;
typedef    CPCChar *      CPPCChar;

#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

// last 32-bit Nuwen MinGW contains Boost 1.54
// http://www.boost.org/doc/libs/1_54_0/libs/utility/doc/html/string_ref.html
// http://www.boost.org/doc/libs/1_53_0/libs/utility/doc/html/string_ref.html
#include <boost/version.hpp>
#include <boost/utility/string_ref.hpp>

typedef boost::string_ref stref;
typedef stref::size_type  sridx; // a.k.a. boost::string_ref::size_type
const auto eosr = stref::npos; // tag not generated if 'const auto eosr( stref::npos )' syntax is used!

#define  ELEMENTS(array)  (sizeof(array)/sizeof(array[0]))
#define  AEOB( array )   array , ELEMENTS(array), #array
// BSOB => "Buffer, SizeOf Buffer"
//
#define  BSOB( buffer )   buffer , sizeof(buffer)

stref scat( PChar dest, size_t sizeof_dest, stref src, size_t destLen ) {
   if( 0==destLen && sizeof_dest > 0 && dest[0] ) { destLen = strlen( dest ); }
   auto srcLen( src.length() );
   if( destLen + srcLen + 1 > sizeof_dest ) {
      srcLen = sizeof_dest - destLen - 1;
      }
   auto rv( srcLen );
   if( srcLen > 0 ) {
      memcpy( dest + destLen, src.data(), srcLen );
      dest[ destLen + srcLen ] = '\0';
      rv = destLen + srcLen;
      }
   return stref( dest, rv );
   }

stref scpy( PChar dest, size_t sizeof_dest, stref src ) {
   auto srcLen( src.length() );
   if( srcLen >= sizeof_dest ) {
       srcLen = sizeof_dest - 1;
       }
   memcpy( dest, src.data(), srcLen );
   dest[ srcLen ] = '\0';
   return stref( dest, srcLen );
   }

int main() {
   char buf[100];
   scpy( BSOB(buf), stref("testing") );
   std::cout << buf << "\n";
   std::cout << "PASS\n";
   return 0;
   }
