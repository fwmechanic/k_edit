-- "unit" test code for k_lib.c submodule _dir

function fmt(...) return string.format(...) end
printf = function (...) print( fmt(...) )     end

local stars = string.rep( "*", 80 )

print( stars )

do -- test _dir C library
   printf( "_dir tests: begin (cwd=%s)", _dir.current() )
   local ods = _dir.dirsep_os()
   local pds = _dir.dirsep_preferred()
   if pds ~= ods then printf( "_dir tests: begin (cwd=%s)", _dir.current(pds) )
      for ix,nm in ipairs( _dir.read_names( ".."..ods..".."..pds, 0 ) ) do  printf( "%4d: %-40s -> '%s'", ix, nm, _dir.fullpath( nm ) )  end
                                                                            printf( "-----------------------------------------------" )
      for ix,nm in ipairs( _dir.read_names( ".."..pds..".."..ods, 0 ) ) do  printf( "%4d: %-40s -> '%s'", ix, nm, _dir.fullpath( nm ) )  end
                                                                            printf( "-----------------------------------------------" )
      end
   for ix,nm in ipairs( _dir.read_names( ".."..ods..".."..ods, 0 ) ) do  printf( "%4d: %-40s -> '%s'", ix, nm, _dir.fullpath( nm ) )  end
                                                                         printf( "-----------------------------------------------" )
   -- local pathx = "..\\..\\*.*"
   local pathx = ".."..ods..".."..ods.."*.*"
   printf( "_dir.fullpath '%s' -> '%s'", pathx, _dir.fullpath( pathx ) )
   print( "_dir tests: passed" )
end

print( stars )
