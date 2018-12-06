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

#if !defined(WL)
// porting abbreviation tool
#if defined(_WIN32)
#   define WL(ww,ll)  ww
#else
#   define WL(ww,ll)  ll
#endif
#endif

// I've been getting auto-happy lately, but in some cases we don't want (or can't use) auto;
// to avoid wasting time re-learning why that this cannot be done in those cases, annotate them with
#define  NOAUTO

//
// NB: macros that expand to 'static' should have a '-I macnm=static' entry in %~dp0ctags.d\k-src.ctags
//
#define  STIL  static inline
#define  STATIC_VAR  static
#define  COMPLEX_STATIC_VAR  STATIC_VAR
#define  STATIC_FXN  static
#define  GLOBAL_VAR
#define  GLOBAL_CONST        const
#define  STATIC_CONST static const

#include <cinttypes>

//  General type Definitions
//
typedef unsigned int UI;
typedef signed   int SI;

typedef  size_t  uint_machineword_t; // 32-bit on i386, 64-bit on x64

#define CAST_AWAY_CONST(less_const_type)  const_cast<less_const_type>
#define cast_add_const(more_const_type)   const_cast<more_const_type>

typedef       char *         PChar;
typedef const char *        PCChar;
typedef      PChar *        PPChar;
typedef char const * const CPCChar;
typedef     PCChar *       PPCChar;
typedef    CPCChar *      CPPCChar;

typedef       void *         PVoid;
typedef const void *        PCVoid;

// Memory models
//                           short int  long    long long    ptr/size_t
// x64 Windows LLP64 IL32P64   16  32    32         64          64
// x64 Linux   LP64  I32LP64   16  32    64         64          64
// 32  <both>   ??     ??      16  32    32         32?         32

#if defined(__GNUC__)
    #define PR__i64       "ll"
    #define PR__i64d      PR__i64 "d"
    #define PR_SIZET      "zu"
    #define PR_PTRDIFFT   "td"
    #define PR_BSRSIZET   WL(PR_SIZET,"lu")
    #define PR_FILESIZET  WL(PR__i64d,"ld")
    #if defined(__x86_64__) || defined(__ppc64__)
        // #define ENVIRONMENT64
        #if defined(_WIN32)
        #   define PR_TIMET PR__i64d
        #else
        #   define PR_TIMET "ld"
        #endif
    #else
        // #   define ENVIRONMENT32
        #   define PR_TIMET "ld"
    #endif
#else
    #error only GCC supported!
#endif

#include <string>

typedef std::back_insert_iterator<std::string> string_back_inserter;

// driven by https://news.ycombinator.com/item?id=8704318 I've deployed
// boost::string_ref to minimize gratuitious std::string mallocs/copies

#include <boost/version.hpp>
#include <boost/utility/string_ref.hpp>
                                 // The final 32-bit Nuwen MinGW release (10.4) contains Boost 1.54
typedef boost::string_ref stref; // http://www.boost.org/doc/libs/1_54_0/libs/utility/doc/html/string_ref.html
typedef stref::size_type  sridx; // a.k.a. boost::string_ref::size_type
constexpr auto eosr = stref::npos; // tag not generated if 'const auto eosr( stref::npos )' syntax is used!
typedef stref::const_iterator         stref_iter;
typedef stref::const_reverse_iterator stref_riter;

// we sometimes need to "printf" (DBG) boost::string_ref referents
// unfortunately static_cast<int> of size_t seems unavoidable per
// http://stackoverflow.com/questions/19145951/printf-variable-string-length-specifier
// http://stackoverflow.com/questions/8081613/using-size-t-for-specifying-the-precision-of-a-string-in-cs-printf
// this is dangerous; hopefully really long strings will not be encountered :-(
//      BSR() is used to pass a stref as a single unit in a printf vararg param list (it expands to two vararg params)
//      BSR()'s corresponding printf format string is expected to be PR_BSR (which consumes two vararg params)
#define BSR(bsr) static_cast<int>((bsr).length()),(bsr).data()
#define PR_BSR ".*s"

//      sr2st(stref) convert a stref to a std::string; there seems to be no added overhead if the created std::string is a temp object
STIL std::string sr2st( stref bsr ) { return std::string( bsr.data(), bsr.length() ); }
// #define       sr2st(       bsr )          std::string( (bsr).data(),(bsr).length() )
STIL stref se2sr( PCChar bos, PCChar eos ) { return stref( bos, eos - bos ); }

typedef WL(__int64,off_t) filesize_t;
typedef signed long       FilesysTime;

#include "attr_format.h"

#if !defined(__GNUC__)
#define __func__  __FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif
#define  FUNC  __PRETTY_FUNCTION__

#if defined(_WIN32)
   #define CDECL__ __cdecl
#else
   #define CDECL__
#endif

//--- EXTERNC depends upon C/C++ ---
//
// note that EXTERNC will affect an actual declaration (definition?) of a
// variable, not "just" affect the external linkage-name of an object defined
// elsewhere...
//
#ifdef __cplusplus
#   define  EXTERNC  extern "C"
#else
#   define  EXTERNC  extern
#endif

#define NO_COPYCTOR(c)   c             ( const c & ) = delete
#define NO_ASGN_OPR(c)   c & operator= ( const c & ) = delete

#define  NewScope

