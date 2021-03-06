--
-- Copyright 2015-2019 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

-- Component in K build process: reads cmdtbl.dat, writes 4 output files.
-- For more details see hdr of cmdtbl.dat

-- require "strict"
require "util"
require "tu"

function mangleName( name )
   return name:gsub( "!", "BANG" )
   end

-- shorthand / optimized-access
   local fmt      = string.format
   local rpad     = util.rpad
   local trim     = util.trim
   local nowhite  = util.nowhite

function nextArg() return table.remove( arg, 1 ) end

local platform_arg = assert( nextArg(), "missing platform_arg" )
local plat_pats = {mingw="[s w]",linux="[s x]"}
local plat_pat = "^" .. assert( plat_pats[platform_arg], platform_arg.." is unknown platform_arg" )

-- io.write( "w_only = '", w_only,"'","\n" )  os.exit(1)

local function maxAccumr()
   local max = 0
   return function( len )
      if len==nil then return max end  -- read mode: updt nil, return max
      if max < len then max = len end  -- updt mode: updt max, return nil
      end
   end

local maxAttrWidth  = maxAccumr()
local maxfnmWidth   = maxAccumr()
local maxNameWidth  = maxAccumr()
local maxMNameWidth = maxAccumr()
local tbl = {}

local TEXTARG_gts_conflict_counter = 0
for line in io.lines() do
   line = line:gsub( "#.*$", "" )
   local plat_mat = line:match( plat_pat )
   if plat_mat then
      line = line:gsub( "^.", "" )
      line = trim( line )
      -- print( line )
      -- print( "\""..line.."\"" )
      local name,fxn,attr_tail = line:match( "^%s*(%S+)%s+(%S+)%s+(.+)$" )
      local oComma = attr_tail:find( "," )
      local attr_expr  = attr_tail
      local helpString = ""
      if oComma then
         attr_expr  = attr_tail:sub( 1, oComma-1 )
         helpString = trim( attr_tail:sub( oComma+1 ) )
         end

      attr_expr = nowhite( attr_expr )
      local mname = mangleName( name )
      name = name:lower()
      -- print( fmt( "mnm=\"%s\", nm=\"%s\", fxn=\"%s\", attr=\"%s\" hlp=\"%s\"", mname, name, fxn, attr_expr, helpString ) )
      assert( nil == tbl[name], "name "..name.." defined twice! (case INsensitive!)" )
      local has_gts = plat_mat == "s"
      local accepts_TEXTARG = attr_expr:match( 'TEXTARG' ) and true or false
      if name ~= 'cancel' and has_gts and accepts_TEXTARG then -- cancel, being destructive, gets a pass...
         print( "error: EdFxn "..name.." accepts TEXTARG and defines GTS::"..name )
         TEXTARG_gts_conflict_counter = 1+TEXTARG_gts_conflict_counter
         end
      tbl[name] = {
           fxn   = fxn
         , attr  = attr_expr
         , help  = helpString
         , mname = mname
         , has_gts = has_gts
         }

      maxNameWidth ( #name      )
      maxMNameWidth( #mname     )
      maxAttrWidth ( #attr_expr )
      maxfnmWidth  ( #fxn       )
      end
   end

if TEXTARG_gts_conflict_counter > 0 then
   print( "DYING: there were "..TEXTARG_gts_conflict_counter.." TEXTARG-GTS:: conflicts" )
   os.exit(1)
   end

-- this prog used to write 4 _output files_, but standard GNU make has completely brain-dead non-support for
-- rules having multiple targets, and the only published workaround turns any incremental build into a FULL build.
-- Therefore I'm modifying this program to generate a SINGLE output file whose sections are bracketed by
-- C preprocessor conditional #if ... #endif
-- 20131013 kgoodwin

local pfh = { "#if defined(CMDTBL_H_ARG_METHODS)\n\n" }
local gfh = { "#elif defined(CMDTBL_H_GTS_METHODS)\n\n" }
local hfh = { "#elif defined(CMDTBL_H_CMD_TBL_PTR_MACROS)\n\n" }
local cfh = { "#elif defined(CMDTBL_H_CMD_TBL_INITIALIZERS)\n\n" }

local hoist1 = "#define  " .. rpad( "", 4+maxMNameWidth() + 30 ) .. "   fn_"  -- const expr optzn
local idx = 0;
for ix, tb in tu.PairsBySortedKeys( tbl ) do  -- print( "-> \"" .. ix, tb.fxn .. "\"" )

   pfh[1+#pfh] = table.concat{ "   bool ", tb.fxn, "();\n" }

   local hasGTSfunc = tb.has_gts
   if hasGTSfunc then
      gfh[1+#gfh] = table.concat{ "   eRV ", tb.fxn, "();\n" }
      end

   hfh[1+#hfh] = table.concat{ "#define  pCMD_", rpad( tb.mname, maxNameWidth() ), "  (g_CmdTable + ", tostring(idx),")\n"
            , hoist1          , rpad( tb.mname, maxMNameWidth() ), "  (pCMD_", tb.mname,"->d_func)\n"
            }

   local fnmrpad = rpad( tb.fxn, maxfnmWidth() )
   local gtspfx = "&GTS::"
   cfh[1+#cfh] = table.concat{ "   { "
            , rpad( '"' .. ix .. '"', 2+maxNameWidth() )
            , ", ", "&ARG::"..fnmrpad
            , ", ", hasGTSfunc and (gtspfx..fnmrpad) or rpad( " nullptr", maxfnmWidth()+#gtspfx )
            , ", ", rpad( tb.attr, maxAttrWidth() )
            , " _AHELP( ", fmt( "%q", tb.help )," ) }, // CMD_", ix,"\n"
            }

   idx = idx + 1
   end

pfh = table.concat( pfh )
gfh = table.concat( gfh )
hfh = table.concat( hfh )
cfh = table.concat( cfh )

local ofh = assert( io.open( "cmdtbl.h" , "w" ) )

ofh:write( "// autogenerated; manual edits will be overwritten!\n"
   , "\n", pfh
   , "\n", gfh
   , "\n", hfh
   , "\n", cfh
   , "\n", "#else\n#error\n#endif", "\n"
   )

assert( ofh:close() )  -- almost entirely redundant, but ...
