--
-- Copyright 2015 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
--
-- This file is part of K.
--
-- K is free software: you can redistribute it and/or modify it under the
-- terms of the GNU General Public License as published by the Free Software
-- Foundation, either version 3 of the License, or (at your option) any later
-- version.
--
-- K is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
-- FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
-- details.
--
-- You should have received a copy of the GNU General Public License along
-- with K.  If not, see <http://www.gnu.org/licenses/>.
--

module( "util", package.seeall )


-- http://www.lua.org/pil/
-- http://lua-users.org/wiki/StringRecipes
-- http://lua-users.org/lists/lua-l/
-- file://c:/klg/sn/k/lua-5.1/doc/contents.html           luaL_openlib  see luaL_register


function trim( strval, spcrplc )
   return strval:gsub( "^%s*(.-)%s*$" , "%1" )
   end


function nowhite( strval, spcrplc )
   return strval:gsub( "(%s+)", "" )
   end


function rpad( str, maxwidth )
   local len = str:len()
   if len < maxwidth then   return str .. string.rep( " ", maxwidth - len )
   else                     return str end
   end

function case_insensitive_pattern( str ) return str:gsub( "%a", function(ch) return "["..ch:upper()..ch:lower(ch).."]" end ) end
function pattern_escape_magic( str ) return str:gsub( "[-%[%]().+*?^$%%]", function(ch) return "%"..ch end ) end


function errassert( expr, msg, level )
   if not expr then error( msg, level+1 ) end
   return expr
   end


function matches( text, pat )  -- splitp
   local rv = {}
   text:gsub( pat or "(%S+)",
      function( match ) rv[ #rv + 1 ] = match  end
      )
   return rv
   end


function byteSource( buffer, offset )
   printf( "byteSource: len %d", buffer:len() )
   offset = offset or 0
   return function( len,  errstr )
      if not len then return buffer:len()  end  -- parameterless call returns bytes remaining/available
      assert( buffer:len() >= len, fmt( "error reading %s, offset %d: needed %d bytes, had %d\n", errstr, offset, len, buffer:len() ) )
      local rvOfs = offset
      local rvBuf = buffer:sub( 1, len )
      buffer = buffer:sub( len+1 )
      offset = offset + len
      return rvBuf, rvOfs
      end
   end
