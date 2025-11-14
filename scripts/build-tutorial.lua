require 'config'
local tut = require 'eressea.tutorial'

local r = 4
for x = 0, 3 do
    for y = 0, 3 do
        local dx = x * (2 * r + 2)
        local dy = -(2 * r + 2) * y
        tut.make_island(dx, dy, r)
    end
end
gmtool.editor()
