--
-- Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

--[[

this file should contain all user or host-system customizations

]]

   add_diff_edfxn( "difg" , ""         , "c:/_tools/diffmerge/sgdm.exe"      , StartGuiProcess )
   add_diff_edfxn( "difk" , ""         , "kdiff3.exe"                        , StartGuiProcess )  -- assumed in PATH as part of TortHg pkg
   add_diff_edfxn( "tc2"  , ""         , "c:/totalcmd/totalcmd.exe /N"       , StartGuiProcess )
-- add_diff_edfxn( "difg" , "DIFFGUI"  , "c:/klg/bin/windiff.exe"            , StartGuiProcess )
   add_diff_edfxn( "difc" , "DIFFCUI"  , "c:/klg/bin/unxutils/diff.exe -cbB" , runEmbShell     )
   add_diff_edfxn( "difb" , "DIFFCBIN" , "c:/klg/bin/unxutils/cmp.exe -l"    , runEmbShell     )

do

   local k_scripts = "c:/klg/scripts"

   local KINIT,kroot,fkroot = GetenvOrNil( "KINIT" ) -- KINIT is set by the editor C++ startup code.
   if KINIT then
       kroot = KINIT:gsub( "\\", "/" )
      fkroot = "file://"..kroot
      end

   local function init_choices()
      local rv = {}
      local function AddInDir( dirnm, ... )
         if dirnm and IsDir( dirnm ) then
            for _,st in ipairs{...} do
               if type(st)=="string" then
                  local val = dirnm.."/"..st
                  rv[1+#rv] = val:gsub( "\\", "/" )
               elseif type(st)=="table" then
                  rv[1+#rv] = st
                  end
               end
            end
         end

      do
      if kroot then
         AddInDir(                                 kroot
                 , { "Lua Reference Manual"     , fkroot.."lua-5.1/doc/contents.html" }
                 , { "std::basic_string_view Manual", "https://en.cppreference.com/w/cpp/string/basic_string_view" }
                 , { "std::string_view Manual"  , "https://en.cppreference.com/w/cpp/header/string_view" }
                 , { "std::basic_string Manual", "https://en.cppreference.com/w/cpp/string/basic_string" }
                 , { "Lua PiL 2e"               , fkroot.."lua-5.1/doc/Programming.in.Lua.2e.pdf" }
                 , { "Lpeg Reference Manual"    , fkroot.."lua-5.1/src/lpeg.html"     }
                 , { "Lpeg.re Reference Manual" , fkroot.."lua-5.1/src/re.html"       }
                 )
         end
      end

      AddInDir( k_scripts, "*.kdb", "*.txt", "home.*", "cpl*.bat" )

      rv[1+#rv] = "%KINIT%"
      rv[1+#rv] = "%USERPROFILE%"
      rv[1+#rv] = "%USERPROFILE%/.gitconfig"
      rv[1+#rv] = "%USERPROFILE%/Mercurial.ini"
      rv[1+#rv] = "%USERPROFILE%/Application Data/Subversion/config"
      rv[1+#rv] = "%USERPROFILE%/MP3/*.m3u|"
      rv[1+#rv] = "%APPDATA%/k_edit"
      local fwtwiki = "http://fwtwiki.nowhere.com/bin/view/BuildTools/"
      rv[1+#rv] = {"FwTWiki", fwtwiki}
      rv[1+#rv] = {"FwTWiki:ArzProcessorCoreManuals", fwtwiki.."ArzProcessorCoreManuals"}
      rv[1+#rv] = "%SystemRoot%"
      rv[1+#rv] = "%SystemRoot%/system32/*.NT"

      local GA = GetenvOrNil( "WANDISK_GCCARM" )
      AddInDir( GA, "share/doc/gcc-arm-none-eabi/pdf/*.pdf", "share/doc/gcc-arm-none-eabi/*.txt" )

      local ST,SDT = GetenvOrNil( "WANDISK_TOOLS_WS" ), GetenvOrNil( "WANDISK_DBG_TOOLS_WS" )
      AddInDir( ST, "ARC/MetaWare/arc/docs/pdf/*.txt", "ARC/MetaWare/arc/docs/pdf/*.pdf" )
      AddInDir( SDT and (SDT ~= ST) and SDT, "ARC/MetaWare/arc/docs/pdf/*.txt", "ARC/MetaWare/arc/docs/pdf/*.pdf" )
      AddInDir( "c:/program files/accurev/doc", "*.txt", "*.pdf" )
      AddInDir( "e:/_downloads/ARC/ARC700-manual-set", "*.pdf" )
      return rv
      end

   local ff_choices = {}
   AddEdFxn{ name = "ff", help = "Menu: Favorite Files", attr = NOARG, key = ""
      , NOARG = function ( arg )
         ff_choices = #ff_choices > 0 and ff_choices or init_choices()
         local _,fspec = Menu.new{ title="Favorite Files", choices=ff_choices }:PickOne()
         if fspec then
            setfile(fspec)
            end
         end
      }
end

local ebySrchHdr         = "http://search.ebay.com/search/search.dll?MfcISAPICommand=GetResult&SortProperty=Meta"
local ebySpecEndingFirst = "EndSort"
local ebySpecNewlyListed = "NewSort&SortOrder=d&pb=&ebaytag1=ebayreg&ht=1"
local ebyTail1           = "&query="
local ebyTail2           = "&ebaytag1code=0"

local googl = "https://www.google.com/"
local function goSiteSrch( site )
   return goUrl( googl.."custom?q=".. UrlSrchTag .."&sa=%C2%BB&domains="..site.."&sitesearch="..site )
 --return goUrl( googl.."webhp?hl=en#?q=site:"..site.."+".. UrlSrchTag )
 --return goUrl( googl.."custom?q=".. UrlSrchTag .."&sa=%C2%BB&domains="..site.."&sitesearch="..site )
   end

local URL_StartPage      = goUrl( "https://startpage.com/do/search?q=".. UrlSrchTag )
-- local URL_Google         = goUrl( googl.."search?hl=en&as_q=" .. UrlSrchTag )
local URL_Google         = goUrl( googl.."search?q=" .. UrlSrchTag )
local URL_HomeServer     = goUrl( "https://files.home.arpa/cgi/search-files?Submit=Find+Matches&search_keys=" .. UrlSrchTag )
local URL_Epinions       = goSiteSrch( "www.epinions.com" )
local URL_Bing           = goUrl( "http://www.bing.com/search?q=" .. UrlSrchTag )
local URL_DuckDuckGo     = goUrl( "https://duckduckgo.com/search?q=" .. UrlSrchTag )
local URL_DDGRG          = goUrl( "https://duckduckgo.com/?q=site%3Arapidgator.net+" .. UrlSrchTag..'&t=ffab&ia=web' )
local URL_Metafilter     = goSiteSrch( "ask.metafilter.com" )
local URL_GoogBobOil     = goSiteSrch( "bobistheoilguy.com" )
local URL_GoogAdvRider   = goSiteSrch( "www.advrider.com" )
local URL_GoogAdvRiderXRL= goUrl( googl.."search?q=site%3Aadvrider.com%2Fforums%2Fshowthread.php+inurl%3A%22t%3D114834%22" .. UrlSrchTag )
local URL_Thumpertalk    = goSiteSrch( "www.thumpertalk.com" )
local URL_canistream_it  = goUrl( "http://www.canistream.it/search/term/" .. UrlSrchTag )
local URL_Wikipedia      = goUrl( "http://en.wikipedia.org/wiki/" .. UrlSrchTag )
local URL_Ebay           = goUrl( ebySrchHdr .. ebySpecEndingFirst .. ebyTail1 .. UrlSrchTag .. ebyTail2 )
local URL_Ebay_NewList   = goUrl( ebySrchHdr .. ebySpecNewlyListed .. ebyTail1 .. UrlSrchTag .. ebyTail2 )
local URL_Craigslist     = goUrl( "http://sfbay.craigslist.org/search/sss?query=" .. UrlSrchTag )
local URL_OpenBSD_man    = goUrl( "http://www.openbsd.org/cgi-bin/man.cgi?query=" .. UrlSrchTag )
local URL_RvsDictionary  = goUrl( "http://onelook.com/?w=*&?loc=revfp&clue=" .. UrlSrchTag )
local URL_GoogleMaps     = goUrl( "http://maps.google.com/maps?f=q&hl=en&q=" .. UrlSrchTag .. "&btnG=Search" )
local URL_GoogleNews     = goUrl( "http://news.google.com/news?hl=en&q=" .. UrlSrchTag .. "&btnG=Search+News" )
local URL_GoogleCode     = goUrl( googl.."codesearch?q=" .. UrlSrchTag .. "&btnG=Search+Code" )

local function clean_mobi(st)
   local ar = {}
   for word in st:gmatch "%w%w+" do  -- NB: only 2+-letter words are passed thru!
      ar[1+#ar] = word
      end
   return table.concat( ar, ' ' )
   end

local URL_MobiSrchAB     = goUrl( 'https://forum.mobilism.org/search.php?keywords='..UrlSrchTag..'&fid%5B%5D=124&sr=topics&sf=titleonly', clean_mobi )

AddEdStringFxn( "google"         , URL_Google        )
AddEdStringFxn( "searchhomeserver", URL_HomeServer   )
AddEdStringFxn( "DuckDuckGo"     , URL_DuckDuckGo    )
AddEdStringFxn( "ddgrg"          , URL_DDGRG         )
AddEdStringFxn( "bobistheoilguy" , URL_GoogBobOil    )
AddEdStringFxn( "bing"           , URL_Bing          )
AddEdStringFxn( "thumpertalk"    , URL_Thumpertalk   )
AddEdStringFxn( "mefi"           , URL_Metafilter    )
AddEdStringFxn( "canistream.it"  , URL_canistream_it )
AddEdStringFxn( "wikipedia"      , URL_Wikipedia     )
AddEdStringFxn( "ebay"           , URL_Ebay          , "alt+7" )
AddEdStringFxn( "craigslist"     , URL_Craigslist    )
AddEdStringFxn( "openbsd_man"    , URL_OpenBSD_man   )
AddEdStringFxn( "rvsdictionary"  , URL_RvsDictionary )
AddEdStringFxn( "googlemaps"     , URL_GoogleMaps    )
AddEdStringFxn( "googlenews"     , URL_GoogleNews    )
AddEdStringFxn( "googlecode"     , URL_GoogleCode    )
AddEdStringFxn( "adv"            , URL_GoogAdvRider  )
AddEdStringFxn( "ab"             , URL_MobiSrchAB    , "alt+7" )

local function frhed(fn) StartGuiProcess( '"c:/_tools/frhed/frhed.exe" ' .. fn ) end
AddEdFxn{ name = "hex"                                                       ,
          help = "open named or current file in hex editor (frhed)"          ,
          attr = BOXSTR+TEXTARG+NOARG                                        ,
          TEXTARG = function( arg ) return frhed( arg.text ) end             ,
          NOARG   = function( arg ) return frhed( GetCurrentFilename() ) end ,
        }

do
   local googMenu = {
        title = "Web Search"
      , choices = {
          default = 1
        , { "Google"            , URL_Google        }
        , { "Home-Server"       , URL_HomeServer    }
        , { "StartPage"         , URL_StartPage     }
        , { "DuckDuckGo"        , URL_DuckDuckGo    }
        , { "s=epinions"        , URL_Epinions      }
        , { "bobistheoilguy"    , URL_GoogBobOil    }
        , { "Bing"              , URL_Bing          }
        , { "ADVrider"          , URL_GoogAdvRider  }
        , { "ADVrider-XRL"      , URL_GoogAdvRiderXRL  }
        , { "ThumperTalk"       , URL_Thumpertalk   }
        , { "Metafilter"        , URL_Metafilter    }
        , { "canistream.it"     , URL_canistream_it }
        , { "Wikipedia"         , URL_Wikipedia     }
        , { "EBay"              , URL_Ebay          }
        , { "Craigslist"        , URL_Craigslist    }
        , { "OpenBSD man"       , URL_OpenBSD_man   }
        , { "reverse dictionary", URL_RvsDictionary }
        , { "Google Maps"       , URL_GoogleMaps    }
        , { "Google News"       , URL_GoogleNews    }
        , { "Google code"       , URL_GoogleCode    }
        }
      }

   AddEdFxn{ name = "websearch", help = "do web searches", attr = BOXSTR+TEXTARG, key = "alt+6"
      , TEXTARG = function ( arg ) OpenUrlOrMenu( arg, googMenu ) end
      }
end

function flatten( ary, rv )
   rv = rv or {}
   for _,val in ipairs( ary ) do
      if type(val) == "table" then flatten( val, rv )
      else
         rv[1+#rv] = val
         end
      end
   return rv
   end

local ww_menu
do
   local dirsep    = "/"  -- I prefer using "/" since I don't have to double it; however editor wc expansion results always contain "\"
   local wc_c      =        { "*.c|", "*.cpp|" }
   local wc_h      =        { "*.h|" }
   local wc_ch     = flatten{ wc_h, wc_c }
   local wc_py     =        { "*.py|" }
   local wc_allflat=        { "*" }
   local wc_ch_py  = flatten{ wc_ch, wc_py }
   local wc_ch_lua = flatten{ wc_ch, "*.lua|" }

   local function djoin( dir, suffix ) return (dir..dirsep..suffix):gsub("[\\/]+",dirsep) end

   local function a_djoin( wsdir, a_dirs )
      local rv = {}
      for _,val in ipairs( a_dirs ) do rv[1+#rv] = djoin( wsdir, val ) end
      return rv
      end

   local function infx( wsdir )
      if wsdir==nil or #wsdir==0 then
         return function( ar ) return               ar                                                 end
      else
         return function( ar ) return "table"==type(ar) and a_djoin( wsdir, ar ) or djoin( wsdir, ar ) end
         end
      end

   local function dXw( a_dir, a_wc )
      return function()
         local rv = {}
         for _,dir in ipairs(a_dir) do
            for _,wc in ipairs(a_wc) do
               rv[1+#rv] = djoin( dir, wc )
               end
            end
         return rv
         end
      end

   local ignore_dirs = { "\\%.kbackup\\?", "\\%.git\\", "\\%.hg\\", "\\%.svn\\" }

   local function ArzFw( wsd )
      local wsTD = wsd("")
      if wsTD=="" then
         wsTD = GetenvOrNil( "WS_TOPDIR" ) or _dir.name_isfile( "pset.bat" ) and ".."  -- ".." is best guess; sometimes good enuf when running on foreign PC's (shell cwd = \.\Make)
         wsd = infx( wsTD )
         end
      if wsTD and _dir.name_isfile( wsd( "pset.bat" ) )  then
         return {
              title = "Workspace Wildcards"
            , choices = {
                   default = 1
                 , { "<bpng>"         , { wsd{ "Make/FwBldScripts/*.lua", "Make/FwBldScripts/*.bat", "Make/FwBldScripts/*.config", "Make/linker_scripts/*|", "Source/*ip.config|", "Source/Product/*.config", },ignore_dirs} }
                 , { "<srcs>"         , { wsd{ "Source/*|", },ignore_dirs} }
                 , { "<srcs_rl>"      , { wsd{ "Source/*|", "ROM_lib/*.c|", "ROM_lib/*.h|", "ROM_lib/*.asm|", "ROM_lib/*.s|", "ROM_lib/*.inc|", },ignore_dirs} }
                 , { "<cmd>"          , { wsd{ "*.cmd|" },ignore_dirs} }
                 , { "<output map>"   , { wsd{ "output/*.map|" },ignore_dirs} }
                 , { "<output dmp>"   , { wsd{ "output/*.dmp|" },ignore_dirs} }
                 , { "<output mak>"   , { wsd{ "output/*.mak|" },ignore_dirs} }
                 , { "<mak>"          , { wsd{ "*.mak|", "*.mik|" },ignore_dirs} }
                 , { "<build.configs>", { wsd{ "build.config|" },ignore_dirs} }
                 , { "<ogn>"          , { wsd{ "*.ogn|" },ignore_dirs} }
                 }
            }
         end
      end

   local function Keditor( wsd )
      if _dir.name_isfile( wsd("k.cpp") ) then
         local dirs = wsd{ ".", "lua-5.1/src" }
         return {
              title = "K editor wildcards"
            , choices = {
                   default = 1
                 , { "<ch>"     , { dXw( dirs, wc_ch     ) , ignore_dirs } }
                 , { "<c>"      , { dXw( dirs, wc_c      ) , ignore_dirs } }
                 , { "<h>"      , { dXw( dirs, wc_h      ) , ignore_dirs } }
                 , { "<ch_lua>" , { dXw( dirs, wc_ch_lua ) , ignore_dirs } }
                 }
            }
         end
      end

   ww_menu = function()
      local wsd = infx()
      for _,fxn in ipairs
         {
         ArzFw    ,
         Keditor  ,
         } do
         local rv = fxn( wsd )
         if rv then return rv end
         end
      end
end

AddEdFxn{ name = "ww", help = "Workspace Wildcards", attr = NOARG+TEXTARG+BOXSTR, key = ""
   , NOARG = function ( arg )
      local amenu = ww_menu()
      if amenu then
         local destnm,a_wc = Menu.new( amenu ):PickOne()
         if destnm then
            local one = a_wc[1]
            if type(one)=="function" then one = one() end
            MultiWcx( destnm, one, a_wc[2] ):PutFocusOn()
            fExecute("mfx")
            end
         end
      end
   , TEXTARG = function ( arg )
      local nm = arg.text
      local amenu = ww_menu()
      if amenu then
       --print( "ww:TEXTARG "..nm )
         for _, ary in ipairs(amenu.choices) do  -- linear search :( since menu choice order !necessarily == choice SORT-order
            if nm == ary[1] then
               MultiWcx( ary[1], ary[2][1], ary[2][2] ):PutFocusOn()
               return true
               end
            end
         Msg( "ww(",nm,") matched nothing" )
         end
      end
   }

do

   -- rc_ = "rare choice ..."

   local function rc_stash( arg )
      local max_cmdexe_cli = 8*1024
      local bc = boxContent( arg )
      local tail = table.concat(bc," ")
            tail = tail:gsub("%s%s+"," ")
      local  stashdir = GetenvOrNil( "STASHDIR" )
      if not stashdir then return Msg("envvar STASHDIR not defined" ) end
             stashdir = stashdir:gsub("[\\/]?$","\\")
      local wsnm     = GetenvOrNil( "WS_NAME" )
      local wsroot   = GetenvOrNil( "WS_TOPDIR" )
      local dc = "" -- os.date("%Y%m%d_%H%M%S")
      local cdcmd,destfnm = ""
      if wsnm and wsroot then
         cdcmd = "cd "..wsroot.."&&"
         destfnm = wsnm.."_"..dc.."_"..tostring(#bc)
      else
         local cwd = GetCwd()
         cwd = cwd:gsub("[\\/:]","-")
         destfnm = cwd.."_"..dc.."_"..tostring(#bc)
         end

      local cmd = cdcmd.."zip.exe "..stashdir..destfnm..".zip "..tail
      if #cmd > max_cmdexe_cli then
         Msgf("stash: cmdline too long (%d > %d)!", #cmd, max_cmdexe_cli )
      else
         runchild( "EMBSHELL>" .. cmd )
         end
      end

   local rc_choices = {
        { "stash (BOX/LINE)"        , rc_stash    },
        { "show stash content (NO)" , function() runchild "EMBSHELL>stashes.bat" end },
      }
   local function rare( arg )
      local _,fxn = Menu.new{ title="Rare commands", choices=rc_choices }:PickOne()
      if fxn then  fxn(arg)  end
      end

   AddEdFxn{ name = "rare", help = "Menu: Rare commands", attr = NOARG+TEXTARG+BOXSTR+NULLARG+NULLEOL+BOXARG+LINEARG, key = "alt+/"
      , NOARG   = rare
      , BOXARG  = rare
      , TEXTARG = rare
      , LINEARG = rare
      }

end

do
   --[[
   home

   ]]
end


return {
   URL_HomeServer = URL_HomeServer,
   }
