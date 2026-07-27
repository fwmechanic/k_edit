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
#include <cstdio>
#include <cstdlib>

#include "my_types.h"
#include "ed_mem.h"

PVoid AllocNZ_( size_t bytes ) {
   return malloc( bytes ? bytes : 1 );
   }

PVoid Alloc0d_( size_t bytes ) {
   return calloc( bytes ? bytes : 1, 1 );
   }

PVoid ReallocNZ_( PVoid pv, size_t bytes ) {
   return realloc( pv, bytes ? bytes : 1 );
   }

void Free_( PVoid pv ) {
   free( pv );
   }

void Abend_MemAllocFailed( PCChar, int, size_t ) {
   abort();
   }

void Abend_UintMulOvflow( PCChar, int, uintmax_t, uintmax_t, uintmax_t ) {
   abort();
   }

#include "BitVector.h"

int main() {
   BitVector<uint64_t> exactWord( 64 );
   assert( !exactWord.IsAnyBitSet() );
   exactWord.SetBit( 0 );
   exactWord.SetBit( 63 );
   exactWord.SetBit( 64 ); // one past the logical and allocated range
   assert( exactWord.IsBitSet( 0 ) );
   assert( exactWord.IsBitSet( 63 ) );
   assert( !exactWord.IsBitSet( 64 ) );
   exactWord.ClrBit( 0 );
   exactWord.ClrBit( 63 );
   exactWord.ClrBit( 64 );
   assert( !exactWord.IsAnyBitSet() );

   BitVector<uint64_t> partialWord( 65 );
   partialWord.SetBit( 64 );
   partialWord.SetBit( 65 ); // allocated word exists, but this logical bit does not
   assert( partialWord.IsBitSet( 64 ) );
   assert( !partialWord.IsBitSet( 65 ) );

   BitVector<uint64_t> empty( 0 );
   empty.SetBit( 0 );
   empty.ClrBit( 0 );
   assert( !empty.IsBitSet( 0 ) );
   assert( !empty.IsAnyBitSet() );

   puts( "PASS" );
   return 0;
   }
