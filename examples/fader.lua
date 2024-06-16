#!/usr/bin/env lua

-- A blink(1) has two LEDs which we will refer to
-- as _top_ and _bottom_.  The `set` command will
-- immediately display the given color on both LEDs,
-- however, the `fade` command allows you to address
-- the LEDs individually.

local blink = require 'blink'

if blink.enumerate() == 0 then
   return
end

local d = blink.open()

-- Set top and bottom LEDs to red.
-- Do so over 100 milliseconds.
d:fade(100, 255, 0, 0, 0)
-- Wait for 500 milliseconds.
blink.sleep(500)

-- Set just the bottom LED to green.
d:fade(100, 0, 255, 0, 1)
blink.sleep(500)

-- Set just the top LED to blue.
d:fade(100, 0, 0, 255, 2)
blink.sleep(3000)

-- Turn both LEDs off.
d:off()


