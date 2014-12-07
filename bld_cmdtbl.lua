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
local foobar = {win32="w",other=""}
local w_only = assert( foobar[platform_arg], platform_arg.." is unknown platform_arg" )

-- io.write( "w_only = '", w_only,"'","\n" )  os.exit(1)

local maxAttrWidth  = 0
local maxfnmWidth   = 0
local maxNameWidth  = 0
local maxMNameWidth = 0
local tbl = {}

for line in io.lines() do
   line = line:gsub( "#.*$", "" )
   if #line > 0 then
      local ch1 = line:sub( 1,1 )  -- io.write( "ch1 = '", ch1,"'","\n" )
      if ch1 == " " or ch1 == w_only then
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
         tbl[name] = {
              fxn   = fxn
            , attr  = attr_expr
            , help  = helpString
            , mname = mname
            }

         if maxNameWidth  < #name      then maxNameWidth  = #name      end
         if maxMNameWidth < #mname     then maxMNameWidth = #mname     end
         if maxAttrWidth  < #attr_expr then maxAttrWidth  = #attr_expr end
         if maxfnmWidth   < #fxn       then maxfnmWidth   = #fxn       end
         end
      end
   end

-- this prog used to write 4 _output files_, but standard GNU make has completely brain-dead non-support for
-- rules having multiple targets, and the only published workaround turns any incremental build into a FULL build.
-- Therefore I'm modifying this program to generate a SINGLE output file whose sections are bracketed by
-- C preprocessor conditional #if ... #endif
-- 20131013 kgoodwin

local pfh = { "#ifdef CMDTBL_H_ARG_METHODS\n" }
local hfh = { "#ifdef CMDTBL_H_CMD_TBL_PTR_MACROS\n" }
local cfh = { "#ifdef CMDTBL_H_CMD_TBL_INITIALIZERS\n" }

local hoist1 = "#define  " .. rpad( "", 4+maxMNameWidth + 30 ) .. "   fn_"  -- const expr optzn
local idx = 0;
for ix, tb in tu.PairsBySortedKeys( tbl ) do  -- print( "-> \"" .. ix, tb.fxn .. "\"" )

   pfh[1+#pfh] = table.concat{ "   bool ", tb.fxn, "();\n" }

   hfh[1+#hfh] = table.concat{ "#define  pCMD_", rpad( tb.mname, maxNameWidth  ), "  (g_CmdTable + ", tostring(idx),")\n"
            , hoist1          , rpad( tb.mname, maxMNameWidth ), "  (pCMD_", tb.mname,"->d_func)\n"
            }

   cfh[1+#cfh] = table.concat{ "   { "
            , rpad( '"' .. ix .. '"', 2+maxNameWidth ), ", &ARG::"
            , rpad( tb.fxn,  maxfnmWidth  ), ", "
            , rpad( tb.attr, maxAttrWidth )
            , " _AHELP( ", fmt( "%q", tb.help )," ) }, // CMD_", ix,"\n"
            }

   idx = idx + 1
   end

pfh[1+#pfh] = "#endif"
hfh[1+#hfh] = "#endif"
cfh[1+#cfh] = "#endif"

pfh = table.concat( pfh )
hfh = table.concat( hfh )
cfh = table.concat( cfh )

local ofh = assert( io.open( "cmdtbl.h" , "w" ) )

ofh:write( "// autogenerated; manual edits will be overwritten!"
   , "\n\n", pfh
   , "\n\n", hfh
   , "\n\n", cfh
   )

assert( ofh:close() )  -- almost entirely redundant, but ...