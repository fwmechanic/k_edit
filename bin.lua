-- if you change or add to this module, _please_ update test\k_lib.lua accordingly!

module( "bin", package.seeall )

require "util"

-- very simple un/packer: supports only fixed length byte strings and 1-/2-/4-byte binary numbers

local function unpack_one( tt, byteSrc, ix, endian )
   local at, bytes = tt:match( "^(%a?)(%d+)$" )
   local errstr = string.format( "unknown unpack[%d] template specifier '%s'", ix, tt )
   util.errassert( bytes, errstr, 3 )
   bytes = 0+bytes
   local raw = byteSrc( bytes, string.format( "unpack[%d] '%s'", ix, tt ) )
   if at ==""   then  return _bin.unpack_num( raw, endian )  end
   if at == "s" then  return raw  end
   error( errstr, 3 )
   end

-- **********  NB!!!  There is a 'unpack' function in the Lua global scope!  See PiL2e pg 39

local function unpack( tmplt, endian, byteString )
   local bsbs = util.byteSource( byteString )
   local aTmplts = util.matches( tmplt )
   local rv = {}
   for ix,ta in ipairs( aTmplts ) do
      rv[ #rv + 1 ] = unpack_one( ta, bsbs, ix, endian )
      end
   return rv
   end

function unpackLE( tmplt, byteString )  return unpack( tmplt, false, byteString ) end
function unpackBE( tmplt, byteString )  return unpack( tmplt, true , byteString ) end

------------

local function pack_one( tt, data, ix, endian )
   local at, bytes = tt:match( "^(%a?)(%d+)$" )
   -- print( "pattern '"..tt.."'".. at )
   local errstr = string.format( "unknown pack[%d] template specifier '%s'", ix, tt )
   util.errassert( bytes, errstr, 3 )
   bytes = 0+bytes
   local badparam = string.format( "bad pack[%d] parameter (%s) for template specifier '%s'", ix, type(data), tt )
   if at == ""  then
      util.errassert( type(data) == "number", badparam, 3 )
      return _bin.pack_num( data, bytes, endian )
      end
   if at == "s" then
      util.errassert( type(data) == "string", badparam, 3 )
      local rv = data:sub( 1, bytes )
      if #rv < bytes then  rv = rv .. string.rep( string.char(0), bytes - #rv )  end
      return rv
      end
   error( errstr, 3 )
   end

local function pack( tmplt, endian, ... )
   local aData = { ... }
   local aTmplts = util.matches( tmplt )
   local rv = {}
   for ix,ta in ipairs( aTmplts ) do
      local data = table.remove( aData, 1 )
      assert( data, "" )
      rv[ #rv + 1 ] = pack_one( ta, data, ix, endian )
      end
   return table.concat( rv )
   end

function packLE( tmplt, ... ) return pack( tmplt, false, ...  ) end
function packBE( tmplt, ... ) return pack( tmplt, true , ...  ) end

