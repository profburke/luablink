#!/usr/bin/env lua

local blink = require 'blink'

local function doit(d, reps, time)
   time = time or 300
   for i = 1,reps do
      d:red(); blink.sleep(time); d:blue(); blink.sleep(time);
   end
   d:off()
end


local d = blink.open()

if d then
   doit(d, 4)
end

