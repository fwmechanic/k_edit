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
require "lib"

local nl = "\n"
local wr = io.write
local lbls = {}

local function nm_min( st )
   st = st:gsub( " Switch$","" )
   st = st:gsub( " Function$","" )
   st = st:gsub( " Macro$","" )
   return st
   end

--[[

 A header without an explicitly specified identifier will be automatically assigned a unique identifier based on the header
 text.  To derive the identifier from the header text,

    Remove all formatting, links, etc.
    Remove all footnotes.
    Remove all punctuation, except underscores, hyphens, and periods.
    Replace all spaces and newlines with hyphens.
    Convert all alphabetic characters to lowercase.
    Remove everything up to the first letter (identifiers may not begin with a number or punctuation mark).
    If nothing is left after this, use the identifier section.
--]]

local ispunct = '!"'.."#$%&'()*+,-./:;<=>?@[\]^_`{|}~"          -- wr( "ispunct    =",ispunct,"|",nl )
local ispunct_hdr = ispunct:gsub("[-_%.]","")                   -- wr( "ispunct_hdr=",ispunct_hdr,"|",nl )
      ispunct_hdr = "["..ispunct_hdr:gsub("(%p)","%%%0").."]+"  -- wr( "ispunct_hdr=",ispunct_hdr,"|",nl )

local function hdr_id( st )
   st = st:gsub(ispunct_hdr,"")
   st = st:gsub("%s","-")
   st = st:gsub("%-%-+","-")
   st = st:lower()
   st = st:gsub("^%A+","")
   assert( #st > 0 )
   return st
   end

local HBAR = "Ä"
local pre = {
   function( ln )
      local htext = ln:match( "^"..HBAR..HBAR.."([^"..HBAR.."]+)"..HBAR )
      if htext then                                      wr( "htext = '",htext,"'",nl)
         local rv = "# "..htext --.." #"
         local key = nm_min( htext )
         local hdid = hdr_id( htext )
         assert( nil == lbls[key] )
         lbls[key] = { hdid, htext }                              wr( "lbls[",key,"] = ",hdid,nl)
         return rv
         end
      end,
   }

local out = {}
local ifh = assert( io.open( "khelp.txt", "r" ) )
for ln in ifh:lines() do
   ln = ln:gsub("[ÅÃÂÙÀ]","+" )
   ln = ln:gsub("Û","#" )
   ln = ln:gsub("þ","*" )
   ln = ln:gsub("[³]","|" )
   for _,xlfx in ipairs(pre) do
      local nuln = xlfx(ln)
      if nuln then
         ln = nuln
         break
         end
      end
   ln = ln:gsub("Ä","-" )
   out[1+#out] = ln
   end
assert( ifh:close() )

wr( nl,nl )

local ofh = assert( io.open( "khelp.md_", "w" ) )
for _,ln in ipairs(out) do
   ln = ln:gsub( "<<([^>]+)>>", function( st )                    -- wr("st=",st,"|",nl)
      local ss = st
      ss = nm_min( ss )                                           -- wr("ss=",ss,"|",nl)
      local su = ss:gsub( "^%l", function(st) return st:upper() end ) -- if su~=ss then wr( "su=",su,"|",nl ) end
      local rplc = lbls[ss] or lbls[su]
      if not rplc then
         wr( "*** unknown label '"..ss.."'", nl )
         return ss
      else
         rplc = "["..rplc[2].."](#".. rplc[1] ..")"
         -- wr( "'",ss,"'->'",rplc,"'",nl )
         return rplc
         end
      end
      )
   ofh:write(ln,nl)
   end
assert( ofh:close() )
