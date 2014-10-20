require "strict"

local function sha1_file( fnm )
   local shaa, bytes = sha1.sha1_file( fnm )
   for ix=1,#shaa do io.write( string.format( "%02x", shaa:byte(ix) ) ) end
   io.write("  ", bytes,"\n")
   end

local nms = _dir.read_names( "./", 1 )

for _,nm in ipairs(nms) do
   for _,pat in ipairs(arg) do
      if nm:match(pat) then
         io.write( nm, ": " )
         sha1_file( nm )
         end
      end
   end
