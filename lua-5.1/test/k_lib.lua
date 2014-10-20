-- "unit" test code for k_lib.c submodules _dir & _bin and Lua bin module
--  which uses primitives in _bin module to present a user-friendly interface

function fmt(...) return string.format(...) end
printf = function (...) print( fmt(...) )     end

require "bin"

local stars = string.rep( "*", 80 )

print( stars )

do -- test _dir C library
   printf( "_dir tests: begin (cwd=%s)", _dir.current() )
   printf( "_dir tests: begin (cwd=%s)", _dir.current("/") )
   for ix,nm in ipairs( _dir.read_names( "../..\\" , 0 ) ) do  printf( "%4d: %-40s -> '%s'", ix, nm, _dir.fullpath( nm ) )  end
                                                               printf( "-----------------------------------------------" )
   for ix,nm in ipairs( _dir.read_names( "..\\../" , 0 ) ) do  printf( "%4d: %-40s -> '%s'", ix, nm, _dir.fullpath( nm ) )  end
                                                               printf( "-----------------------------------------------" )
   for ix,nm in ipairs( _dir.read_names( "..\\..\\", 0 ) ) do  printf( "%4d: %-40s -> '%s'", ix, nm, _dir.fullpath( nm ) )  end
                                                               printf( "-----------------------------------------------" )
   -- local pathx = "..\\..\\*.*"
   local pathx = "../../*.*"
   printf( "_dir.fullpath '%s' -> '%s'", pathx, _dir.fullpath( pathx ) )
   print( "_dir tests: passed" )
end

print( stars )
