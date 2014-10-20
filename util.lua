--
--  Copyright 2006 by Kevin L. Goodwin; All rights reserved
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
