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

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
module( "Menu", package.seeall )
-----------------------------------------------------------------------------

local function FgBg( fg, bg ) return _bin.bitor( _bin.shiftl(bg,4), fg ) end

local Blk=0 Blu=1 Grn=2 Cyn=3 Red=4 Pnk=5 Brn=6 LGry=7 DGry=8 LBlu=9 LGrn=0xA LCyn=0xB LRd=0xC LPnk=0xD Yel=0xE Wht=0xF

local isLinux = _dir.dirsep_os() == "/"

-- Default menu colors:

local mbg = Pnk

-- local color_popup = { frame=FgBg(Wht,Blk), header=FgBg(Blk,LGry), normal=FgBg(Wht,Blk), selected=FgBg(Wht,Grn) }
-- local color_popup = { frame=FgBg(Wht,mbg), header=FgBg(Blk,LGry), normal=FgBg(Wht,mbg), selected=FgBg(Wht,Grn) }
-- local color_popup = { frame=FgBg(Wht,mbg), header=FgBg(Blk,LGry), normal=FgBg(Wht,mbg), selected=FgBg(Wht,Grn) }
-- local color_popup = { frame=FgBg(Wht,mbg), header=FgBg(Blk,LGry), normal=FgBg(Wht,mbg), selected=FgBg(Blk,Yel) } -- long-time favorite
   local color_popup = { frame=FgBg(Wht,Grn), header=FgBg(Wht,Red ), normal=FgBg(Wht,Grn), selected=FgBg(Wht,LBlu) } -- for a change, Christmas!

local chHbar = "Í"
local ulc, urc = "É", "»"
local llc, lrc = "È", "¼"
local vert = "º"
local chevr, chevl = "¯ ", " ®"

if isLinux then
   chHbar = " "
   ulc, urc = " ", " "
   llc, lrc = " ", " "
   vert = " "
   chevr, chevl = "> ", " <"
   end

