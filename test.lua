#!/usr/bin/env lua -i

local luablink = require 'luablink'

print('LuaBlink version: ' .. luablink._VERSION)
print "\n"
print('VID: ' .. luablink.VID)
print('PID: ' .. luablink.PID)
print "\n\n"

b = luablink.open()
