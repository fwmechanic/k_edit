//
// Copyright 2015-2018 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include <limits>  // std::numeric_limits<unsigned int>::max()
#ifndef  SIZE_MAX
#define  SIZE_MAX  std::numeric_limits<size_t>::max()
#endif

extern void  Abend_MemAllocFailed( PCChar szFile, int nLine, size_t byteCount ) __attribute__((noreturn));
extern void  Abend_UintMulOvflow( PCChar szFile, int nLine, uintmax_t nelems, uintmax_t elsize, uintmax_t maxAllowed ) __attribute__((noreturn));

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

template<typename uinttype>
inline uinttype CHK_PRODUCT( uinttype nelems, uinttype elsize, PCChar szFile, int nLine ) {
   if( nelems > std::numeric_limits<uinttype>::max() / elsize ) {
      Abend_UintMulOvflow( szFile, nLine, nelems, elsize, std::numeric_limits<uinttype>::max() );
      }
   return nelems * elsize;
   }

template<typename type>
inline void MoveArray_( type * dest, const type * src, size_t elements, PCChar szFile, int nLine ) {
   memmove( static_cast<void*>(dest), src, CHK_PRODUCT( elements, sizeof(*src), szFile, nLine ) );
   }
#define MoveArray( dest, src, elements ) MoveArray_( (dest), (src), (elements), __FILE__, __LINE__ )

template<typename Ptr>
inline void AllocBytesNZ_( Ptr &rv, size_t bytes, PCChar szFile, int nLine ) {
   rv = static_cast<Ptr>( AllocNZ( bytes ) );
   if( rv == nullptr ) {
      Abend_MemAllocFailed( szFile, nLine, bytes );
      }
   }
#define AllocBytesNZ( rv, bytes ) AllocBytesNZ_( (rv), (bytes), __FILE__, __LINE__ )

template<typename Ptr>
inline void AllocBytesZ_( Ptr &rv, size_t bytes, PCChar szFile, int nLine ) {
   rv = reinterpret_cast<Ptr>( Alloc0d( bytes ) );
   if( rv == nullptr ) {
      Abend_MemAllocFailed( szFile, nLine, bytes );
      }
   }
#define AllocBytesZ( rv, bytes ) AllocBytesZ_( (rv), (bytes), __FILE__, __LINE__ )

template<typename Ptr>
inline void AllocArrayNZ_( Ptr &rv, size_t elements, PCChar szFile, int nLine ) {
   AllocBytesNZ_( rv, CHK_PRODUCT( elements, sizeof(*rv), szFile, nLine ), szFile, nLine );
   }
#define AllocArrayNZ( rv, elements ) AllocArrayNZ_( (rv), (elements), __FILE__, __LINE__ )

template<typename Ptr>
inline void AllocArrayZ_( Ptr &rv, size_t elements, PCChar szFile, int nLine ) {
   AllocBytesZ_( rv, CHK_PRODUCT( elements, sizeof(*rv), szFile, nLine ), szFile, nLine );
   }
#define AllocArrayZ( rv, elements ) AllocArrayZ_( (rv), (elements), __FILE__, __LINE__ )

template<typename Ptr>
inline void ReallocArray_( Ptr &rv, size_t elements, PCChar szFile, int nLine ) {
   const auto bytes( CHK_PRODUCT( elements, sizeof(*rv), szFile, nLine ) );
   rv = reinterpret_cast<Ptr>( ReallocNZ( rv, bytes ) );
   if( rv == nullptr ) {
      Abend_MemAllocFailed( szFile, nLine, bytes );
      }
   }
#define ReallocArray( rv, elements ) ReallocArray_( (rv), (elements), __FILE__, __LINE__ )

template<typename type>
inline type * DupArray_( const type *pSrc, size_t elements, PCChar szFile, int nLine ) {
   type *rv;
   const auto bytes( CHK_PRODUCT( elements, sizeof(*rv), szFile, nLine ) );
   AllocBytesNZ_( rv, bytes, szFile, nLine );
   memcpy( static_cast<void*>(rv), pSrc, bytes );
   return rv;
   }
#define DupArray( pSrc, elements ) DupArray_( (pSrc), (elements), __FILE__, __LINE__ )
