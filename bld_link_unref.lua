require "strict"
require "tu"

local unrefs = {}

for line in io.lines() do
   local unref = line:match( "undefined reference to `(.+)'" )
   if unref then
      unrefs[unref] = (unrefs[unref] or 0) + 1
      end
   end

local unrefdCount = 0
for nm, count in pairs( unrefs ) do  -- print( "-> \"" .. ix, tb.fxn .. "\"" )
-- for nm, count in tu.pairsBySortedValues( unrefs ) do  -- print( "-> \"" .. ix, tb.fxn .. "\"" )
   io.write( string.format("%3d", count), "  '", nm, "'\n" )
   unrefdCount = 1+ unrefdCount
   end

io.stderr:write( unrefdCount, " undefined references", "\n" )
os.exit(1)
