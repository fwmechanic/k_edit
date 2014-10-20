/*
** $Id: linit.c,v 1.14.1.1 2007/12/27 13:02:25 roberto Exp $
** Initialization of libraries for lua.c
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"
#include "k_lib.h"                     /* 20060825 kgoodwin private library (k_lib.c) */

extern int luaopen_pack(lua_State *L); /* 20070313 kgoodwin PubDomain code (lpack.c) */

static const luaL_Reg lualibs[] = {
  {"", luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_IOLIBNAME, luaopen_io},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_DBLIBNAME, luaopen_debug},
  {LUA_KLIBNAME, luaopen_klib},        /* 20060825 kgoodwin private library (k_lib.c) */
  {"lpack", luaopen_pack },            /* 20070313 kgoodwin PubDomain code (lpack.c) */
//{"pcre", luaopen_rex_pcre },         /* 20080105 kgoodwin PubDomain code (lrexlib w/significant hacks to static compile) */
  {"lpeg", luaopen_lpeg },             /* 20081110 kgoodwin PubDomain code (lpeg.c) */
  {NULL, NULL}
};


LUALIB_API void luaL_openlibs (lua_State *L) {
  const luaL_Reg *lib = lualibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }
}
