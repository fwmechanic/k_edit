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

#include "my_fio.h"

#include <cassert>
#include <cstdio>
#include <vector>

int DBG( PCChar, ... ) {
   return 0;
   }

STATIC_FXN void write_file( PCChar filename, span<const unsigned char> content ) {
   auto ofh( fopen_ptr( filename, "wb" ) );
   assert( ofh );
   if( !content.empty() ) {
      assert( fwrite( content.data(), 1, content.size(), ofh.get() ) == content.size() );
      }
   }

STATIC_FXN std::vector<unsigned char> read_file( PCChar filename ) {
   auto ifh( fopen_ptr( filename, "rb" ) );
   assert( ifh );
   assert( fseek( ifh.get(), 0, SEEK_END ) == 0 );
   const auto bytes( ftell( ifh.get() ) );
   assert( bytes >= 0 );
   assert( fseek( ifh.get(), 0, SEEK_SET ) == 0 );
   std::vector<unsigned char> content( static_cast<size_t>(bytes) );
   if( !content.empty() ) {
      assert( fread( content.data(), 1, content.size(), ifh.get() ) == content.size() );
      }
   return content;
   }

STATIC_FXN void test_copy_size( size_t bytes ) {
   constexpr auto src = "my_fio_unittest-src.tmp";
   constexpr auto dst = "my_fio_unittest-dst.tmp";

   std::vector<unsigned char> expected( bytes );
   for( size_t ix=0 ; ix < expected.size() ; ++ix ) {
      expected[ix] = static_cast<unsigned char>( ix * 131u + ix / 251u );
      }
   if( expected.size() > 4 ) {
      expected[0] = '\r';
      expected[1] = '\n';
      expected[2] = 0;
      expected[3] = 0x1a;
      }

   write_file( src, expected );
   assert( CopyFileManuallyOk( src, dst ) );
   assert( read_file( dst ) == expected );

   remove( src );
   remove( dst );
   }

int main() {
   test_copy_size( 0 );
   test_copy_size( 1 );
   test_copy_size( 32 * 1024 - 1 );
   test_copy_size( 32 * 1024 );
   test_copy_size( 32 * 1024 + 1 );
   test_copy_size( 3 * 32 * 1024 + 17 );
   puts( "PASS" );
   return 0;
   }
