//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

//####  lh_ = my Lua Helper functions

#include "lua_intf_common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

int LDS( PCChar tag, lua_State *L ) {
   const auto stkEls( lua_gettop(L) );
   DBG( "+luaStack, %d els @ '%s'", stkEls, tag );
   for( auto ix(1); ix <= stkEls; ++ix ) {
      DBG( " luaStack[%d] is %s", ix, lua_typename( L, ix ) );
      }
   0 && DBG( "-luaStack Dump @ '%s'", tag );
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

#if USE_STATE_ELB
LREGP(xb,PXbuf)
PXbuf get_xb( lua_State *L ) { return LREGP_get_xb( L ); }
#endif


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
class LuaCallCleanup {
   lua_State *d_L;

   public:

   LuaCallCleanup( lua_State *L ) : d_L(L) {}
   ~LuaCallCleanup() {
      lua_settop( d_L, 0 );  // Lua refman: "If index is 0, then all stack elements are removed."
      }
   };

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

   if( nsize == osize )  // yes this happens ?!
      return ptr;

   const auto delta( nsize - osize );

   s_LuaHeapSize += delta;

   if( nsize == 0 ) {
      DBG_LUA_ALLOC && ptr && DBG( "%s %Iu free %5Iu (%5Iu/%5Iu) P=            %p", __func__, s_LuaHeapSize, delta, osize, nsize, ptr );
      free( ptr );   // ANSI requires that free(nullptr) has no effect
      return nullptr;
      }

   // most of this conditional code is for debug/logging purposes

   const PVoid rv( realloc( ptr, nsize ) );  // ANSI requires that realloc(nullptr, size) == malloc(size)
   DBG_LUA_ALLOC && DBG( "%s %Iu %s %5Iu (%5Iu/%5Iu) P=%p -> %p", __func__, s_LuaHeapSize, ptr ? "real" : "new ", delta, osize, nsize, ptr, rv );

   return rv;
   }


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

// stolen from lua-5.1/src/lua.c
STATIC_FXN int docall (lua_State *L, int narg, int clear) {
  auto base( lua_gettop(L) - narg );  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
//signal(SIGINT, laction);
  const auto status( lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base) );
//signal(SIGINT, SIG_DFL);
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}

STATIC_FXN bool init_lua_ok( lua_State **pL, void (*cleanup)(lua_State *L)=nullptr, void (*openlibs)(lua_State *L)=nullptr ); // forward

