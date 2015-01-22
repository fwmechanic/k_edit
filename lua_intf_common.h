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

static inline stref Sr_( lua_State *L, int n ) {
   size_t length;
   const auto data( luaL_checklstring( L, (n), &length ) );
   if( !data ) return stref();
   else        return stref( data, length );
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

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
