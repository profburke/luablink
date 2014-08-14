
local lblink = require 'lblink'

local function doit(d,n)
  for i = 1,n do
     d:red(); d:sleep(300); d:blue(); d:sleep(300);
  end
  d:off()
end


local d = lblink.open()

if d then
  doit(d, 4)
end