local MAX_DISP_COLS = 1024
local bar  = chHbar:rep( MAX_DISP_COLS )
local spcs =  (" "):rep( MAX_DISP_COLS )
local function pad( st, wid )
   if #st > wid then return st:substr( 1, wid ) end
   return st .. (" "):rep( wid - #st )
   end

local function vid_save()    HideCursor() end
local function vid_restore() UnhideCursor()  DirectVidClear()  DispRefreshWholeScreenNow() end
local function vid_wrYX(str,yLine,xCol,color) -- the key that unlocks pandora's box 09-Jan-2002 klg
   -- DBG( "X,Y=(".. xCol .. "," .. yLine .. ")="..str.."|" )
   DirectVidWrStrColorFlush( yLine, xCol, str, color )
   end


local function prepTitle( str, len, fillch )
   str = str or ""
   local pre,post = ulc, urc
   if #str < len then
      pre  = pre .. fillch:rep( int((len - #str)/2) )
      post = fillch:rep( int((len - #str)/2) + int((len - #str)%2) ) .. post
      end
   return pre, str, post
   end

local MenuProto_ = { extra = 4 }  -- prototype-object for Menu instances
function new(obj)
   obj = obj or {}
   setmetatable( obj, MenuProto_ )
   MenuProto_.__index = MenuProto_
   return obj
   end

function MenuProto_:prepAboutChoices()
   self.opt = self.opt or {}
   self.color = self.color or FgBg( Wht, Brn )
   self.aboutChoices = {}
   local ac = self.aboutChoices
   local choices_ = {}
   for i, val in ipairs( self.choices ) do  -- Msg( i .. ":".. val )
      choices_[1+#choices_] = ifx( type(val) == "table", val[1], val )
      end

   local max = 0
   for i, val in ipairs( choices_ ) do  -- Msg( i .. ":".. val )
      -- val = val:gsub( "&", "" )    -- more on this later
      choices_[i] = val
      max = Max( max, #val + self.extra )
      end

   ac.title = self.title
   if ac.title then
      ac.title = " " .. ac.title .. " "
      max = Max( max, #ac.title )
      end

   ac.count  = #choices_
   ac.maxlen = max
   ac.minVisibleChoice  = 1
   ac.numVisibleChoices = Min( ac.count, ScreenLines() - 6 )  -- 6 = 2 (frame) + 2 (top & bottom gap) + 2 (dialog & status lines)

   -- center the menu's window
   ac.minY = int( (ScreenLines() - ac.numVisibleChoices )/2-2 )  -- "-2" magic number to position max menu window correctly
   ac.minX = int( (ScreenCols()  - ac.maxlen            )/2 )

   ac.titPre, ac.title,ac.titPost = prepTitle( ac.title, ac.maxlen, chHbar )

   -- Create self.paddedChoices array containing all choices padded to aboutChoices.maxlen
   self.paddedChoices = {}
   local wid = ac.maxlen-self.extra
   for ix, val in ipairs( choices_ ) do
      self.paddedChoices[ix] = self.opt.center_choices
         and pad( (" "):rep( (wid - #val)/2 ) .. val, wid )
         or  pad( val, wid )
      end

   -- self.keyTbl = {}
   -- for ix,choice in ipairs(choices_) do
   --    if choice:match("^~") then
   --       par[ix] = pad(substr(choices_[ix],2),width)
   --       firstselect++
   --    else
   --       # Log( "par["ix"]="pad(choices_[ix],width) )
   --
   --       par[ix] = pad(choices_[ix],width)
   --
   --       # 06-Oct-2002 klg
   --       # keyTbl is indexed by first char (capital) key,
   --       #
   --       # value is,
   --       # if positive, choice number uniquely identified by key
   --       # if negative the FIRST choice number, when key does NOT uniquely identify a single choice
   --       #
   --       if choice:match( /([a-z0-9])/i ) then
   --          key = toupper( substr( choices_[ix], RSTART, 1 ) )
   --          if keyTbl[key] then
   --             if keyTbl[key] > 0 then
   --                keyTbl[key] = -keyTbl[key]  # negative value means more than one match
   --                # Log( "keyTbl["key"]="keyTbl[key] )
   --                end
   --          else
   --             keyTbl[key] = i - firstselect
   --             # Log( "keyTbl["key"]="keyTbl[key] )
   --             end
   --          end
   --       end
   --    end

   end


function MenuProto_:frame()
   local color = color_popup.frame
   local ac = self.aboutChoices
   if #ac.title == 0 then
      vid_wrYX( ac.titPre..ac.titPost, ac.minY, ac.minX, color )
   else
      vid_wrYX( ac.titPre  , ac.minY , ac.minX                      , color )
      vid_wrYX( ac.title   , ac.minY , ac.minX+#ac.titPre           , color_popup.header )
      vid_wrYX( ac.titPost , ac.minY , ac.minX+#ac.titPre+#ac.title , color )
      end
   local hfrm = vert .. spcs:sub(1,ac.maxlen) .. vert
   for curY = ac.minY + 1, ac.minY + ac.numVisibleChoices do  vid_wrYX( hfrm, curY, ac.minX, color )  end
   vid_wrYX(    llc .. bar:sub(1,ac.maxlen)  .. lrc, ac.minY + ac.numVisibleChoices + 1, ac.minX, color )
   end


function MenuProto_:update( curChoice )
   local ac = self.aboutChoices
   local curY = ac.minY + 1
   for ix = ac.minVisibleChoice, ac.minVisibleChoice + ac.numVisibleChoices - 1 do
      local color, ls, rs = color_popup.normal, "  ", "  "
      if ix == curChoice then color = color_popup.selected  ls,rs = chevr, chevl end
      vid_wrYX( ls..self.paddedChoices[ ix ]..rs, curY, ac.minX+1, color )
      curY = curY + 1
      end
   -- DispRefreshWholeScreenNow()
   end


-- Draw a popup menu containing the items in the menyarray and wait
-- for the user to select an item from the menu.  Return the number
-- of the chosen item, or nil if the user pressed ESC.
--
-- parameter initialChoice is optional
--
-- An "&" in a menu item indicates that the letter following & will
-- select that item rather than the first letter.  The & are not displayed.

function MenuProto_:PickOne( initialChoice )
   vid_save()
   self:prepAboutChoices()
   self:frame()

   local ac = self.aboutChoices
   local curChoice,theChoice = initialChoice or 1
   while true do
      self:update( curChoice )  -- redraw the contents of the menu
      local  key = GetKey()
   -- DBG(  "KEY="..key )
      if     key=="esc"   then                       break
      elseif key=="enter" then theChoice = curChoice break
      elseif key=="up"    then curChoice = curChoice - 1
      elseif key=="down"  then curChoice = curChoice + 1
      elseif key=="pgup"  then curChoice = curChoice - (ac.numVisibleChoices-1)
      elseif key=="pgdn"  then curChoice = curChoice + (ac.numVisibleChoices-1)
      elseif key=="home"  then curChoice = 1
      elseif key=="end"   then curChoice = ac.count
   -- elseif key[keyTbl] then  -- TBD
   --    Log( "hit: keyTbl["..key.."]="..keyTbl[key] )
   --    if     keyTbl[key] <  0 then curChoice = -keyTbl[key]+1
   --    elseif keyTbl[key] >= 0 then theChoice =  keyTbl[key]+1  break
   --       end
      else Bell() -- Beep for unrecognized keyboard input.
         end

   -- DBG( curChoice..", "..ac.count )
      curChoice = Max( curChoice, 1 )
      curChoice = Min( curChoice, ac.count )

      ac.minVisibleChoice = Max( ac.minVisibleChoice, curChoice - (ac.numVisibleChoices - 1) )
      ac.minVisibleChoice = Min( ac.minVisibleChoice, curChoice )

   -- DBG( "curChoice="..curChoice..", minVisibleChoice="..ac.minVisibleChoice )
      end

   vid_restore()

   if not theChoice then return end

   -- ok, user made a valid choice; return index-of-choice, chosen-value
   if type(self.choices[theChoice]) == "table" then                -- self.choices is an array of 2-element arrays ([1]=displayed choice, [2]=value chosen [hidden])
      --     index-of-choice             chosen-value
      --     |                           |
      return self.choices[theChoice][1], self.choices[theChoice][2]
      end

   return    theChoice                 , self.choices[theChoice]   -- self.choices is an array of strings, each is BOTH displayed choice AND value chosen
   end
