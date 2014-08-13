#!/usr/bin/env lua 

-- figure out a better way to do this...
package.cpath = package.cpath .. ';' .. '../src/?.so'


local lblink = require 'lblink'

local numericvars = {'VID', 'PID' }
local stringvars = { '_VERSION' }
local functions = { 'enumerate', 'list', 'open' }


local function maketester(t)
   return function(library, name)
      assert(type(library[name]) == t, name .. ' not defined as a ' .. t .. '.')
   end
end


local testFunctionDefined = maketester('function')
local testStringsDefined = maketester('string')
local testNumericDefined = maketester('number')


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
