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

#pragma once

#define  DLINK_HEAD_KEEPS_COUNT  1
#if DLINK_HEAD_KEEPS_COUNT
#define  DLINK_COUNT( x )  x
#else
#define  DLINK_COUNT( x )
#endif

//
// Doubly Linked List template/macro-set that preserves type-checking and allows
// an object to be a member of more than one DLL at once.  The last is not a
// frequent requirement, but this capability can be accomodated w/o extra
// overhead, so why not provide it?
//
// Limitation: in order to support linking an object into multiple Doubly Linked
// Lists simultaneously, the OPERATION MACROS (e.g.  DLINK_INSERT_FIRST,
// DLINK_REMOVE) are parameterized by 'link field name' (the _name_ of the
// struct/class member that links the list of interest).  As a result, we can't
// implement OPERATIONS as template functions (each usage of such a template will
// occur at a different offset within T, duplicating code hugely).  Instead,
// macros are used (oy vey!), which end up duplicating code in a slightly
// different way...
//
// An important requirement of this implementation is that type-safety NOT be
// compromised; there are NO void pointers, and only a few const_casts (and no
// other casts) used herein!
//
// One advantage of this implementation is that the prev and next pointers are
// (intended to be) stored within the object being linked.  IOW, the extra node
// mem-block containing prev-node+next-node+object pointers commonly found in
// other implementations is not necessary in this implementation.
//
// ONLY macro code (erstwhile member functions) should use the (public) data
// members of DLinkEntry.
//
// An arbitrary element can be removed without traversing the list.  A new
// element can be added before or after an existing element, or at the head or
// tail of the list.
//
// DLINK lists requiring mutual-exclusion facilities (mutex or RWLock as
// appropriate) should be declared as DLinkHeadMutex<T> / DLinkHeadRWLock<T>
// rather than DLinkHead, and the client-code should use Lock() and Unlock() (or
// LockRd()/UnlockRd() or LockWr()/UnlockWr()) as necessary.
//

template <class T>
struct DLinkHead {
   // following members are logically private, should be accessed only by DLINK_ macros below
   typedef       T * P;
   typedef const T *CP;
   P                dl_first = nullptr;
   P                dl_last  = nullptr;
   DLINK_COUNT( unsigned count = 0; )

public: // the intended "public interface"

   void clear()          { dl_first = dl_last = nullptr; count = 0; }
   bool empty()    const { return dl_first == nullptr; }
   CP   frontK()   const { return cast_add_const(CP)(dl_first); } // probably never needed; DO NOT rename front()
   P    front()    const { return                    dl_first ; }
   CP   backK()    const { return cast_add_const(CP)(dl_last) ; } // probably never needed; DO NOT rename back()
   P    back ()    const { return                    dl_last  ; }
   DLINK_COUNT( unsigned length() const { return count; } )
   };


template <class T>
struct DLinkEntry {
   // following members are logically private, should be accessed only by DLINK_ macros below
   typedef       T * P;
   typedef const T *CP;
   P                dl_next = nullptr;
   P                dl_prev = nullptr;

public: // the intended "public interface"

   void clear()       { dl_next = dl_prev = nullptr; }
   CP Next()    const { return cast_add_const(CP)(dl_next); }
   P  Next()          { return                    dl_next ; }
   CP Prev()    const { return cast_add_const(CP)(dl_prev); }
   P  Prev()          { return                    dl_prev ; }
   };


#define DLL_ASSERT( expr )

// 'field' is always the name of a DLinkEntry instance contained within the linked object
// 'field' is the reason all of the below are MACROS not functions or templates

#define DLINK_NEXT( pEl, field )        ((pEl)->field.dl_next)
#define DLINK_PREV( pEl, field )        ((pEl)->field.dl_prev)

// these loops traverse correctly even if var is deleted within loop

// DLINKC_.. means "Const thisvar" (user code is NOT altering the list)
// DLINK..A means "auto thisvar" (thisvar is declared auto inside the for loop defined by the macro)

#define DLINKC_FIRST_TO_LAST( Hd,field,thisvar)        for(     (thisvar)=(Hd).front() ;                                                                 (thisvar)  ; (thisvar)=DLINK_NEXT((thisvar),field) )
#define DLINKC_FIRST_TO_LASTA(Hd,field,thisvar)        for( auto thisvar =(Hd).front() ;                                                                  thisvar   ;  thisvar =DLINK_NEXT((thisvar),field) )
#define DLINK_FIRST_TO_LAST(  Hd,field,thisvar,nxtvar) for(     (thisvar)=(Hd).front() ; ((nxtvar)=((thisvar) ? DLINK_NEXT((thisvar),field ) : nullptr), (thisvar)) ; (thisvar)=(nxtvar) )

#define DLINKC_LAST_TO_FIRST( Hd,field,thisvar)        for( auto thisvar =(Hd).back()  ;                                                                  thisvar   ;  thisvar =DLINK_PREV((thisvar),field) )
#define DLINK_LAST_TO_FIRST(  Hd,field,thisvar,nxtvar) for(     (thisvar)=(Hd).back()  ; ((nxtvar)=((thisvar) ? DLINK_PREV((thisvar),field ) : nullptr), (thisvar)) ; (thisvar)=(nxtvar) )

// coding convention: when reading a field, the corresponding method is
// used; when writing, assignment is (unavoidably[1]) direct to the field
// [1] since I _don't_ want to write "setter methods" for each field

