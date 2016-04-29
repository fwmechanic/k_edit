//
// Copyright 2015-2016 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#pragma once

/*
   The BitVector class is designed for efficiency in checking whether any bit is
   set.  Initial target is screen refresh code: a set bit indicates a screen line
   needing refresh, so if any are set we go off into the refresh logic.  Since
   the "anything pending" parallel/cached status bit no longer exists, it's
   important that the any-bit-set?  test be speedy.

   Note that BitVector<uint32_t> is as efficient as it gets on a 32-bit CPU (big
   shock there!); a BitVector<uint64_t> is no better.

   20090223 kgoodwin
*/
template <class T>
class BitVector {
   typedef       T *P;
   typedef const T *CP;
   enum { BITS_PER_EL = 8*sizeof(T), ALLBITS = ~static_cast<T>(0) };

   const size_t     d_T_els ;
   const P          d_bits  ;

private:

   size_t bitnum( size_t bn ) const { return bn % BITS_PER_EL; }
   size_t elenum( size_t bn ) const { return bn / BITS_PER_EL; }

public:

   BitVector( size_t bits )
   : d_T_els( (bits+BITS_PER_EL-1)/BITS_PER_EL )
   , d_bits(  P( Alloc0d( d_T_els * sizeof(T) ) ) )
   {}

   ~BitVector() { delete d_bits; }

   void SetBit  ( size_t bn )       {         d_bits[ elenum( bn ) ] |=  BIT( bitnum( bn ) ); }
   void ClrBit  ( size_t bn )       {         d_bits[ elenum( bn ) ] &= ~BIT( bitnum( bn ) ); }
   bool IsBitSet( size_t bn ) const { return (d_bits[ elenum( bn ) ] &   BIT( bitnum( bn ) )) != 0; }
   bool IsAnyBitSet() const { // _this_ is the reason for this class existing: fast search for any bit being set
      const P pastEnd( d_bits + d_T_els );
      for( P pel=d_bits ; pel < pastEnd ; ++pel ) {
         if( *pel ) {
            return true;
            }
         }
      return false;
      }
   void ClrAllBits() {
      const P pastEnd( d_bits + d_T_els );
      for( P pel=d_bits ; pel < pastEnd ; ++pel ) {
         *pel = 0;
         }
      }
   void SetAllBits() {
      const P pastEnd( d_bits + d_T_els );
      for( P pel=d_bits ; pel < pastEnd ; ++pel ) {
         *pel = ALLBITS;
         }
      }
   };