// returns the number of elements in an array
#define  ELEMENTS(array)  (sizeof(array)/sizeof(array[0]))

// from Meyers Effective Modern C++
// Attempts to instantiate this template
template<typename T> class TD;
// will elicit an error message, because there's no template definition to
// instantiate.  To see the types for x and y, just try to instantiate TD
// with their types:
// TD<decltype(x)> xType; // elicit errors containing
// TD<decltype(y)> yType; // x's and y's types

// Since I've adopted a tactic of always including a trailing 'sizeof buffer'
// parameter whenever specifying a PChar (destination) parameter, here is a macro
// that will save some typing in circumstances where an array variable name (vs. a
// pointer) is passed to such a function:
//
// BSOB => "Buffer, SizeOf Buffer"
// AEOA => "Array , ElementsOf Array"
#define  BSOB( buffer )   buffer , sizeof(buffer)
#define  AEOA( array )    array , ELEMENTS(array)

//-----
// "To ensure compatibility with C code and pre-bool C++ code, implicit numeric
// to bool conversions adhere to C's convention.  Every nonzero value is
// implicitly converted to true in a context that requires a bool value, and a
// zero value is implicitly converted to false."
//
// However:
//
// C++ directly assigning int (numeric) rvalues to bool variables causes this
// diagnostic from MSVC 7.1
//
// warning C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)
//
// to avoid this (with no semantic change), use 'boolvar = ToBOOL(numexpr)'
//
#define ToBOOL(numexpr)     (!!(numexpr))

#define BIT(n)     (static_cast<unsigned long long>(1) << (n))
#define BITS(n)    (BIT(n)-1)
// BITS(n) has a bug: when n == the # of bits in unsigned long long (64), the expr BIT(n) overflows
// as a stopgap, we have ALLBITS(T):
#define ALLBITS(T) (~static_cast<T>(0))

// Macro that determines if a number is a power of two
#define ISPOWER2(x) (!((x)&((x)-1)))

#define ROUNDUP_TO_NEXT_POWER2( val, pow2 )  ((val + ((pow2)-1)) & ~((pow2)-1))

// sizeof_struct_member
// called like offsetof, but returns sizeof the member of the struct
// (otherwise, you'd have to declare an instance of struct, then sizeof(instance.member))
#define sizeof_struct_member(type,memb)  sizeof( (*(type*)0).memb )

// use KSTRLEN on a const char array containing ASCIIZ string ONLY!
#define KSTRLEN( szCharArray )   (sizeof( szCharArray ) - 1)

#ifndef CompileTimeAssert
   // now using static_assert
   #define CompileTimeAssert(expr)  extern char _CompileTimeAssert[(expr)?1:-1]
// #define CompileTimeAssert(expr)  struct CompileTimeAssert_ ## __FILE__ ## _ ## __LINE__ { int bf : (expr); }
// #define CompileTimeAssert(expr)  do { typedef int ai[(expr)?1:0]; } while(0)
// #define CompileTimeAssert(expr)  extern char _CompileTimeAssert[expr?1:0];
#endif

//
// Calling a class method given an object and a pointer to a member function is
// a MASSIVE SYNTACTIC HEADACHE!  Instead use this macro to do it.
//
// NB: If you have a pointer to an object, the invocation will look like
//
// CALL_METHOD( *pObject, pMethod )()
//
// from http://www.parashift.com/c++-faq-lite/pointers-to-members.html#faq-33.5
//
#define  CALL_METHOD( object, pMethod )  ((object).*(pMethod))

namespace Path {
   typedef std::string str_t;
   };

// readability template macros

template<typename T> inline T    Abs ( T t ) { return ( t < 0 ) ?  -t : +t; }
template<typename T> inline T    Sign( T t ) { return ( t < 0 ) ?  -1 : +1; }

template<typename T> inline T    AbsDiff   ( T t1, T t2 )    { return (t1 > t2) ? (t1-t2) : (t2-t1); }

template<typename T> inline void NoLessThan( T *v, T limit ) { if( *v <= limit ) {*v = limit;} }
template<typename T> inline void NoMoreThan( T *v, T limit ) { if( *v >= limit ) {*v = limit;} }

template<typename T>
inline void Constrain( T loLimit, T *v, T hiLimit ) {
   if( *v >= hiLimit ) { *v = hiLimit; }
   if( *v <= loLimit ) { *v = loLimit; }
   }

template<typename T>
bool WithinRangeInclusive( T nLower, T toCheck, T nUpper ) {
   return (toCheck >= nLower && toCheck <= nUpper); // ASSUMES nLower <= nUpper !!!
   }

class BoolOneShot { // simple utility functor
   bool first = true;
public:
   operator bool()   { const auto rv( first ); first = false; return rv; }
   };

// for making writable copies of const char []
#define  ALLOCA_STRDUP( NmOf_pDest, NmOf_strlenVar, pSrcStr, pSrcStrlen ) \
      const int NmOf_strlenVar( pSrcStrlen );                             \
      PChar NmOf_pDest = PChar( alloca( NmOf_strlenVar + 1 ) );           \
      memcpy( NmOf_pDest, pSrcStr, NmOf_strlenVar );                      \
      NmOf_pDest[ NmOf_strlenVar ] = '\0';

#if !defined(__GNUC__)
#define __func__  __FUNCTION__
#endif
