//
// Copyright 2015-2019 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

//####  lh_ = my Lua Helper functions

#include "lua_intf_common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

int LDS( PCChar tag, lua_State *L ) {
   const auto top( lua_gettop(L) );
   DBG( "+luaStack.bottom, %d els @ '%s'", top, tag );
   for( auto ix(1); ix <= top; ++ix ) {
      const auto type( lua_type( L, ix ) );
      switch( type ) {
         case LUA_TSTRING:  DBG( " [%d]=\"%s\"", ix, lua_tostring( L, ix ) ); break;
         case LUA_TBOOLEAN: DBG( " [%d]=%s"    , ix, lua_toboolean( L, ix ) ? "true":"false" ); break;
         case LUA_TNUMBER:  DBG( " [%d]=%f"    , ix, lua_tonumber( L, ix ) ); break;
         default:           DBG( " [%d]={%s}"  , ix, lua_typename( L, type ) ); break;
         }
      }
   0 && DBG( "-luaStack.top Dump @ '%s'", tag );
   return 1;
   }

// lua_getglobal (among other Lua APIs) panics if _G[name] == nil unless we wrap in lua_pcall
STATIC_FXN int callback_getglobal_tos( lua_State* L ) {
   lua_getglobal( L, lua_tostring(L, -1) );
   return 1;
   }
STATIC_FXN bool lh_getglobal_failed( lua_State* L, PCChar name ) {
   lua_pushcfunction( L, callback_getglobal_tos );
   lua_pushstring( L, name );
   return lua_pcall(L, 1, 1, 0) != 0;  // true == error
   }

bool lh_push_global_function( lua_State *L, const char *szFuncnm ) {
   if( lh_getglobal_failed( L, szFuncnm ) ) {
      return !Msg( "%s: Lua symbol '%s' is NOT A global", __func__, szFuncnm );
      }
   if( !lua_isfunction( L, -1 ) ) {
      return !Msg( "%s: Lua symbol '%s' is NOT A FUNCTION", __func__, szFuncnm );
      }
   return false;
   }

// 20160601 replace C varargs with typesafe variadic template approach (cost: build/link time)

STATIC_FXN int lh_push_( lua_State *L, stref  in ) { lua_pushlstring(  L, in.data(), in.length() ); return 1; }
STATIC_FXN int lh_push_( lua_State *L, PFBUF  in ) { l_construct_FBUF( L, in );      return 1; }
STATIC_FXN int lh_push_( lua_State *L, PView  in ) { l_construct_View( L, in );      return 1; }
STATIC_FXN int lh_push_( lua_State *L, double in ) { lua_pushnumber(   L, in );      return 1; }
STATIC_FXN int lh_push_( lua_State *L, int    in ) { lua_pushinteger(  L, in );      return 1; }
STATIC_FXN int lh_push_( lua_State *L, PCChar in ) { lua_pushstring(   L, in );      return 1; }
STATIC_FXN int lh_push_( lua_State *L, bool   in ) { lua_pushboolean(  L, in != 0 ); return 1; }

template<typename HEAD>
int lh_push( lua_State *L, HEAD v ) { return lh_push_( L, v ); }
template<typename HEAD, typename... TAIL>
int lh_push( lua_State *L, HEAD head, TAIL... tail ) {
   const auto r1( lh_push( L, head ) );  // force order of eval HEAD -> ... TAIL
   return r1 + lh_push( L, tail... );
   }

#define LUA_TOBOOLEAN(L, ix) ToBOOL( lua_toboolean( L, ix ) )

STATIC_FXN void lua_assigncppstring( lua_State *L, int ix, std::string &out ) {
   size_t srcBytes; auto pSrc( lua_tolstring(L, ix, &srcBytes) );  0 && DBG( "%s raw='%" PR_BSR "'", __func__, (int)srcBytes, pSrc );
   out.assign( pSrc, srcBytes );                                   0 && DBG( "%s cst='%s'", __func__, out.c_str() );
   }

#if 0
STATIC_FXN bool lh_pop_bool( lua_State *L ) { return 0 != lua_toboolean( L, -1 ); }
STATIC_FXN void lh_pop_( lua_State *L, int ix, bool        &out ) { out = 0 != lua_toboolean( L, ix ); }
STATIC_FXN void lh_pop_( lua_State *L, int ix, int         &out ) { out =      lua_tointeger( L, ix ); DBG("%s =%d", __func__, out ); }
STATIC_FXN void lh_pop_( lua_State *L, int ix, double      &out ) { out =      lua_tonumber ( L, ix ); }
STATIC_FXN void lh_pop_( lua_State *L, int ix, std::string &out ) {
   size_t srcBytes; auto pSrc( lua_tolstring(L, ix, &srcBytes) );  0 && DBG( "%s raw='%" PR_BSR "'", __func__, (int)srcBytes, pSrc );
   out.assign( pSrc, srcBytes );                                   1 && DBG( "%s cst='%s'", __func__, out.c_str() );
   }

// does NOT work as intended (in all cases, v is a reference, and the referent is NOT updated
template<typename HEAD>
int lh_pop( lua_State *L, HEAD v ) { lh_pop_( L, -1, v ); return 1; }
template<typename HEAD, typename... TAIL>
int lh_pop( lua_State *L, HEAD head, TAIL... tail ) {
   const auto rt( lh_pop( L, tail... ) );  // force order of eval TAIL -> ... HEAD
   return rt +    lh_pop( L, head    )  ;
   }
#endif

STATIC_VAR lua_State* L_edit;
STATIC_VAR lua_State* L_restore_save;
GLOBAL_VAR PFBUF s_pFbufLuaLog;

STIL void get_LuaRegistry( lua_State* L, PCChar varnm ) { lua_getfield( L, LUA_REGISTRYINDEX, varnm ); }
STIL void set_LuaRegistry( lua_State* L, PCChar varnm ) { lua_setfield( L, LUA_REGISTRYINDEX, varnm ); }

void set_LuaRegistryInt( lua_State* L, PCChar varnm, int val ) {
   lua_pushinteger(L, val);
   set_LuaRegistry( L, varnm );
   }

int get_LuaRegistryInt( lua_State* L, PCChar varnm ) {
   get_LuaRegistry( L, varnm );
   const auto rv( luaL_optint( L, -1, -1 ) );
   lua_pop(L, 1);
   return rv;
   }

