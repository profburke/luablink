#!/usr/bin/env lua 


-- In case we want to run tests when library is built but not installed.
local LUA_PATH_SEP = package.config:sub(3, 3)
package.cpath = package.cpath .. LUA_PATH_SEP .. '../src/?.so'



local function maketester(t)
   return function(library, name)
      assert(type(library[name]) == t, string.format('%s not defined as a %s.', name, t)
   end
end


local testFunctionDefined = maketester('function')
local testStringsDefined = maketester('string')
local testNumericDefined = maketester('number')






local lblink = require 'lblink'

local numericvars = {'VID', 'PID' }
local stringvars = { '_VERSION' }
local functions = { 'enumerate', 'list', 'open' }


for _,n in ipairs(numericvars) do
   testNumericDefined(lblink, n)
end


for _,s in ipairs(stringvars) do
   testStringsDefined(lblink, s)
end


for _,f in ipairs(functions) do
   testFunctionDefined(lblink, f)
end

print "Success"
