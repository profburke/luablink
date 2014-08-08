#!/usr/bin/env lua -i

local lblink = require 'lblink'

print('LuaBlink version: ' .. lblink._VERSION)
print "\n"
print('VID: ' .. lblink.VID)
print('PID: ' .. lblink.PID)
print "\n\n"

b = lblink.open()
