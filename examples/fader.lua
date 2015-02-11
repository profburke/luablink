#!/usr/bin/env lua

local blink = require 'blink'


if blink.enumerate() == 0 then
   return
end

local d = blink.open()

print(d)

d:fade(100, 255, 0, 0, 0)
blink.sleep(200)

d:fade(100, 0, 255, 0, 1)
blink.sleep(200)

d:fade(100, 0, 0, 255, 2)
blink.sleep(200)

while 1 do end


