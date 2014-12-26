//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

#include "ed_main.h"
#include "fname_gen.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

// book  http://www.lua.org/pil/
//       file://c:/klg/sn/k/lua-5.1/doc/contents.html

// see http://www.allacrost.org/ (open-)source code for an example of cooperating Lua and C++ interface

extern "C"
{
   #include <lua.h>
   #include <lauxlib.h>
   #include <lualib.h>
   #include <k_lib.h>
}

enum { DBG_LUA=0 };

//#########  lua_edlib exports

extern void l_OpenStdLibs   ( lua_State *L );
extern void l_register_EdLib( lua_State *L );

extern int  l_construct_View( lua_State *L, PView pView );
extern int  l_construct_FBUF( lua_State *L, PFBUF pFBuf );
extern int  l_construct_Win ( lua_State *L, PWin  pWin  );

extern int  l_View_function_cur( lua_State *L );

//#########  utility externs

extern int   LDS( PCChar tag, lua_State *L );

//#########  common inlines

STIL PChar getfieldStrdup( lua_State *L, const char *key ) { lua_getfield(L, -1, key); PChar rv( Strdup( luaL_checkstring(L, -1) ) ); lua_pop(L, 1); return rv; }
STIL int   getfieldInt(    lua_State *L, const char *key ) { lua_getfield(L, -1, key); const int rv( luaL_checkint(L, -1) )         ; lua_pop(L, 1); return rv; }

STIL void setfield(  lua_State *L, PCChar key, PCChar        val ) { lua_pushstring(    L, val ); lua_setfield( L, -2, key ); }
STIL void setfield(  lua_State *L, PCChar key, int           val ) { lua_pushinteger(   L, val ); lua_setfield( L, -2, key ); }
STIL void setfield(  lua_State *L, PCChar key, lua_CFunction val ) { lua_pushcfunction( L, val ); lua_setfield( L, -2, key ); }

STIL void setglobal( lua_State *L, PCChar key, PCChar val ) { lua_pushstring(  L, val ); lua_setglobal( L, key ); }
STIL void setglobal( lua_State *L, PCChar key, int    val ) { lua_pushinteger( L, val ); lua_setglobal( L, key ); }

#define  USE_STATE_ELB  0

 #if USE_STATE_ELB
 extern PXbuf get_xb( lua_State *L );
 #endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