#define  LREGI(nm) \
   STATIC_CONST char ksz##nm[] = #nm; \
   STIL int  LREGI_get_##nm( lua_State *L          ) { return get_LuaRegistryInt( L, ksz##nm      ); } \
   STIL void LREGI_set_##nm( lua_State *L, int val ) {        set_LuaRegistryInt( L, ksz##nm, val ); }

//---------------------------

void set_LuaRegistryPtr( lua_State* L, PCChar varnm, void *val ) {
   lua_pushlightuserdata (L, val);
   set_LuaRegistry( L, varnm );
   }

void *get_LuaRegistryPtr( lua_State* L, PCChar varnm ) {
   get_LuaRegistry( L, varnm );
   auto rv( lua_touserdata( L, -1 ) );
   lua_pop(L, 1);
   return rv;
   }

#define  LREGP(nm,type) \
   STATIC_CONST char ksz##nm[] = #nm; \
   STIL type LREGP_get_##nm( lua_State *L           ) { return static_cast<type>(get_LuaRegistryPtr( L, ksz##nm  )); } \
   STIL void LREGP_set_##nm( lua_State *L, type val ) {        set_LuaRegistryPtr( L, ksz##nm, reinterpret_cast<void *>(val) ); }

//---------------------------

//######################################################################################################################
//
// Historical notes on Lua GARBAGE COLLECTION
//
//    First-implementation (20060721) policy was to call "lua_gc(LUA_GCCOLLECT)"
//    inside LuaCallCleanup() DTOR.  This was observed to have the following
//    consequence:
//
//    1.  Editor invokes Lua EdFxn edhelp.
//
//    2.  edhelp calls C code l_FBUF_function_new (Lua FBUF ctor), passing a
//        string parameter (the filename).  In this case, the string parameter
//        is a Lua string expression, not a Lua variable (so there are probably
//        NO references to the string parameter itself).
//
//    3.  As part of filename resolution, l_FBUF_function_new calls
//        CompletelyExpandFName_wEnvVars, which calls
//        LuaCtxt_Edit::ExpandEnvVarsOk, a wrapper around the Lua function
//        StrExpandEnvVars.
//
//    4.  At the end of its execution, LuaCtxt_Edit::ExpandEnvVarsOk, being a
//        good wrapper function, destroys an instance of LuaCallCleanup, which
//        call_ED_ lua_gc(LUA_GCCOLLECT), which, among other things, _typically_
//        blows away the Lua string parameter to l_FBUF_function_new, resulting
//        in an otherwise "good" C pointer in l_FBUF_function_new pointing to junk.
//
//    Two apparent solutions:
//
//    1.  Strdup() ALL string function parameters coming from Lua.
//
//    2.  Change my GC invocation to be "less aggressive" so Lua values aren't
//        GC'd until they are no longer used by C code.
//
//    2a. Don't GC in the execution thread; GC in the Idle Handler thread (which
//        is really a coroutine therefore mututally exclusive vs. the execution
//        thread).
//
//    #1 is undesirable for performance reasons (Strdup'ing every string
//    parameter takes a lot of time) and correctness reasons (can you say "high
//    liklihood of memory leak"?).  I suppose I could always use ALLOCA_STRDUP,
//    which causes even MORE object-code-size growth than using Strdup...
//
//    #2 is a challenge to do right: the editor core can call Lua functions (EX:
//    LuaCtxt_Edit::ExpandEnvVarsOk) directly (that is, with no underlaying Lua
//    invocations pending) at any time.  If GC is not done at those times out of
//    fear of the case above, then garbage will collect over an indefinite
//    period, which is undesirable (although one could say: "RAM is cheap, who
//    cares?").
//
//    #2a: If this can be done with a guarantee that is no concurrency hazard
//    [which as noted in the updated description of 2a above, _is_ the case], it
//    may be ideal.  Historical caveat: If not, this will be a problem anyway
//    since it's likely that _eventually_ Lua code will be called from the Idle
//    thread.  [20140123 I don't think this is a concern since the Idle thread
//    does not preempt itself]
//
//-----------------
//
// update 20060721 Lua appears to make COPIES of function parameter values that
//                 are passed to C (which are immediately susceptible to be
//                 GC'd) even if at the source level, the parameter is (the
//                 value of) a persistent variable.
//
//                 It strikes me that, if I DO NO GC AT ALL (which I believe
//                 means I'll be using the background GC built into the Lua core
//                 itself (the refman implies this exists), it MAY decide to GC
//                 at a "inopportune time", causing the same sorts of problems
//                 described above.
//
//-----------------
//
// update 20060803
//                 I was pushed into adopting solution 2a (above).  The story:
//                 TAWK has been "banished" (totally removed).  This means I'm
//                 now using the Lua-version of tags lookup (and a Lua version
//                 of everything else that TAWK used to do) full-time.  So I'm
//                 now regularly (at work) loading much larger tags-databases
//                 into Lua.  And I also have an event-handler for GETFOCUS
//                 written in Lua.  Meaning I was calling lua_gc(LUA_GCCOLLECT)
//                 after the Lua event-handler hooking occurred.  What I've
//                 inferred is that every call to lua_gc(LUA_GCCOLLECT) causes
//                 the examination of every (heap) object that has been
//                 allocated.  With a large tags database (3-deep nested table),
//                 this is surely many thousands of objects.  Which resulted in
//                 a very noticable slowdown in the rate of file switching when
//                 I hold down the 'setfile' command's key (my famous
//                 "blink-compare" technique).  With some experimentation, this
//                 delay was attributed to call to lua_gc(LUA_GCCOLLECT).  So I
//                 decided to try the proper solution: LuaIdleGC(), which calls
//                 lua_gc(LUA_GCSTEP) is the result.  Varying the 'luagcstep'
//                 switch value, it's fun to watch the Lua heapsize shrink back
//                 once I stop hitting keys.  In terms of robustness: so far so
//                 good.  I expected no problems, and there have been none so
//                 far...
//
class LuaCallCleanup_ {
   lua_State *d_L;
public:
   LuaCallCleanup_( lua_State *L, const char *fxn ) : d_L(L) {
      const auto els( lua_gettop( d_L ) ); if( els ) { DBG( "%s @ entry stack contains %d (%s)", __func__, els, fxn ); LDS( __func__, L ); }
      }
   ~LuaCallCleanup_() {
      lua_settop( d_L, 0 );  // Lua refman: "If index is 0, then all stack elements are removed."
      }
   };

