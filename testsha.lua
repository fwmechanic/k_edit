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
