//
//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
//

#pragma once

#include <limits>  // std::numeric_limits<unsigned int>::max()
#ifndef  SIZE_MAX
#define  SIZE_MAX  std::numeric_limits<size_t>::max()
#endif

extern void  MemErrFatal( PCChar opn, size_t byteCount, PCChar msg );

#define  MEM_BP  0
#if      MEM_BP
extern bool g_fMemBpEnabled;
STIL void MEM_CBP( bool fEnable )  { g_fMemBpEnabled = fEnable; }
STIL int  MEM_CBP()                { if( g_fMemBpEnabled ) { SW_BP } return 1; }
#else
STIL void MEM_CBP( bool fEnable )  {}
STIL int  MEM_CBP()                { return 1; }
#endif

// Unchecked alloc APIs: TRY! NOT TO USE THESE DIRECTLY!!!

extern PVoid AllocNZ_  (           size_t bytes );
extern PVoid Alloc0d_  (           size_t bytes );
extern PVoid ReallocNZ_( PVoid pv, size_t bytes );
extern void  Free_     ( PVoid pv               );

STIL   PVoid AllocNZ   (           size_t bytes )   { return AllocNZ_  (     bytes ); }
STIL   PVoid Alloc0d   (           size_t bytes )   { return Alloc0d_  (     bytes ); }
STIL   PVoid ReallocNZ ( PVoid pv, size_t bytes )   { return ReallocNZ_( pv, bytes ); }

extern PChar Strdup( stref src, size_t extra_nuls=0 ); // turn stref into ASCIZ (i.e. having ONE '\0' appended), w/extra_nuls _additional_ '\0' chars appended if requested
STIL   PChar Strdup( PCChar st, size_t chars ) { return Strdup( stref( st, chars ) ); }
STIL   PChar Strdup( PCChar st, PCChar eos )   { return Strdup( st, eos-st       ); }

template<typename Ptr>
inline void Delete0( Ptr &ptr ) {
   delete ptr;
   ptr = nullptr;
   }

template<typename Ptr>
inline void DeleteUp( Ptr &ptr, Ptr newp=nullptr ) {
   delete ptr;
   ptr = newp;  // updt ptr w/new value
   }

template<typename Ptr>
inline void Free0( Ptr &ptr ) {
   Free_( PVoid(ptr) );
   ptr = nullptr;
   }

template<typename Ptr>
inline void FreeUp( Ptr &ptr, Ptr newp=nullptr ) {
   Free_( PVoid(ptr) );
   ptr = newp;  // updt ptr w/new value
   }

template<typename type>
inline void MoveArray( type * dest, const type * src, size_t elements=1 ) {
   memmove( dest, src, elements * sizeof(*src) );
   }

template<typename type>
inline type * DupArray( const type *pSrc, size_t elements=1 ) {
   type *rv;
   AllocArrayNZ( rv, elements );
   memcpy( rv, pSrc, elements * sizeof(*rv) );
   return rv;
   }

template<typename Ptr>
inline void AllocBytesNZ( Ptr &rv, size_t bytes, PCChar msg="" ) {
   rv = Ptr ( AllocNZ( bytes ) );
   if( rv == nullptr )
      MemErrFatal( __func__, bytes, msg );
   }

template<typename Ptr>
inline void AllocBytesZ( Ptr &rv, size_t bytes, PCChar msg="" ) {
   rv = reinterpret_cast<Ptr>( Alloc0d( bytes ) );
   if( rv == nullptr )
      MemErrFatal( __func__, bytes, msg );
   }

STIL size_t CHK_PRODUCT( size_t nelems, size_t elsize, PCChar msg="" ) {
   if( (SIZE_MAX / elsize) < nelems ) {
      MemErrFatal( __func__, SIZE_MAX, msg );
      }
   return nelems * elsize;
   }

template<typename Ptr>
inline void AllocArrayNZ( Ptr &rv, size_t elements=1, PCChar msg="" ) {
   AllocBytesNZ( rv, CHK_PRODUCT( elements, sizeof(*rv), msg ), msg );
   }

template<typename Ptr>
inline void AllocArrayZ( Ptr &rv, size_t elements=1, PCChar msg="" ) {
   AllocBytesZ( rv, CHK_PRODUCT( elements, sizeof(*rv), msg ), msg );
   }

template<typename Ptr>
inline void ReallocArray( Ptr &rv, size_t elements=1, PCChar msg="" ) {
   const auto bytes( CHK_PRODUCT( elements, sizeof(*rv), msg ) );
   rv = reinterpret_cast<Ptr>( ReallocNZ( rv, bytes ) );
   if( rv == nullptr )
      MemErrFatal( __func__, bytes, msg );
   }