// pNew is new element
#define DLINK_INSERT_FIRST(Hd, pNew, field)       \
do { auto &Hd_( Hd ); auto pNew_( pNew );         \
   if( (pNew_->field.dl_next = Hd_.front()) ) {   \
      Hd_.front()->field.dl_prev = pNew_;         \
      }                                           \
   else {                                         \
      DLL_ASSERT(Hd_.back() == nullptr);          \
      Hd_.dl_last = pNew_;                        \
      }                                           \
   Hd_.dl_first = pNew_;                          \
   pNew_->field.dl_prev = nullptr;                \
   DLINK_COUNT( ++Hd_.count; )                    \
   } while( 0 )

// pNew is new element
#define DLINK_INSERT_LAST(Hd, pNew, field)        \
do { auto &Hd_( Hd ); auto pNew_( pNew );         \
   if( (pNew_->field.dl_prev = Hd_.back()) ) {    \
      Hd_.back()->field.dl_next = pNew_;          \
      }                                           \
   else {                                         \
      DLL_ASSERT(Hd_.front() == nullptr);         \
      Hd_.dl_first = pNew_;                       \
      }                                           \
   Hd_.dl_last = pNew_;                           \
   pNew_->field.dl_next = nullptr;                \
   DLINK_COUNT( ++Hd_.count; )                    \
   } while( 0 )

// pRef is existing element; pNew is new element
#define DLINK_INSERT_AFTER(Hd, pRef, pNew, field)              \
do { auto &Hd_( Hd ); auto pRef_( pRef ); auto pNew_( pNew );  \
   DLL_ASSERT(DLINK_ON_LIST(Hd_, pRef_, field));               \
   if( (pNew_->field.dl_next = pRef_->field.dl_next) ) {       \
      pRef_->field.dl_next->field.dl_prev = pNew_;             \
      }                                                        \
   else {                                                      \
      Hd_.dl_last = pNew_;                                     \
      }                                                        \
   pRef_->field.dl_next = pNew_;                               \
   pNew_->field.dl_prev = pRef_;                               \
   DLINK_COUNT( ++Hd_.count; )                                 \
   } while( 0 )

// pRef is existing element; pNew is new element
#define DLINK_INSERT_BEFORE(Hd, pRef, pNew, field)             \
do { auto &Hd_( Hd ); auto pRef_( pRef ); auto pNew_( pNew );  \
   DLL_ASSERT(DLINK_ON_LIST(Hd_, pRef_, field));               \
   if( (pNew_->field.dl_prev = pRef_->field.dl_prev) ) {       \
      pRef_->field.dl_prev->field.dl_next = pNew_;             \
      }                                                        \
   else {                                                      \
      Hd_.dl_first = pNew_;                                    \
      }                                                        \
   pRef_->field.dl_prev = pNew_;                               \
   pNew_->field.dl_next = pRef_;                               \
   DLINK_COUNT( ++Hd_.count; )                                 \
   } while( 0 )

// remove pEl from the list Hd
#define DLINK_REMOVE(Hd, pEl, field)                            \
do { auto &Hd_( Hd ); auto pEl_( pEl );                         \
   DLL_ASSERT(DLINK_ON_LIST(Hd_, pEl_, field));                 \
   if( pEl_->field.dl_prev ) {                                  \
      pEl_->field.dl_prev->field.dl_next = pEl_->field.dl_next; \
      }                                                         \
   else {                                                       \
      Hd_.dl_first = pEl_->field.dl_next;                       \
      }                                                         \
   if( pEl_->field.dl_next ) {                                  \
      pEl_->field.dl_next->field.dl_prev = pEl_->field.dl_prev; \
      }                                                         \
   else {                                                       \
      Hd_.dl_last = pEl_->field.dl_prev;                        \
      }                                                         \
   DLINK_COUNT( --Hd_.count; )                                  \
   pEl->field.clear();                                          \
   } while( 0 )

// pEl is UNINITIALIZED on entry, is an effective return value!
#define DLINK_REMOVE_FIRST(Hd, pEl, field)            \
do { auto &Hd_( Hd );                                 \
   pEl = Hd_.front();                                 \
   if( pEl ) {                                        \
      Hd_.dl_first = pEl->field.dl_next;              \
      if( Hd_.front() ) {                             \
         Hd_.front()->field.dl_prev = nullptr;        \
         }                                            \
      else {                                          \
         Hd_.dl_last = nullptr;                       \
         }                                            \
      DLINK_COUNT( --Hd_.count; )                     \
      pEl->field.clear();                             \
      }                                               \
   } while( 0 )

// Move srcHd's list onto the end of destHd's list
#define DLINK_JOIN(destHd, srcHd, field)              \
do { auto &destHd_(destHd); auto &srcHd_(srcHd);      \
   if( !destHd_.front() ) { /* destHd empty */        \
      destHd_.dl_first = srcHd_.front();              \
      destHd_.dl_last  = srcHd_.back();               \
      }                                               \
   else { /* cat srcHd onto destHd */                 \
      destHd_.back()->field.dl_next = srcHd_.front(); \
      srcHd_.front()->field.dl_prev = destHd_.back(); \
      destHd_.dl_last = srcHd_.back();                \
      }                                               \
   DLINK_COUNT( destHd_.count += srcHd_.count; )      \
   srcHd_.clear();                                    \
   } while( 0 )

// Move srcHd's list to destHd; srcHd is left empty
#define DLINK_MOVE_HD(destHd, srcHd)                  \
do { auto &destHd_(destHd); auto &srcHd_(srcHd);      \
   destHd_.dl_first = srcHd_.front();                 \
   destHd_.dl_last  = srcHd_.back();                  \
   DLINK_COUNT( destHd_.count = srcHd_.count; )       \
   srcHd_.clear();                                    \
   } while( 0 )