#define LuaCallCleanup( L )  LuaCallCleanup_ lcc( L, __func__ )

//---------------------------------------------------------------------------------

GLOBAL_VAR lua_State **g_L[] = {  // in prep for more L's (for special purposes such as state-file write/read)
   &L_edit         ,
   &L_restore_save ,
   };                  // of course this (AAA) makes the pointers themselves (e.g. L_edit) _shared variables_

GLOBAL_VAR int g_iLuaGcStep = 10;
void LuaIdleGC() {
   if( g_iLuaGcStep ) {
      for( const auto &gl : g_L ) {
         if( *gl ) {                                  // AAA
            lua_gc( *gl, LUA_GCSTEP, g_iLuaGcStep );  // AAA
            }
         }
      }
   }

//---------------------------------------------------------------------------------

STATIC_VAR size_t s_LuaHeapSize;

size_t LuaHeapSize() { return s_LuaHeapSize; }

enum { DBG_LUA_ALLOC=0 };

STATIC_FXN void *l_alloc( void *ud, void *ptr, size_t osize, size_t nsize ) {
   static_cast<void>(ud); // mask 'not used' warning
   if( nsize == osize ) { // yes this happens ?!
      return ptr;
      }
   const auto delta( nsize - osize );
   s_LuaHeapSize += delta;
   if( nsize == 0 ) {
      DBG_LUA_ALLOC && ptr && DBG( "%s %" PR_SIZET " free %5" PR_SIZET " (%5" PR_SIZET "/%5" PR_SIZET ") P=            %p", __func__, s_LuaHeapSize, delta, osize, nsize, ptr );
      free( ptr );   // ANSI requires that free(nullptr) has no effect
      return nullptr;
      }
   // most of this conditional code is for debug/logging purposes
   const PVoid rv( realloc( ptr, nsize ) );  // ANSI requires that realloc(nullptr, size) == malloc(size)
   DBG_LUA_ALLOC && DBG( "%s %" PR_SIZET " %s %5" PR_SIZET " (%5" PR_SIZET "/%5" PR_SIZET ") P=%p -> %p", __func__, s_LuaHeapSize, ptr ? "real" : "new ", delta, osize, nsize, ptr, rv );
   return rv;
   }

// reliable reading of Lua global variables (including '.'-separated table expressions) from C
//
STIL bool getTblVal( lua_State *L, PCChar key, int ix ) { 0 && DBG( "%s indexing '%s'", __func__, key );
//                                                        [-o, +p, x]
//      o: how many elements the function pops from the stack
//      p: how many elements the function pushes onto the stack
//      x: tells whether the function may throw errors:
//         '-' means the function never throws any error;
//         'm' means the function may throw an error only due to not enough memory;
//         'e' means the function may throw other kinds of errors;
//         'v' means the function may throw an error on purpose
//
   const auto gettingGlobal( ix==0 );
   // unfortunately there is not a lua_getfield-like API which takes a stref-like, thus we have to pass ASCIZ here
   lua_getfield( L, gettingGlobal ? LUA_GLOBALSINDEX : -1, key );  // [-0, +1, e]  if field key is not defined
   if( !gettingGlobal ) {  // if not reading global (therefore lua_getfield of table [formerly] on top of stack)
      lua_remove(L, -2);   // pop table [formerly] on top of stack    [-1, +0, e]  which (its field having been read) is no longer needed
      }
   return true;
   }

STATIC_FXN bool gotTblVal( lua_State *L, PCChar pcRvalNm ) { enum { DB=0 };
   // because there is not a lua_getfield-like API which takes a stref-like, we must
   // COPY pcRvalNm so we can poke NULs into it to form sequence of ASCIZ's
   ALLOCA_STRDUP( rvNm, rvlen, pcRvalNm, Strlen( pcRvalNm ) );
   PCChar name[ 20 ] = { rvNm };
   auto depth(1);
   for( PChar pc(rvNm); pc < rvNm + rvlen; ++pc ) {
      if( *pc == '.' ) {
         *pc = '\0';
         if( !(depth < ELEMENTS(name)) ) {           DB && DBG( "%s MAX DEPTH (%" PR_SIZET ") exceeded: %s", __func__, ELEMENTS(name), pcRvalNm );
            return false;
            }
         name[depth++] = pc+1;
         }
      }                                              DB && DBG( "%s depth = %d", __func__, depth );
   for( auto ix(0); ix < depth; ++ix ) {             DB && DBG(  "%s ix = %d", __func__, ix );
      if( !getTblVal( L, name[ix], ix ) ) {
         return false;
         }
      if( ix < depth-1 && !lua_istable( L, -1 ) ) {  DB && DBG( "%s field '%s' is not a table in '%s'", __func__, name[ix], pcRvalNm );
         return false;
         }
      }                                           // DB && DBG( "%s ix(out) = %d", __func__, ix );
   return true; // *** caller is responsible for converting TOS to appropriate C value ***
   }

STATIC_FXN PChar CopyLuaString( PChar dest, size_t sizeof_dest, lua_State *L, int stackLevel ) {
   if( sizeof_dest > 0 ) {
      dest[0] = '\0';
      const auto pSrc( lua_tostring( L, -1 ) ); // converts number to string!
      if( pSrc ) {
         scpy( dest, sizeof_dest, pSrc );
         }
      }
   return dest;
   }

// returns false if any errors
STATIC_FXN bool LuaTblKeyExists( lua_State *L, PCChar tableDescr ) { 0 && DBG( "+%s '%s'?", __func__ , tableDescr );
   if( !L ) { return false; }
   LuaCallCleanup( L );
   const auto rv( gotTblVal( L, tableDescr ) && !lua_isnil(L,-1) );  0 && DBG( "-%s '%s' %c", __func__ , tableDescr, rv?'t':'f' );
   return rv;
   }

