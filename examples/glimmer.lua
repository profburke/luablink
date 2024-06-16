#!/usr/bin/env lua

local blink = require 'blink'

-- Lua implementation of the glimmer C function
-- in the blink1-tool source.
local function glimmer(d, n, r, g, b)
   n = n or 3
   r = r or 127
   g = g or 127
   b = b or 127

   local delayMillis = 500
   local millis = 300
      
   for i = 1,n do
      d:fade(millis, r, g, b, 1)
      d:fade(millis, r/2, g/2, b/2, 2)
      blink.sleep(delayMillis/2)
      
      d:fade(millis, r/2, g/2, b/2, 1)
      d:fade(millis, r, g, b, 2)
      blink.sleep(delayMillis/2)
   end

   d:fade(millis, 0, 0, 0, 1)
   d:fade(millis, 0, 0, 0, 2)
end

local d = blink.open()

if d then
   glimmer(d, 5,  120, 230, 90)
end
