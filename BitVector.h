//
// Copyright 2009-2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

/*
   The BitVector class is designed for efficiency in checking whether any bit is
   set.  Initial target is screen refresh code: a set bit indicates a screen line
   needing refresh, so if any are set we go off into the refresh logic.  Since
   the "anything pending" parallel/cached status bit no longer exists, it's
   important that the any-bit-set?  test be speedy.

   Note that BitVector<U32> is as efficient as it gets on a 32-bit CPU (big
   shock there!); a BitVector<U64> is no better.

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
      for( P pel=d_bits ; pel < pastEnd ; ++pel )
         if( *pel )
            return true;

      return false;
      }
   void ClrAllBits() {
      const P pastEnd( d_bits + d_T_els );
      for( P pel=d_bits ; pel < pastEnd ; ++pel )
         *pel = 0;
      }
   void SetAllBits() {
      const P pastEnd( d_bits + d_T_els );
      for( P pel=d_bits ; pel < pastEnd ; ++pel )
         *pel = ALLBITS;
      }
   };
