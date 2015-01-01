//
// Kevin's macros to make writing Lua functions in C easy
//

#ifndef  LUA_K_LIB_H_
#define  LUA_K_LIB_H_

// accessing parameters

   // I_(n) macro does not handle corner case where int value would have MSBit
   // set.  For those cases where it is assumed that the "integer" value is
   // actually unsigned, you should use U_ which calls GetUint_Hack.
   //
   // note also that if a numeric RETURN value could be > MAX_INT, you should use R_num instead of R_int
   //
   // this behavior seen with Lua 5.1.2 and 5.1.3
   //
   // NB: if this code is condensed, it may stop working...
   //
   // 20080505 kgoodwin discovered/wrote

extern unsigned GetUint_Hack( lua_State *L, int n );

#define  S_(n)              luaL_checkstring( L, (n) )
static inline stref Sr_( lua_State *L, int n ) {
   size_t length;
   const auto data( luaL_checklstring( L, (n), &length ) );
   if( !data ) return stref();
   else        return stref( data, length );
   }

#define  I_(n)              luaL_checkint(    L, (n) )
#define  U_(n)              GetUint_Hack(     L, (n) )
#define  N_(n)              luaL_checknumber( L, (n) )
#define  So0_(n)            luaL_optstring(   L, (n), 0 )
#define  Io_(n,dflt)        luaL_optint(      L, (n), (dflt))
#ifdef __cplusplus
#define  LuaBool(n)         bool(lua_toboolean(L, (n) ) != 0)
#endif
#define  B_(n)             (luaL_checkint(    L, (n) ) != 0)
#define  Bytes_(n,bytes)     (                       luaL_checklstring( L, (n), &bytes ))
#define  UBytes_(n,bytes)    ((const unsigned char *)luaL_checklstring( L, (n), &bytes ))

// returning 0 or 1 items from a function

#define  RZ                 return 0
#define  P_nil()            lua_pushnil(     L )
#define  P_bool(val)        lua_pushboolean( L, (val) )
#define  P_int(val)         lua_pushinteger( L, (val) )
#define  P_num(val)         lua_pushnumber(  L, (val) )
#define  P_str(val)         lua_pushstring ( L, (val) )
#define  P_lstr(pbuf,bytes) lua_pushlstring( L, (pbuf), (bytes) )
#define  P_strf(fmt,val)    lua_pushfstring( L, (fmt), (val) )

#define  R_nil()            P_nil()            ; return 1
#define  R_bool(val)        P_bool(val)        ; return 1
#define  R_int(val)         P_int(val)         ; return 1
#define  R_num(val)         P_num(val)         ; return 1
#define  R_str(val)         P_str(val)         ; return 1
#define  R_lstr(pbuf,bytes) P_lstr(pbuf,bytes) ; return 1
#define  R_strf(fmt,val)    P_strf(fmt,val)    ; return 1


#define  ERR_strf(fmt,val)  lua_pushfstring( L, fmt, (val) ); lua_error( L )

#define  LUAFUNC_( func )  static int func( lua_State *L )

#define LUA_KLIBNAME "klib"
LUALIB_API int luaopen_klib (lua_State *L);
LUALIB_API int luaopen_rex_pcre (lua_State *L);
LUALIB_API int luaopen_lpeg (lua_State *L);

#endif
