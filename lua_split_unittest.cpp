//
// Copyright 2026 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

// Include the implementation so its file-local Lua functions can be exercised
// directly.  The unit-test target uses function/data sections and link-time
// garbage collection so unrelated editor integration is not linked.
#define UNITTEST_LUA_SPLIT
#include "lua_edlib.cpp"

#include <cassert>
#include <initializer_list>
#include <string>

STATIC_FXN void push_split_call( lua_State *L, lua_CFunction splitFunction, const std::string &text, PCChar separator ) {
   lua_settop( L, 0 );
   lua_pushcfunction( L, splitFunction );
   lua_pushlstring( L, text.data(), text.size() );
   lua_pushstring( L, separator );
   }

STATIC_FXN void expect_multi( lua_State *L, const std::string &text, PCChar separator, std::initializer_list<PCChar> expected ) {
   push_split_call( L, LExFx::split_str, text, separator );
   assert( lua_pcall( L, 2, LUA_MULTRET, 0 ) == 0 );
   assert( lua_gettop(L) == static_cast<int>(expected.size()) );
   auto resultIndex = 1;
   for( const auto value : expected ) {
      assert( std::string(luaL_checkstring( L, resultIndex++ )) == value );
      }
   }

STATIC_FXN void expect_table( lua_State *L, const std::string &text, PCChar separator, std::initializer_list<PCChar> expected ) {
   push_split_call( L, LExFx::split_str_tbl, text, separator );
   assert( lua_pcall( L, 2, 1, 0 ) == 0 );
   assert( lua_istable( L, 1 ) );
   assert( lua_objlen( L, 1 ) == expected.size() );
   auto resultIndex = 1;
   for( const auto value : expected ) {
      lua_rawgeti( L, 1, resultIndex++ );
      assert( std::string(luaL_checkstring( L, -1 )) == value );
      lua_pop( L, 1 );
      }
   }

STATIC_FXN void expect_empty_separator_error( lua_State *L, lua_CFunction splitFunction ) {
   push_split_call( L, splitFunction, "abc", "" );
   assert( lua_pcall( L, 2, LUA_MULTRET, 0 ) != 0 );
   const std::string error( luaL_checkstring(L, -1) );
   assert( error.find("separator must not be empty") != std::string::npos );
   }

STATIC_FXN std::string repeated_split_input( size_t separatorCount ) {
   std::string text;
   text.reserve( separatorCount * 2 + 1 );
   for( size_t ix=0 ; ix < separatorCount ; ++ix ) {
      text.append( "x," );
      }
   text.push_back( 'x' );
   return text;
   }

STATIC_FXN void expect_stack_boundary( lua_State *L ) {
   const auto accepted( repeated_split_input(7900) );
   push_split_call( L, LExFx::split_str, accepted, "," );
   assert( lua_pcall( L, 2, LUA_MULTRET, 0 ) == 0 );
   assert( lua_gettop(L) == 7901 );

   const auto rejected( repeated_split_input(8000) );
   push_split_call( L, LExFx::split_str, rejected, "," );
   assert( lua_pcall( L, 2, LUA_MULTRET, 0 ) != 0 );
   const std::string error( luaL_checkstring(L, -1) );
   assert( error.find("too many split results") != std::string::npos );
   }

int main() {
   auto L( luaL_newstate() );
   assert( L );

   expect_multi( L, "", "--", { "" } );
   expect_multi( L, "abc", "--", { "abc" } );
   expect_multi( L, "::a::b::", "::", { "", "a", "b", "" } );
   expect_multi( L, "a::::b", "::", { "a", "", "b" } );
   expect_table( L, "", "--", { "" } );
   expect_table( L, "::a::b::", "::", { "", "a", "b", "" } );
   expect_empty_separator_error( L, LExFx::split_str );
   expect_empty_separator_error( L, LExFx::split_str_tbl );
   expect_stack_boundary( L );

   lua_close( L );
   puts( "PASS" );
   return 0;
   }
