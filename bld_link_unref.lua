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

require "strict"
require "tu"

local ofnm = "link.unref"

local ofh = assert( io.open( ofnm, "wt" ) )

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
   ofh:write( string.format("%3d", count), "  '", nm, "'\n" )
   unrefdCount = 1+ unrefdCount
   end


io.stderr:write( unrefdCount, " undefined references, see ", ofnm, "\n" )
os.exit(1)
