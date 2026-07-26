//
// Copyright 2026 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include <cassert>
#include <cstring>
#include <iostream>

#include "my_strutils.h"

int isWordChar( int ) {
   return false;
   }

int DBG( PCChar, ... ) {
   return 0;
   }

STATIC_FXN int upperAscii( int ch ) {
   return ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch;
   }

int main() {
   char sprintfDest[20];
   assert( safeSprintf( span{sprintfDest}, "%s:%d", "value", 3 ) == sprintfDest );
   assert( 0 == strcmp( sprintfDest, "value:3" ) );

   char sprintfUntouched = 'x';
   assert( safeSprintf( stbuf( &sprintfUntouched, 0 ), "ignored" ) == &sprintfUntouched );
   assert( sprintfUntouched == 'x' );

   char doubled[8];
   assert( DoubleBackslashes( span{doubled}, "a\\b" ) == 4 );
   assert( 0 == strcmp( doubled, "a\\\\b" ) );

   char doubledTruncated[4];
   assert( DoubleBackslashes( span{doubledTruncated}, "a\\b" ) == 3 );
   assert( 0 == strcmp( doubledTruncated, "a\\\\" ) );

   char doubleUntouched = 'x';
   assert( DoubleBackslashes( stbuf( &doubleUntouched, 0 ), "a\\b" ) == 0 );
   assert( doubleUntouched == 'x' );

   char copyDest[4];
   assert( scpy( span{copyDest}, "abcde" ) == 3 );
   assert( 0 == strcmp( copyDest, "abc" ) );

   char appendDest[6] = "ab";
   assert( scat( span{appendDest}, "cdef" ) == 5 );
   assert( 0 == strcmp( appendDest, "abcde" ) );

   char explicitLenDest[6] = { 'a', 'b', 'x', 'x', 'x', 'x' };
   assert( scat( span{explicitLenDest}, "cd", 2 ) == 4 );
   assert( 0 == strcmp( explicitLenDest, "abcd" ) );

   char copyUntouched = 'x';
   assert( scpy( span<char>( &copyUntouched, 0 ), "abc" ) == 0 );
   assert( copyUntouched == 'x' );

   char untouched = 'x';
   xlatStr( span<char>( &untouched, 0 ), "abc", upperAscii );
   assert( untouched == 'x' );

   char nulOnly[] = { 'x' };
   xlatStr( span{nulOnly}, "abc", upperAscii );
   assert( nulOnly[0] == '\0' );

   char emptySrc[] = { 'x', 'x' };
   xlatStr( span{emptySrc}, "", upperAscii );
   assert( emptySrc[0] == '\0' );
   assert( emptySrc[1] == 'x' );

   char truncated[4];
   xlatStr( span{truncated}, "abcde", upperAscii );
   assert( 0 == strcmp( truncated, "ABC" ) );

   char exact[4];
   xlatStr( span{exact}, "abc", upperAscii );
   assert( 0 == strcmp( exact, "ABC" ) );

   char inPlace[] = "aBc";
   xlatStr( span{inPlace}, inPlace, upperAscii );
   assert( 0 == strcmp( inPlace, "ABC" ) );

   std::cout << "PASS\n";
   return 0;
   }