// returns nullptr if any errors
STATIC_FXN PChar LuaTbl2S0( lua_State *L, PChar dest, size_t sizeof_dest, PCChar tableDescr ) { 0 && DBG( "+%s '%s'?", __func__ , tableDescr );
   if( !L ) { return nullptr; }
   LuaCallCleanup( L );
   if( !gotTblVal( L, tableDescr ) ) { return nullptr; }
   return CopyLuaString( dest, sizeof_dest, L, -1 );
   }

enum { DBLUATBRD=0 };
// returns empty string if any errors
STATIC_FXN PChar LuaTbl2S( lua_State *L, PChar dest, size_t sizeof_dest, PCChar tableDescr, PCChar pszDflt ) {
   if( sizeof_dest > 0 ) {
      dest[0] = '\0';
      auto rv( LuaTbl2S0( L, dest, sizeof_dest, tableDescr ) );
      if( !rv ) {
         if( pszDflt ) {
            scpy( dest, sizeof_dest, pszDflt );                   DBLUATBRD && DBG( "%s   '%s' -> '%s' (dflt)"     , __func__ , tableDescr, dest );
            }
         else {                                                   DBLUATBRD && DBG( "%s   '%s' -> '%s' (no-dflt)"  , __func__ , tableDescr, dest );
            }
         }
      else {                                                      DBLUATBRD && DBG( "%s   '%s' -> '%s' (from Lua)" , __func__ , tableDescr, dest );
         }
      }
   else {                                                         DBLUATBRD && DBG( "%s   '%s' -> '' (empty-dest)" , __func__ , tableDescr );
      }
   return dest;
   }

// returns empty string if any errors
PChar LuaCtxt_Edit::Tbl2S( PChar dest, size_t sizeof_dest, PCChar tableDescr, PCChar pszDflt ) { return LuaTbl2S( L_edit, dest, sizeof_dest, tableDescr, pszDflt ); }

STATIC_FXN int lua_atpanic_handler( lua_State *L ) {
   linebuf msg;
   CopyLuaString( BSOB(msg), L, -1 );
   DBG( "########################################## %s called ##########################################", __func__ );
   DBG( "###  %s", msg );
   DBG( "########################################## %s called ##########################################", __func__ );
   SW_BP;
   exit(1);
   }

STIL void DeleteAllLuaCmds() { CmdIdxRmvCmdsByFunction( fn_runLua() ); }

STATIC_FXN void goto_file_line( PFBUF pFBuf, LINE yLine ) {
   pFBuf->PutFocusOn();
   g_CurView()->MoveCursor( yLine, 0 );
   fExecute( "begline" );
   }