STATIC_FXN void l_handle_pcall_error( lua_State *L, bool fCompileErr=false ) { // c:\klg\sn\k\util.lua:107: variable 'at' is not declared
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
   s_pFbufLuaLog->PutLastLine( "" );
   const auto yMsgStart( s_pFbufLuaLog->LastLine()+1 );
   s_pFbufLuaLog->FmtLastLine( "*** Lua %stime error", tp );
   s_pFbufLuaLog->PutLastMultiline( msg );

#if 1

   s_pFbufLuaLog->PutLastLine( "" );

   const auto pv( s_pFbufLuaLog->PutFocusOn() );
   pv->MoveCursor( yMsgStart, 0 );

#else
   const PCChar pll = strrchr( msg, '\n' );
   PCChar pMsg = pll && (*(pll+1) == '\t') && *(pll+2) ? pll+2 : msg;  // if Lua's emsg is multiline, the last line has a leading tab to skip
   DBG( "pMsg='%s'", pMsg );

   // For simple Regex string searches (vs. search-thru-file-until-next-match) ops, use RegexCompile + Regex::Match

   CapturedStrings cs( 4 );
   {
   auto pre( RegexCompile( "^(.*):(\\d+): (.*)$", true ) );
   if( !pre ) {
      Msg( "%s had regex COMPILE error", __func__ );
      return;
      }
   int mc;
   auto pmatch( pre->Match( 0, pMsg, Strlen(pMsg), &mc, STR_HAS_BOL_AND_EOL, &cs ) );
   RegexDestroy( pre );
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
   if( !pFBuf )
      return;

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

STATIC_FXN void call_EventHandler( lua_State *L, PCChar eventName ) {
   if( !L )
      return;

   if( LREGI_get_EvtHandlerEnabled( L ) < 1 )
      return;

   DBG_LUA_ALLOC && DBG( "%s: heapChange ...", __func__ );

   const size_t s_LuaHeapBytesAtStart( LuaHeapSize() );

   LuaCallCleanup lcc( L );

   if( lh_getglobal_failed( L, "CallEventHandlers" ) || !lua_isfunction( L, -1 ) )
      return;

   lua_pushstring( L, eventName );  0 && DBG( "C calling CallEventHandlers(%s)", eventName );

   const auto failed( docall( L, 1, 1 ) );
   if( failed ) {
      LREGI_set_EvtHandlerEnabled( L, 0 );         // don't want avalanche of errors
      l_handle_pcall_error( L );
      }
   }

void LuaCtxt_ALL::call_EventHandler( PCChar eventName ) {
   0 && DBG( "L_edit=%p, L_restore_save=%p", L_edit, L_restore_save );
   call_EventHandler( L_edit        , eventName );
   call_EventHandler( L_restore_save, eventName );
   }


bool ARG::ExecLuaFxn() {
   // Using ARG::CmdName(), we can discover the name of the fxn being
   // invoked.  We will call the Lua function of the same name.  If the Lua
   // entity of that name is a table rather than a function, we will attempt to
   // call table.d_argType() (e.g.  function.TEXTARG()).  If table.d_argType() is
   // not found, we will attempt to call table.ANYARG() (e.g.
   // function.ANYARG()).
   //
   // 20060309 klg
   //
   const auto cmdName( CmdName() );
   0 && DBG( "%s: '%s'", __func__, cmdName );

   const auto L( L_edit );
   LuaCallCleanup lcc( L );

   // table=GetEdFxn_FROM_C(cmdName)    -- 20110216 kgoodwin replaced old scheme which wrote all cmd tables into k.lua._G (!!!)
   // GetEdFxn_FROM_C is implemented in k.luaedit
   if( lh_getglobal_failed( L, "GetEdFxn_FROM_C" ) || !lua_isfunction( L, -1 ) ) {
      0 && DBG( "%s: '%s' %d", __func__, "GetEdFxn_FROM_C", lua_isfunction( L, -1 ) );
      return Msg( "Lua does not implement GetEdFxn_FROM_C" );
      }
   lua_pushstring( L, cmdName );
   {
   const auto failed( docall( L, 1, 0 ) );
   if( failed ) {
      0 && DBG( "%s: docall GetEdFxn_FROM_C(%s) failed", __func__, cmdName );
      return Msg( "docall GetEdFxn_FROM_C(%s) failed"          , cmdName );
      }
   }

   const auto funcIsTable( lua_istable( L, -1 ) );
   0 && DBG( "%s: '%s' %d", __func__, "funcIsTable", funcIsTable );

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
   auto argTypeNm( ArgTypeName() );
   0 && DBG( "%s: '%s', ARG=%s, 0x%X", __func__, cmdName, argTypeNm, d_argType );
   if( funcIsTable ) { // support Lua decomposition edfxn.BOXARG
      0 && DBG( "%s: '%s' is table", __func__, cmdName );
      lua_getfield(L, -1, argTypeNm);
      if( !lua_isfunction( L, -1 ) ) { // edfxn.ANYARG is last chance/catchall
         lua_pop( L, 1 );
         STATIC_CONST char kszAnyarg[] = "ANYARG";
         lua_getfield( L, -1, kszAnyarg );
         if( lua_isfunction( L, -1 ) )
            argTypeNm = kszAnyarg;
         }
      }

   if( !lua_isfunction( L, -1 ) ) {
      if( funcIsTable )
         return Msg( "Lua CMD %s.%s is not a function", cmdName, argTypeNm );
      else
         return Msg( "Lua CMD %s is not a function", cmdName );
      }

   lua_createtable(L, 0, 12);  // ref os_date; 12 = number of fields
   const auto actualArg( d_argType & ACTUAL_ARGS );
   switch( actualArg ) {
      case STREAMARG : return BadArg();  // not yet supported
      default        : return BadArg();

      case LINEARG   : setfield( L, "minY", 1+d_linearg.yMin );
                       setfield( L, "maxY", 1+d_linearg.yMax );
                       setfield( L, "minX", 1+0              );
                       setfield( L, "maxX",   COL_MAX         );
                       break;

      case BOXARG    : setfield( L, "minY", 1+d_boxarg.flMin.lin );
                       setfield( L, "maxY", 1+d_boxarg.flMax.lin );
                       setfield( L, "minX", 1+d_boxarg.flMin.col );
                       setfield( L, "maxX", 1+d_boxarg.flMax.col );
                       break;

      case TEXTARG   : setfield( L, "text"   , d_textarg.pText ); //lint -fallthrough
      case NOARG     :                                            //lint -fallthrough
      case NULLARG   : {
                       const int xAll( 1+g_CursorCol () );
                       const int yAll( 1+g_CursorLine() );
                       setfield( L, "cursorX", xAll );
                       setfield( L, "minX"   , xAll );
                       setfield( L, "maxX"   , xAll );
                       setfield( L, "cursorY", yAll );
                       setfield( L, "minY"   , yAll );
                       setfield( L, "maxY"   , yAll );
                       }
                       break;
      }

   0 && DBG( ".%s = %d", "type", actualArg );
   setfield( L, "argCount", d_cArg );
   setfield( L, "type"    , actualArg );

   l_construct_FBUF( L, g_CurFBuf() );
   lua_setfield( L, -2, "fbuf" );

   l_View_function_cur( L );
   lua_setfield( L, -2, "view" );

   lua_pushboolean( L, d_fMeta );          0 && DBG( ".%s = %s", "fMeta", d_fMeta?"true":"false" );
   lua_setfield( L, -2, "fMeta" );

   {
   const auto failed( docall( L, 1, 0 ) );
   if( failed ) {
      UnhideCursor();
      l_handle_pcall_error( L );
      return false;
      }
   }

   auto rv(true); // default
   if(      lua_isboolean( L, -1 ) )  rv = lua_toboolean( L, -1 ) != 0;
   else if( lua_isnil    ( L, -1 ) )  rv = false;
   // else DBG( "Lua edfxn '%s' did not return a boolean!", cmdName );

   return rv;
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
   if( *pL ) { DBG( "#######################  CLOSING current Lua session  ###################" );
      if( !cleanup ) { cleanup = (void (*)(lua_State *L))LREGP_get_cleanup( *pL ); }
      if(  cleanup ) { cleanup( *pL ); }
      lua_close( *pL );
      *pL = nullptr;
      }

   if( !openlibs )
      return true;

   s_pFbufLuaLog->MakeEmpty();
   s_pFbufLuaLog->MoveCursorToBofAllViews();

   DBG( "%s+ *(%p)=init'ing", __func__, pL );
   { // NB: each Lua 5.1.2 lua_newstate consumes 2.5KB!  20070724 kgoodwin
    const auto luaHeapBytesAtStart( LuaHeapSize() );
    *pL = lua_newstate( l_alloc, nullptr );
    const auto heapChange( LuaHeapSize() - luaHeapBytesAtStart );
    if( !*pL ) { DBG( "%s- lua_newstate FAILED", __func__ );
       return false;
       }
    DBG_LUA_ALLOC && DBG( "%s lua_newstate (%Iu bytes)  ******************************************", __func__, heapChange );
   }

   {
   lua_getfield( *pL, LUA_REGISTRYINDEX, "schickelgruber-nickel" );
   Assert( lua_isnil( *pL, -1 ) );
   lua_pop( *pL, 1 );
   }

   lua_atpanic( *pL, lua_atpanic_handler );
   lua_sethook( *pL, l_hook_handler, LUA_MASKLINE | LUA_MASKCOUNT, INT_MAX );

   LuaCallCleanup lcc( *pL );

   openlibs( *pL );

   // override package.path so required Lua code is only looked for in sm dir as editor exe
   // (I used to setenv("LUA_PATH") but that hosed child processes that ran Lua.exe)

   lua_getglobal( *pL, "package" );  Assert( lua_istable( *pL, -1 ) );  // NB: package table does not exist until l_OpenStdLibs() has been called!
   setfield( *pL, "path", FmtStr<_MAX_PATH>( "%s?.lua", getenv( "KINIT" ) ) );

   LREGP_set_cleanup( *pL, reinterpret_cast<void *>(cleanup) );

   DBG( "%s- *(%p)=%p", __func__, pL, *pL );
   return true;
   }

enum { FDBG=0 };

STATIC_FXN bool loadLuaFileFailed( lua_State **pL, PCChar fnm ) {
   FDBG && DBG( "%s+1 L=%p fnm='%s'",  __func__, *pL, fnm );
   auto fLoadOK(false);
   const auto luaRC( luaL_loadfile( *pL, fnm ) );
   FDBG && DBG( "%s+2 L=%p luaRC=%u",  __func__, *pL, luaRC );
   switch( luaRC ) {
      case LUA_ERRFILE:    DBG( "LUA_ERRFILE"   ); l_handle_pcall_error( *pL, true ); break;
      case LUA_ERRSYNTAX:  DBG( "LUA_ERRSYNTAX" ); l_handle_pcall_error( *pL, true ); break;
      case LUA_ERRMEM:     Msg( "Lua memory error while loading '%s'"     , fnm );  break;
      default:             Msg( "Unknown Lua error %d loading '%s'", luaRC, fnm );  break;

      case 0:
           FDBG && LDS( "LoadLuaFileFailed post-load/pre-pcall", *pL );
           {
           const auto failed( docall( *pL, 0, 0 ) );
           if( failed ) {
              FDBG && DBG( "%s L=%p docall() Failed",  __func__, *pL );
              //
              // NB!  The above docall() runtime includes the COMPILE phase
              //      of any modules require()'d by 'fnm'.  As such, even if
              //      we get here, we could be facing a compile error, so we call
              //      l_handle_pcall_error( , true ) so the L gets shut down
              //      ASAP, preventing a double fault which causes a Lua panic.
              //
              //      20060922 klg
              //
              l_handle_pcall_error( *pL, true );
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

   if( !fLoadOK && *pL ) { DBG( "%s failed", __func__ );
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
   Reread_FileExtensionSettings();
 #if USE_STATE_ELB
   LREGP_set_xb( L, new Xbuf );
 #endif
   }
STATIC_FXN void L_edit_openlibs( lua_State *L ) {
   l_OpenStdLibs( L );
   l_register_EdLib( L );
   }
STATIC_FXN void L_edit_cleanup( lua_State *L ) {
   DeleteAllLuaCmds();
 #if USE_STATE_ELB
   delete LREGP_get_xb( L );
 #endif
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
      if( !pFBuf )
         return Msg( "'%s' not found?", psrc_LuaCtxt_Edit );

      if( g_CurFBuf() == pFBuf )
         return LuaCtxt_Edit::InitOk( pFBuf->Name() );

      pFBuf->PutFocusOn();
      return true;
      }
   return false;
   }


struct StrDest {
   PChar  d_pBuf;
   size_t d_bytes;

        StrDest( PChar pBuf, size_t bytes ) : d_pBuf( pBuf ), d_bytes( bytes ) {}
   void BindBuf( PChar pBuf, size_t bytes ) { d_pBuf= pBuf ;  d_bytes= bytes ;  }
   void CopyTo( PCChar src, size_t bytes ) {
      size_t actual = Min( d_bytes, bytes );
      memcpy( d_pBuf, src, actual );
      d_bytes = actual;
      }
   };  STD_TYPEDEFS(StrDest)


// straight from PIL2e (call_va)

STATIC_FXN bool vcallLuaOk( lua_State *L, const char *szFuncnm, const char *szSig, va_list vl=0 ) {
   if( !L ) return false;

   if( lh_getglobal_failed( L, szFuncnm ) || !lua_isfunction( L, -1 ) )
      return Msg( "%s: Lua symbol '%s' is NOT A FUNCTION", __func__, szFuncnm );

   // push arguments
   auto pszSigStart( szSig );
   int narg;
   for( narg=0 ; *szSig ; ++narg, ++szSig ) {
      switch( *szSig ) {
       default:   return Msg( "%s: invalid param-type[%Id] (%c)", szFuncnm, (szSig - pszSigStart), szSig[0] );
       case 'F':  l_construct_FBUF( L, va_arg(vl, PFBUF )                 );  break;
       case 'V':  l_construct_View( L, va_arg(vl, PView )                 );  break;
       case 'd':  lua_pushnumber(   L, va_arg(vl, double)                 );  break;
       case 'i':  lua_pushinteger(  L, va_arg(vl, int   )                 );  break;
       case 'b':  lua_pushboolean(  L, va_arg(vl, int   ) != 0            );  break; // NB: boolean is an int-sized thing!
       case 's':  lua_pushstring(   L, va_arg(vl, PCChar)                 );  break;
       case 'S':  lua_pushstring(   L, va_arg(vl, std::string *)->c_str() );  break;
       case 'x':  lua_pushstring(   L, va_arg(vl, PXbuf )->c_str()         );  break;
       case '>':  goto CALL_LUA_FUNCTION;
       }
      luaL_checkstack( L, 1, "too many arguments" );
      }

CALL_LUA_FUNCTION:
   auto nres( Strlen( ++szSig ) );  // number of expected results

   {
   LuaCallCleanup lcc( L );
   if( docall( L, narg, 0 ) != 0 )  // do the call
      l_handle_pcall_error( L );

   pszSigStart = szSig;
   for( nres = -nres ; *szSig ; ++nres, ++szSig ) {
      switch( *szSig ) {
       case 'd':  if( !lua_isnumber( L, nres ) )  goto WRONG_RESULT_TYPE;
                  *va_arg(vl, double *) =      lua_tonumber( L, nres );
                  break;

       case 'i':  if( !lua_isnumber( L, nres ) )  goto WRONG_RESULT_TYPE;
                  *va_arg(vl, int *)    = (int)lua_tointeger( L, nres );
                  break;

       case 'b':  *va_arg(vl, int *)    =      lua_toboolean( L, nres );  // NB: boolean is an int-sized thing!
                  break;

       case 'h':  if( !lua_isstring( L, nres ) )  goto WRONG_RESULT_TYPE;
                  {
                  size_t srcBytes;
                  auto pSrc( lua_tolstring(L, nres, &srcBytes) );
                  ++srcBytes;
                  PChar pDest;
                  AllocBytesNZ( pDest, srcBytes ); // can copy binary data, not just string
                  auto psd( va_arg(vl, PStrDest) );
                  psd->BindBuf( pDest, srcBytes );
                  psd->CopyTo( pSrc, srcBytes );
                  }
                  break;

       case 'S':  if( !lua_isstring( L, nres ) )  goto WRONG_RESULT_TYPE;
                  {
                  size_t srcBytes;
                  auto pSrc( lua_tolstring(L, nres, &srcBytes) );
                  va_arg(vl, std::string *)->assign( pSrc, srcBytes );
                  }
                  break;

       case 'x':  if( !lua_isstring( L, nres ) )  goto WRONG_RESULT_TYPE;
                  {
                  size_t srcBytes;
                  auto pSrc( lua_tolstring(L, nres, &srcBytes) );
                  va_arg(vl, PXbuf)->cpy( pSrc, srcBytes+1 );
                  }
                  break;

       default:   return Msg( "%s: unsupported return type specifier '%c'", __func__, *szSig );
       }
      }

   return true;

WRONG_RESULT_TYPE:

   return Msg( "%s: wrong result-type[%d] (expected '%c')", __func__, nres, szSig[0] );
   }
   }

STATIC_FXN bool callLuaOk( lua_State *L, const char *szFuncnm, const char *szSig, ... ) {
   va_list  vl;
   va_start(vl, szSig);
   const auto ok( vcallLuaOk( L, szFuncnm, szSig, vl ) );
   va_end( vl );
   return ok;
   }

bool LuaCtxt_Edit::ExecutedURL( PCChar strToExecute ) {
   int didit;
   return callLuaOk( L_edit, "ExecutedURL", "s>b", strToExecute, &didit ) && didit;
   }

// intf into Lua locn-list subsys
void LuaCtxt_Edit::LocnListInsertCursor()               {        callLuaOk( L_edit, "LocnListInsertCursor", ">" ); }


bool LuaCtxt_Edit::nextmsg_setbufnm     ( PCChar src )  { return callLuaOk( L_edit, "nextmsg_setbufnm_FROM_C"  , "s>" , src ); }
bool LuaCtxt_Edit::nextmsg_newsection_ok( PCChar src )  { return callLuaOk( L_edit, "nextmsg_newsection_FROM_C", "s>" , src ); }
bool LuaCtxt_Edit::ReadPseudoFileOk     ( PFBUF src, int *pRvBool )  { *pRvBool = false;
                                                          return callLuaOk( L_edit, "ReadPseudoFileOk_FROM_C"  , "F>b", src, pRvBool ); }

COL FBOP::GetSoftcrIndentLua( PFBUF fb, LINE yLine ) {
   COL rv;
   return callLuaOk( L_edit, "Softcr_col_from_C", "Fi>i", fb, yLine, &rv ) ? rv-1 : -1;
   }

void FBOP::LuaSortLineRange( PFBUF fb, LINE y0, LINE y1, COL xLeft, COL xRight, bool fCaseIgnored, bool fOrdrAscending, bool fDupsKeep ) {
   SW_BP;
   callLuaOk( L_edit, "SortLineRange_from_C"
      , "Fiiiibbb>"
      , fb
      , y0 - 1
      , y1 - 1
      , xLeft
      , xRight
      , fCaseIgnored
      , fOrdrAscending
      , fDupsKeep
      );
   }

//
// String transformation functions have standard signature:
//
// These can behave as either:
//
// src==0: single buffer (dest is input and output)
// src!=0: double buffer
//
// there is no efficiency difference, since the Lua result has to be copied into
// dest regardless.
//
STIL bool Lua_x2x( lua_State *L, PCChar functionName, PXbuf pxb ) {
   return callLuaOk( L, functionName, "x>x", pxb, pxb );
   }

STIL bool Lua_S2S( lua_State *L, PCChar functionName, Path::str_t &inout ) {
   return callLuaOk( L, functionName, "S>S", &inout, &inout );
   }

STATIC_FXN PChar Lua_s2h( lua_State *L, PCChar functionName, PCChar src ) {
   StrDest sbDest( 0, 0 );
   const auto ok( callLuaOk( L, functionName, "s>h", src, &sbDest ) );
   if( !ok )
      return Strdup( src );      // if failed, copy input to output if necessary
   return sbDest.d_pBuf;
   }

bool  LuaCtxt_Edit::ExpandEnvVarsOk    ( Path::str_t &st ) { return Lua_S2S( L_edit, "StrExpandEnvVars"  , st  ); }
bool  LuaCtxt_Edit::from_C_lookup_glock( PXbuf pxb ) { return Lua_x2x( L_edit, "Lua_from_C_lookup_glock" , pxb ); }

//
// reliable reading of Lua global variables (including '.'-separated table expressions) from C
//
STIL bool getTblVal( lua_State *L, PCChar key, int ix ) { 0 && DBG( "%s indexing '%s'", __func__, key );
   lua_getfield( L, ix==0?LUA_GLOBALSINDEX:-1, key );
   return true;
   }

STATIC_FXN bool gotTblVal( lua_State *L, PCChar pcRvalNm ) {
   ALLOCA_STRDUP( rvNm, rvlen, pcRvalNm, Strlen( pcRvalNm ) );
   PCChar name[ 20 ];
   auto depth(0);
   name[depth++] = rvNm;
   for( PChar pc(rvNm); pc < rvNm + rvlen; ++pc )
      if( *pc == '.' ) {
         *pc = '\0';
         if( !(depth < ELEMENTS(name)) ) { 0 && DBG( "%s MAX DEPTH (%Iu) exceeded: %s", __func__, ELEMENTS(name), pcRvalNm );
            return false;
            }
         name[depth++] = pc+1;
         }

   0 && DBG( "%s depth = %d", __func__, depth );

   auto ix(0);
   for( ; ix < depth-1; ++ix ) { 0 && DBG(  "%s ix = %d", __func__, ix );
      if( !getTblVal( L, name[ix], ix ) )
         return false;
      if( !lua_istable( L, -1 ) ) { 0 && DBG( "%s field '%s' is not a table in '%s'", __func__, name[ix], pcRvalNm );
         return false;
         }
      }

   0 && DBG( "%s ix(out) = %d", __func__, ix );
   return getTblVal( L, name[ix], ix ); // *** caller is responsible for converting TOS to appropriate C value ***
   }

// use THIS fxn to copy Lua strings: fail-safe!
PChar CopyLuaString( PChar dest, size_t sizeof_dest, lua_State *L, int stackLevel ) { // converts number to string!
   dest[0] = '\0';
   const auto pSrc( lua_tostring( L, -1 ) );
   if( pSrc )
      safeStrcpy( dest, sizeof_dest, pSrc );
   return dest;
   }

// use THIS fxn to Strdup Lua strings: _NOT_ fail-safe!
PChar DupLuaString( lua_State *L, int stackLevel ) { // converts number to string!
   const auto pSrc( lua_tostring( L, -1 ) );
   if( !pSrc ) return nullptr;
   return Strdup( pSrc );
   }

// returns nullptr if any errors
STATIC_FXN PChar LuaTbl2S0( lua_State *L, PChar dest, size_t sizeof_dest, PCChar tableDescr ) {
   0 && DBG( "+%s '%s'?", __func__ , tableDescr );
   if( !L ) return nullptr;

   LuaCallCleanup lcc( L );
   if( !gotTblVal( L, tableDescr ) )
      return nullptr;

   return CopyLuaString( dest, sizeof_dest, L, -1 );
   }

// returns nullptr if any errors
STATIC_FXN PChar LuaTbl2DupS0( lua_State *L, PCChar tableDescr ) {
   0 && DBG( "+%s '%s'?", __func__ , tableDescr );
   if( !L ) return nullptr;

   LuaCallCleanup lcc( L );
   if( !gotTblVal( L, tableDescr ) )
      return nullptr;

   return DupLuaString( L, -1 );
   }

// returns empty string if any errors
STATIC_FXN PChar LuaTbl2S( lua_State *L, PChar dest, size_t sizeof_dest, PCChar tableDescr, PCChar pszDflt ) {
   0 && DBG( "+%s '%s'?", __func__ , tableDescr );
   auto rv( LuaTbl2S0( L, dest, sizeof_dest, tableDescr ) );
   if( !rv ) {
      if( pszDflt )
         safeStrcpy( dest, sizeof_dest, pszDflt );
      else
         dest[0] = '\0';
      }

   0 && DBG( "-%s '%s' -> '%s'", __func__ , tableDescr, dest );
   return dest;
   }

STATIC_FXN PChar LuaTbl2DupS( lua_State *L, PCChar tableDescr, PCChar pszDflt ) {
   1 && DBG( "+%s '%s'?", __func__ , tableDescr );
   auto rv( LuaTbl2DupS0( L, tableDescr ) );
   if( !rv ) {
      if( pszDflt ) {
         rv = Strdup( pszDflt );
         }
      }

   1 && DBG( "-%s '%s' -> '%s'", __func__ , tableDescr, rv );
   return rv;
   }

// returns dfltVal if any errors
STATIC_FXN int LuaTbl2Int( lua_State *L, PCChar tableDescr, int dfltVal ) {
   if( !L )
      return dfltVal;

   LuaCallCleanup lcc( L );

   auto rv(dfltVal);
   if( gotTblVal( L, tableDescr ) ) {
      if( !lua_isnumber( L, -1 ) ) {
         DBG( "%s '%s' is not a number", __func__, tableDescr );
         }
      else {
         rv = lua_tointeger( L, -1 );
         }
      }
   0 && DBG( "%s '%s' (%d) -> %d", __func__, tableDescr, dfltVal, rv );
   return rv;
   }

// returns nullptr if any errors
PChar LuaCtxt_Edit::Tbl2S0( PChar dest, size_t sizeof_dest, PCChar tableDescr )  { return LuaTbl2S0( L_edit, dest, sizeof_dest, tableDescr ); }

// returns empty string if any errors
PChar LuaCtxt_Edit::Tbl2S( PChar dest, size_t sizeof_dest, PCChar tableDescr, PCChar pszDflt ) { return LuaTbl2S( L_edit, dest, sizeof_dest, tableDescr, pszDflt ); }

// returns dup of pszDflt or nullptr if any errors
PChar LuaCtxt_Edit::Tbl2DupS0( PCChar tableDescr, PCChar pszDflt ) { return LuaTbl2DupS( L_edit, tableDescr, pszDflt ); }

// returns dfltVal if any errors
int LuaCtxt_Edit::Tbl2Int( PCChar tableDescr, int dfltVal ) { return LuaTbl2Int( L_edit, tableDescr, dfltVal ); }

#pragma GCC diagnostic pop
