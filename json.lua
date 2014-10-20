-- 20101025 kgoodwin  head explody!

-- http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html
-- http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html#grammar

-- http://www.facepunch.com/threads/750140-gm_lpeg-Epic-pattern-shit?p=15553820&viewfull=1#post15553820

require'lpeg'

local P  = lpeg.P  --
local R  = lpeg.R  -- range
local S  = lpeg.S  -- set
local V  = lpeg.V  -- reference rule in grammar
local C  = lpeg.C  -- Capture the match(es) of the parameter pattern(s)?
local Cc = lpeg.Cc -- Capture pattern-time constant (matches the empty string)
local Ct = lpeg.Ct
local Cg = lpeg.Cg
local Cf = lpeg.Cf
local Cs = lpeg.Cs

lpeg.locale(lpeg)  -- injects alnum, alpha, cntrl, digit, graph, lower, print, punct, space, upper, xdigit, each one containing a correspondent pattern, into the passed table.

local _ = lpeg.space^0  -- at least 0 repetitions of space
local X = lpeg.xdigit
local escapes = {t='\t', n='\n', f='\f', r='\r', b='\b', v='\v'}
local null = newproxy()
local function char_escape(chars) --[[io.write("char_escape(",chars,")\n")]] return string.char(tonumber(chars, 16)) end

local function visit(tb, fx, done)
   done = done or {}
   for k,v in pairs(tb) do
      if type(v) == "table" then
         if not done[v] then
            done[v] = true
            visit(v, fx, done)
            end
      else
         fx(tb,k)
         end
      end
   return tb
   end
local function strip_nulls(tb)
   return visit( tb, function(tb,k) if tb[k] == null then tb[k] = "nil" end end )
   end

local function process_array(ar)
   local rv = {}
   for i, v in ipairs(ar) do
      if v ~= null then rv[1+#rv] = v end
      end
   return rv
   end

local json_grammar = P{ -- this table contains a grammar
   -- Match space, followed by a value, followed by space. This is the initial thing to match.
   _*C(V'value')*_,

   value = V'string'+V'object'+V'number'+V'array'+V'constant',

   -- Match "string": value in a capture group. The string and value, as they are captured apropriately in this patterns, get passed to the Cf below.
   kvpair = Cg(V'string'*_*':'*_*V'value'),

   object =
      '{'*_*Cf(   -- Match "{", followed by a 'folding pattern'. It calls 'rawset' with any captures it gets. BUT, because there is a capture group above, those two will be passed together.
         Ct''     -- doing this will create a blank table. This is the table to push our data to. The '' is so that it matches, well, 'nothing'. And just always captures a blank table.
         *((V'kvpair'*(_*','*_*V'kvpair')^0)+0)  -- This matches (a kvpair followed by any number of (a comma then a kvpair)) or nothing
         , rawset  -- The blank table is the first value to pass to rawset. The folding capture goes: rawset(rawset(rawset(t, 1, 'a'), 2, 'b'), 3, 'c'). And because rawset returns the table, it all works out!
         )*_*P','^-1
         *_*'}',  -- Allow for up to 1 trailing comma, then match }

   array = '['*_*Ct(  -- Match [, followed by a new 'table capture'. It basicaly inserts every capture into the table, numbered by order captured.
      V'value'*(_*','*_*V'value')^0+1  -- match a value followed by any number of (a comma and then a value)
      )/process_array    -- Pass this table to the process_array function which will make it purty.
      *_*P','^-1*_*']',  -- Allow for up to 1 trailing comma, then match ]

   escape = P"\\"/""*(       -- Match \, and replace it with a blank string in the end result.
         S'"\\/bfnrtv'/escapes  -- Then match a basic escape, and replace it with the appropriate value in the end result.
         +'u'*C(X*X*X*X)/char_escape),  -- OR, match u, and capture the 4 hex digits, then pass the captured digits to char_escape, and replace the end result with the return value.

   string = '"'*Cs((V'escape'+1-'"')^0)*'"',  -- Match ", followed by any number of (escape sequence OR one character) then ". Use lpeg.Cs because we want the replaces in escape to work (the / ones)

   number = C(P'-'^-1*R'09'^1*('.'*R'09'^1)^-1*    -- Capture (a potential minus sign, then at least one digit, then potentially (a dot, and then at least one digit)...
              (S'Ee'*S'+-'^-1*R'09'^1)^-1)/tonumber, -- and then potentially (an E or an e, then potentially (a + or a -), then at least one digit)

   constant = "true"*Cc(true)+"false"*Cc(false)+"null"*Cc(null),  -- (Match 'true', and capture true) OR (Match 'false' and capture false) OR (Match 'null' and capture a special null value, which is later turned to nil)
   }

module("json")
function decode(str)
   local index, res = json_grammar:match(str)
   return strip_nulls(res)
   end