// stolen from lua-5.1/src/lua.c
STATIC_FXN int traceback (lua_State *L) {
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

// stolen from lua-5.1/src/lua.c, except nres param to lua_pcall is passed directly
STATIC_FXN int lh_pcall_inout (lua_State *L, int narg, int nres) { enum { DB=0 };
  const auto top( lua_gettop(L) );  /* function index */
  const auto base( top - narg );  /* function index */
                                                                  DB && DBG(   "lh_pcall_inout(%d/%d,%d,%d)", narg, top, nres, base );
                                                                  DB && LDS( "+ lh_pcall_inout:pre-ins-traceback-fxn", L );
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
//signal(SIGINT, laction);
                                                                  DB && LDS( "+ lh_pcall_inout:lua_pcall", L );
  const auto pcall_rv( lua_pcall(L, narg, nres, base) );          DB && LDS( "- lh_pcall_inout:lua_pcall", L );

//signal(SIGINT, SIG_DFL);
  lua_remove(L, base);  /* remove traceback function */           DB && LDS( "+ lh_pcall_inout:post-rmv-traceback-fxn", L );
  /* force a complete garbage collection in case of errors */
  if (pcall_rv != 0) { lua_gc(L, LUA_GCCOLLECT, 0); }
  return pcall_rv;
}

STATIC_FXN bool init_lua_ok( lua_State **pL, void (*cleanup)(lua_State *L)=nullptr, void (*openlibs)(lua_State *L)=nullptr ); // forward

STATIC_FXN void lh_handle_pcall_err( lua_State *L, bool fCompileErr=false ) { // c:\klg\sn\k\util.lua:107: variable 'at' is not declared
   linebuf msg;
   CopyLuaString( BSOB(msg), L, -1 );
   // if( fCompileErr )
   //    {
   //    init_lua_ok( &L );  // shutdown Lua so that core->Lua calls made below will short-circuit vs. causing a Lua panic
   //    Assert( L == 0 );
   //    L = 0;
   //    }

   DBG( "L=%p, LuaMsg='%s'", L, msg );
   const auto tp( fCompileErr?"compile ":"run" );
   // STATIC_CONST char rtErrTmplt[] = "*** Lua %stime error";
   //Msg( rtErrTmplt, tp, msg );
   s_pFbufLuaLog->PutLastLineRaw( "" );
   const auto yMsgStart( s_pFbufLuaLog->LastLine()+1 );
   s_pFbufLuaLog->FmtLastLine( "*** Lua %stime error\n%s", tp, msg );
#if 1
   s_pFbufLuaLog->PutLastLineRaw( "" );
   const auto pv( s_pFbufLuaLog->PutFocusOn() );
   pv->MoveCursor( yMsgStart, 0 );
#else
   const PCChar pll = strrchr( msg, '\n' );
   PCChar pMsg = pll && (*(pll+1) == '\t') && *(pll+2) ? pll+2 : msg;  // if Lua's emsg is multiline, the last line has a leading tab to skip
   DBG( "pMsg='%s'", pMsg );
   // For simple Regex string searches (vs. search-thru-file-until-next-match) ops, use Regex_Compile + CompiledRegex::Match
   std::vector<stref> cs( 4 );
   {
   auto pre( Regex_Compile( "^(.*):(\\d+): (.*)$", true ) );
   if( !pre ) {
      Msg( "%s had regex COMPILE error", __func__ );
      return;
      }
   int mc;
   auto pmatch( pre->Match( 0, pMsg, Strlen(pMsg), &mc, STR_HAS_BOL_AND_EOL, &cs ) );
   Delete0( pre );
   if( !pmatch ) {
      Msg( "%s had regex match error", __func__ );
      return;
      }
   0 && DBG( "%d captures!", cs.Count() );
   // for( int ix=0; ix < cs.Count(); ++ix )
   //    DBG( "cs%d='%s'", ix, cs.Str(ix) );
   if( cs.Count() != 4 ) {
      Msg( "%s had regexcapture-count error", __func__ );
      return;
      }
   }
   PChar endptr;
   const auto yLine( strtoul( cs.Str(2), &endptr, 10 ) - 1 );
   auto pFBuf( OpenFileNotDir_NoCreate( cs.Str(1) ) );
   if( !pFBuf ) {
      return;
      }
   goto_file_line( pFBuf, yLine );
   PCChar emsg( cs.Str(3) );
   // if possible, display a truncated msg, so user doesn't have to mentally
   // discard the file location info which we've just navigated to for his
   // convenience...
   //
   Msg( rtErrTmplt, tp, emsg );
   s_pFbufLuaLog->FmtLastLine( rtErrTmplt, tp, emsg );
#endif
   }

LREGI(EvtHandlerEnabled)
LREGP(cleanup,void *)

STATIC_FXN void call_EventHandler( lua_State *L, PCChar eventName ) { enum { DB=0 };
   if( !L || LREGI_get_EvtHandlerEnabled( L ) < 1 ) {
      return;
      }
   DBG_LUA_ALLOC && DBG( "%s: heapChange ...", __func__ );
   const size_t s_LuaHeapBytesAtStart( LuaHeapSize() );
   LuaCallCleanup( L );
   if( lh_push_global_function( L, "CallEventHandlers" ) ) {
      return;
      }
   const auto failed( lh_pcall_inout( L, lh_push( L, eventName ), 0 ) );  DB && DBG( "C calling CallEventHandlers(%s)", eventName );
   if( failed ) {
      LREGI_set_EvtHandlerEnabled( L, 0 );         // don't want avalanche of errors
      lh_handle_pcall_err( L );
      }
   }

void LuaCtxt_ALL::call_EventHandler( PCChar eventName ) {
   0 && DBG( "L_edit=%p, L_restore_save=%p", L_edit, L_restore_save );
   call_EventHandler( L_edit        , eventName );
   call_EventHandler( L_restore_save, eventName );
   }

bool ARG::ExecLuaFxn() { enum { DB=0 };
   // Using ARG::CmdName(), we discover the name of the fxn being invoked.
   // We will call the Lua function of the same name.  If the Lua entity of
   // that name is a table rather than a function, we will attempt to call
   // table.d_argType() (e.g.  function.TEXTARG()).  If table.d_argType() is
   // not found, we will attempt to call table.ANYARG() (e.g. function.ANYARG()).
   //
   // 20060309 klg
   //
   const auto cmdName( CmdName() );                                              DB && DBG( "%s: '%s'", __func__, cmdName );
   const auto L( L_edit );
   LuaCallCleanup( L );
   // table=GetEdFxn_FROM_C(cmdName)    -- 20110216 kgoodwin replaced old scheme which wrote all cmd tables into k.lua._G (!!!)
   // GetEdFxn_FROM_C is implemented in k.luaedit
   STATIC_CONST auto s_edfx_lookup = "GetEdFxn_FROM_C";
   if( lh_push_global_function( L, s_edfx_lookup ) ) {
      return false;
      }
   {
   const auto failed( lh_pcall_inout( L, lh_push( L, cmdName ), 1 ) );
   if( failed ) {                                                                DB && DBG( "%s: docall GetEdFxn_FROM_C(%s) failed", __func__, cmdName );
      return Msg( "docall GetEdFxn_FROM_C(%s) failed", cmdName );
      }
   }
   const auto funcIsTable( lua_istable( L, -1 ) );                               DB && DBG( "%s: '%s' %d", __func__, "funcIsTable", funcIsTable );
   auto argTypeNm( ArgTypeName() );                                              DB && DBG( "%s: '%s', ARG=%s, 0x%X", __func__, cmdName, argTypeNm, d_argType );
   if( funcIsTable ) { /* support Lua decomposition, e.g. edfxn.BOXARG */        DB && DBG( "%s: '%s' is table", __func__, cmdName );
      lua_getfield(L, -1, argTypeNm);
      if( !lua_isfunction( L, -1 ) ) { // edfxn.ANYARG is last chance/catchall
         lua_pop( L, 1 );
         STATIC_CONST auto kszAnyarg = "ANYARG";
         lua_getfield( L, -1, kszAnyarg );
         if( lua_isfunction( L, -1 ) ) {
            argTypeNm = kszAnyarg;
            }
         }
      }
   if( !lua_isfunction( L, -1 ) ) {
      if( funcIsTable ) { return Msg( "Lua CMD %s.%s is not a function", cmdName, argTypeNm ); }
      else              { return Msg( "Lua CMD %s is not a function", cmdName );               }
      }
   // note that only a SUBSET of ARG type bits can be set here, leading to
   // non-obvious Lua EdFxn mapping of attr to function-key (e.g.  CURSORFUNC &
   // MACROFUNC both map to NOARG functions)
   //
   // This is documented where NOARG is defined, in column "Actually can be set
   // in ARG::Abc?": there are 6 ARG types marked "true".  Also, ArgTypeName()
   // can only return the names of these 6 ARG types.
   //
   // 20091119 kgoodwin
   //
   const auto actualArg( ActualArgType() );
   lua_createtable(L, 0, 12);  // ref os_date; 12 = number of fields
   switch( actualArg ) {
      case STREAMARG : return BadArg();  // not yet supported
      default        : return BadArg();
      case LINEARG   : setfield( L, "minY", 1+d_linearg.yMin );
                       setfield( L, "maxY", 1+d_linearg.yMax );
                       setfield( L, "minX", 1+0              );
                       setfield( L, "maxX",   COL_MAX        );
                       break;
      case BOXARG    : setfield( L, "minY", 1+d_boxarg.flMin.lin );
                       setfield( L, "maxY", 1+d_boxarg.flMax.lin );
                       setfield( L, "minX", 1+d_boxarg.flMin.col );
                       setfield( L, "maxX", 1+d_boxarg.flMax.col );
                       break;
      case TEXTARG   : setfield( L, "text"   , d_textarg.pText ); DB && DBG( "TEXTARG = %s", d_textarg.pText );
                       ATTR_FALLTHRU;
      case NOARG     :
      case NULLARG   :{const int xAll( 1+g_CursorCol () );
                       const int yAll( 1+g_CursorLine() );
                       setfield( L, "cursorX", xAll );
                       setfield( L, "minX"   , xAll );
                       setfield( L, "maxX"   , xAll );
                       setfield( L, "cursorY", yAll );
                       setfield( L, "minY"   , yAll );
                       setfield( L, "maxY"   , yAll );
                      }break;
      }                                                                          DB && DBG( ".%s = %d", "type", actualArg );
   setfield( L, "argCount", d_cArg );
   setfield( L, "type"    , actualArg );
   l_construct_FBUF( L, g_CurFBuf() );  lua_setfield( L, -2, "fbuf" );
   l_View_function_cur( L );            lua_setfield( L, -2, "view" );
   lua_pushboolean( L, d_fMeta );       lua_setfield( L, -2, "fMeta" );          DB && DBG( ".%s = %s", "fMeta", d_fMeta?"true":"false" );
   const auto failed( lh_pcall_inout( L, 1, 1 ) );
   if( failed ) {
      UnhideCursor();
      lh_handle_pcall_err( L );
      return false;
      }
   return LUA_TOBOOLEAN( L, -1 );
   }

//######################################################################################################################

STATIC_FXN void l_hook_handler( lua_State *L, lua_Debug *ar ) {
   if( ExecutionHaltRequested() ) {
      lua_pushstring( L, "Asynchronous ExecutionHaltRequested" );
      lua_error( L );
      }
   }

void LuaClose() {
   for( const auto &gl : g_L ) {
      if( *gl ) {
         init_lua_ok( gl );
         }
      }
   }

STATIC_FXN bool init_lua_ok( lua_State **pL, void (*cleanup)(lua_State *L), void (*openlibs)(lua_State *L) ) {
   if( *pL ) { 0 && DBG( "#######################  CLOSING current Lua session  ###################" );
      if( !cleanup ) { cleanup = (void (*)(lua_State *L))LREGP_get_cleanup( *pL ); }
      if(  cleanup ) { cleanup( *pL ); }
      lua_close( *pL );
      *pL = nullptr;
      }
   if( !openlibs ) {
      return true;
      }
   s_pFbufLuaLog->MakeEmpty();
   s_pFbufLuaLog->MoveCursorToBofAllViews();
   0 && DBG( "%s+ *(%p)=init'ing", __func__, pL );
   { // NB: each Lua 5.1.2 lua_newstate consumes 2.5KB!  20070724 kgoodwin
    const auto luaHeapBytesAtStart( LuaHeapSize() );
    *pL = lua_newstate( l_alloc, nullptr );
    const auto heapChange( LuaHeapSize() - luaHeapBytesAtStart );
    if( !*pL ) { DBG( "%s- lua_newstate FAILED", __func__ );
       return false;
       }
    DBG_LUA_ALLOC && DBG( "%s lua_newstate (%" PR_SIZET " bytes)  ******************************************", __func__, heapChange );
   }
   {
   lua_getfield( *pL, LUA_REGISTRYINDEX, "schickelgruber-nickel" );
   Assert( lua_isnil( *pL, -1 ) );
   lua_pop( *pL, 1 );
   }
   lua_atpanic( *pL, lua_atpanic_handler );
   lua_sethook( *pL, l_hook_handler, LUA_MASKLINE | LUA_MASKCOUNT, INT_MAX );
   LuaCallCleanup( *pL );
   openlibs( *pL );
   // override package.path so required Lua code is only looked for in sm dir as editor exe
   // (I used to setenv("LUA_PATH") but that impacted child processes that ran Lua.exe)
   lua_getglobal( *pL, "package" );  Assert( lua_istable( *pL, -1 ) );  // NB: package table does not exist until l_OpenStdLibs() has been called!
   setfield( *pL, "path", FmtStr<_MAX_PATH>( "%s?.lua", getenv( "KINIT" ) ) );
   LREGP_set_cleanup( *pL, reinterpret_cast<void *>(cleanup) );
   0 && DBG( "%s- *(%p)=%p", __func__, pL, *pL );
   return true;
   }

enum { FDBG=0 };

STATIC_FXN bool loadLuaFileFailed( lua_State **pL, PCChar fnm ) {
   FDBG && DBG( "%s+1 L=%p fnm='%s'",  __func__, *pL, fnm );
   auto fLoadOK(false);
   const auto luaRC( luaL_loadfile( *pL, fnm ) );
   FDBG && DBG( "%s+2 L=%p luaRC=%u",  __func__, *pL, luaRC );
   switch( luaRC ) {
      case LUA_ERRFILE:    DBG( "LUA_ERRFILE"   ); lh_handle_pcall_err( *pL, true ); break;
      case LUA_ERRSYNTAX:  DBG( "LUA_ERRSYNTAX" ); lh_handle_pcall_err( *pL, true ); break;
      case LUA_ERRMEM:     Msg( "Lua memory error while loading '%s'"     , fnm );  break;
      default:             Msg( "Unknown Lua error %d loading '%s'", luaRC, fnm );  break;
      case 0:
           FDBG && LDS( "LoadLuaFileFailed post-load/pre-pcall", *pL );
           {
           const auto failed( lh_pcall_inout( *pL, 0, 0 ) );
           if( failed ) {
              FDBG && DBG( "%s L=%p docall() Failed",  __func__, *pL );
              //
              // NB!  The above docall() runtime includes the COMPILE phase
              //      of any modules require()'d by 'fnm'.  As such, even if
              //      we get here, we could be facing a compile error, so we call
              //      lh_handle_pcall_err( , true ) so the L gets shut down
              //      ASAP, preventing a double fault which causes a Lua panic.
              //
              //      20060922 klg
              //
              lh_handle_pcall_err( *pL, true );
              }
           else {
           #if 0
           // const char tnm[] = "_VERSION";
           // const char tnm[] = "package.loaded.string.gmatch";
              const char tnm[] = "dp.help";
              linebuf lbutt;
              DBG( "%s = '%s'", tnm, LuaTbl2S( *pL, BSOB(lbutt), tnm, "" ) );
           #endif
              fLoadOK = true;
              }
           }
           break;
      }
   FDBG && DBG( "%s+3 L=%p fLoadOK=%c",  __func__, *pL, fLoadOK?'t':'f' );
   if( !fLoadOK ) { DBG( "%s failed", __func__ );
      init_lua_ok( pL );
      }
   FDBG && DBG( "%s- L=%p fLoadOK=%c",  __func__, *pL, fLoadOK?'t':'f' );
   return fLoadOK;
   }

STATIC_FXN bool LuaCtxt_InitOk(
     PCChar filename
   , char **dupdFnm
   , lua_State **pL
   , void (*cleanup)  (lua_State *L)
   , void (*openlibs) (lua_State *L)
   , void (*post_load)(lua_State *L)
   ) {
   if( dupdFnm ) {
      Free0( *dupdFnm );
      *dupdFnm = Strdup( filename );
      }
   if(   init_lua_ok( pL, cleanup, openlibs )
      && loadLuaFileFailed( pL, filename )
     ) {
      post_load( *pL ); // Lua environment-loaded hook-outs
      }
   return *pL != nullptr;
   }

STATIC_FXN void L_restore_save_post_load( lua_State *L ) { // Lua environment-loaded hook-outs
   }

STATIC_FXN void L_std_openlibs( lua_State *L ) {
   l_OpenStdLibs( L );
// l_register_EdLib( L ); //BUGBUG remove or trim down?
   }

STATIC_VAR PChar psrc_LuaCtxt_restore_save;
bool LuaCtxt_State::InitOk( PCChar filename ) {
   return LuaCtxt_InitOk(
         filename
      , &psrc_LuaCtxt_restore_save
      , &L_restore_save
      ,  nullptr
      ,  L_std_openlibs
      ,  L_restore_save_post_load
      );
   }

STATIC_FXN void L_edit_post_load( lua_State *L ) { // Lua environment-loaded hook-outs
   LREGI_set_EvtHandlerEnabled( L, 1 );
   Reread_FTypeSettings();
   }

STATIC_FXN void L_edit_openlibs( lua_State *L ) {
   l_OpenStdLibs( L );
   l_register_EdLib( L );
   }

STATIC_FXN void L_edit_cleanup( lua_State *L ) {
   DeleteAllLuaCmds();
   }

STATIC_VAR PChar psrc_LuaCtxt_Edit;
bool LuaCtxt_Edit::InitOk( PCChar filename ) {
   return LuaCtxt_InitOk(
         filename
      , &psrc_LuaCtxt_Edit
      , &L_edit
      ,  L_edit_cleanup
      ,  L_edit_openlibs
      ,  L_edit_post_load
      );
   }

bool ARG::lua() {
   if( d_cArg > 0 ) {
      s_pFbufLuaLog->PutFocusOn();
      return true;
      }
   WriteAllDirtyFBufs();  // In case any required Lua files are dirty. Pseudofiles are not saved.
   if( psrc_LuaCtxt_Edit ) {
      const auto pFBuf( OpenFileNotDir_NoCreate( psrc_LuaCtxt_Edit ) );
      if( !pFBuf ) {
         return Msg( "'%s' not found?", psrc_LuaCtxt_Edit );
         }
      if( g_CurFBuf() == pFBuf ) {
         return LuaCtxt_Edit::InitOk( pFBuf->Name() );
         }
      pFBuf->PutFocusOn();
      return true;
      }
   return false;
   }

bool LuaCtxt_Edit::ExecutedURL( PCChar strToExecute, bool fNullarg ) {
   constexpr bool rv_fail = false;
   auto L( L_edit ); if( !L ) { return rv_fail; }
   lua_settop( L, 0 );      // clear the stack
   LuaCallCleanup( L ); // clear the stack on function return
   if( lh_push_global_function( L, "ExecutedURL" ) ) {
      return rv_fail;
      }
   if( lh_pcall_inout( L, lh_push( L, strToExecute ) + lh_push( L, fNullarg ), 1 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
      return rv_fail;
      }
   return LUA_TOBOOLEAN( L, -1 );
   }

// intf into Lua locn-list subsys
void LuaCtxt_Edit::LocnListInsertCursor() {
   auto L( L_edit ); if( !L ) { return; }
   lua_settop( L, 0 );      // clear the stack
   LuaCallCleanup( L ); // clear the stack on function return
   if( lh_push_global_function( L, "LocnListInsertCursor" ) ) {
      return;
      }
   if( lh_pcall_inout( L, 0, 0 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
      }
   }

bool LuaCtxt_Edit::nextmsg_setbufnm     ( PCChar src )  {
   constexpr bool rv_fail = false;
   auto L( L_edit ); if( !L ) { return rv_fail; }
   lua_settop( L, 0 );      // clear the stack
   LuaCallCleanup( L ); // clear the stack on function return
   if( lh_push_global_function( L, "nextmsg_setbufnm_FROM_C" ) ) {
      return rv_fail;
      }
   if( lh_pcall_inout( L, lh_push( L, src ), 0 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
      return rv_fail;
      }
   return true;
   }

bool LuaCtxt_Edit::nextmsg_newsection_ok( PCChar src )  {
   constexpr bool rv_fail = false;
   auto L( L_edit );
   if( !L ) { return rv_fail; }
   lua_settop( L, 0 );      // clear the stack
   LuaCallCleanup( L ); // clear the stack on function return
   if( lh_push_global_function( L, "nextmsg_newsection_FROM_C" ) ) {
      return rv_fail;
      }
   if( lh_pcall_inout( L, lh_push( L, src ), 0 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
      return rv_fail;
      }
   return true;
   }

bool Lua_ConfirmYes( PCChar prompt ) {
   constexpr bool rv_fail = false;
   auto L( L_edit ); if( !L ) { return rv_fail; }
   lua_settop( L, 0 );  // clear the stack
   LuaCallCleanup( L ); // clear the stack on function return
   if( lh_push_global_function( L, "MenuConfirm" ) ) {
      return rv_fail;
      }
   if( lh_pcall_inout( L, lh_push( L, prompt ), 1 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
      return rv_fail;
      }
   return LUA_TOBOOLEAN( L, -1 );
   }

void FBOP::LuaSortLineRange( PFBUF fb, LINE y0, LINE y1, COL xLeft, COL xRight, bool fCaseIgnored, bool fOrdrAscending, bool fDupsKeep ) {
   // SW_BP;
   // callLuaOk( L_edit, "SortLineRange_from_C"
   //    , "Fiiiibbb>"
   //    , fb
   //    , y0 - 1
   //    , y1 - 1
   //    , xLeft
   //    , xRight
   //    , fCaseIgnored
   //    , fOrdrAscending
   //    , fDupsKeep
   //    );
   }

COL FBOP::GetSoftcrIndentLua( PFBUF fb, LINE yLine ) {
   constexpr COL rv_fail = -1;
   auto L( L_edit ); if( !L ) { return rv_fail; }
   lua_settop( L, 0 );      // clear the stack
   LuaCallCleanup( L ); // clear the stack on function return
   if( lh_push_global_function( L, "Softcr_col_from_C" ) ) {
      return rv_fail;
      }
   if( lh_pcall_inout( L, lh_push( L, fb, yLine ), 1 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
      return rv_fail;
      }
   COL rv( lua_tointeger( L, -1 ) ); 0 && DBG( "%s =%d", __func__, rv-1 );
   return rv - 1;
   }

STIL bool Lua_S2S( lua_State *L, PCChar functionName, Path::str_t &inout, bool fClearInoutOnFail=true ) { enum { DB=0 };
   constexpr bool rv_fail = false;                                           DB && DBG( "%s+ %s %s", __func__, functionName, inout.c_str() );
   if( !L ) { return rv_fail; }
   lua_settop( L, 0 );      // clear the stack
   LuaCallCleanup( L ); // clear the stack on function return
   if( lh_push_global_function( L, functionName ) ) {
      if( fClearInoutOnFail ) {
         inout.clear();
         }                                                                   DB && DBG( "%s- %s FAIL-findfxn %s", __func__, functionName, inout.c_str() );
      return rv_fail;
      }
   if( lh_pcall_inout( L, lh_push( L, inout ), 1 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
      if( fClearInoutOnFail ) {
         inout.clear();
         }                                                                   DB && DBG( "%s- %s FAIL-pcall %s", __func__, functionName, inout.c_str() );
      return rv_fail;
      }
   lua_assigncppstring( L, -1, inout );                                      DB && DBG( "%s- %s %s", __func__, functionName, inout.c_str() );
   return true;
   }

// functionName may contain '.' (table dereferences)
STATIC_FXN bool LuaTblS2S( lua_State *L, PCChar functionName, Path::str_t &inout, bool fClearInoutOnFail=true ) { enum { DB=0 };
   constexpr bool rv_fail = false;                                           DB && DBG( "%s+ %s<-%s", __func__, functionName, inout.c_str() );
   if( !L ) { return rv_fail; }
   LuaCallCleanup( L );
   if( !gotTblVal( L, functionName ) ) {                                     DB && DBG( "%s- %s FAIL-gotTblVal %s", __func__, functionName, inout.c_str() );
      goto FAIL;
      }
   if( !lua_isfunction( L, -1 ) ) {                                          DB && DBG( "%s- %s FAIL-gotTblVal!=fxn %s", __func__, functionName, inout.c_str() );
      goto FAIL;
      }
   if( lh_pcall_inout( L, lh_push( L, inout ), 1 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
FAIL:
      if( fClearInoutOnFail ) {
         inout.clear();
         }                                                                   DB && DBG( "%s- %s FAIL-pcall %s", __func__, functionName, inout.c_str() );
      return rv_fail;
      }
   lua_assigncppstring( L, -1, inout );                                      DB && DBG( "%s- %s->%s", __func__, functionName, inout.c_str() );
   return true;
   }

bool LuaCtxt_Edit::ReadPseudoFileOk( PFBUF src ) {
   constexpr bool rv_fail = false;
   auto L( L_edit ); if( !L ) { return rv_fail; }
   lua_settop( L, 0 );      // clear the stack
   LuaCallCleanup( L ); // clear the stack on function return
   if( lh_push_global_function( L, "ReadPseudoFileOk_FROM_C" ) ) {
      return rv_fail;
      }
   if( lh_pcall_inout( L, lh_push( L, src ), 1 ) != 0 ) {  // do the call
      lh_handle_pcall_err( L );
      return rv_fail;
      }
   return LUA_TOBOOLEAN( L, -1 );
   }

bool  LuaCtxt_Edit::ExpandEnvVarsOk    ( Path::str_t &st ) { return Lua_S2S( L_edit, "StrExpandEnvVars"        , st ); }
bool  LuaCtxt_Edit::from_C_lookup_glock( std::string &st ) { return Lua_S2S( L_edit, "Lua_from_C_lookup_glock" , st ); }
bool  LuaCtxt_Edit::ShebangToFType     ( Path::str_t &st ) { return LuaTblS2S( L_edit, "filesettings.ShebangToFType", st, false ); }
bool  LuaCtxt_Edit::FnmToFType         ( Path::str_t &st ) { return LuaTblS2S( L_edit, "filesettings.FnmToFType", st, false ); }

// returns dfltVal if any errors
STATIC_FXN int LuaTbl2Int( lua_State *L, PCChar tableDescr, int dfltVal ) {
   if( !L ) {                          DBLUATBRD && DBG( "%s '%s' 0x%02X=%3d (dflt; no Lua ctxt)", __func__, tableDescr, dfltVal, dfltVal );
      return dfltVal;
      }
   LuaCallCleanup( L );
   auto rv(dfltVal);
   if(    gotTblVal( L, tableDescr )
       && (lua_isnumber( L, -1 ) || (( DBLUATBRD && DBG( "%s '%s' is not a number"       , __func__, tableDescr )), false ))
     ) {
      rv = lua_tointeger( L, -1 );     DBLUATBRD && DBG( "%s '%s' 0x%02X=%3d (from Lua)" , __func__, tableDescr, rv, rv );
      }
   else {                              DBLUATBRD && DBG( "%s '%s' 0x%02X=%3d (dflt)"     , __func__, tableDescr, rv, rv );
      }
   return rv;
   }

// returns dfltVal if any errors
int LuaCtxt_Edit::Tbl2Int( PCChar tableDescr, int dfltVal ) { return LuaTbl2Int( L_edit, tableDescr, dfltVal ); }
bool LuaCtxt_Edit::TblKeyExists( PCChar tableDescr ) { return LuaTblKeyExists( L_edit, tableDescr ); }


#pragma GCC diagnostic pop
