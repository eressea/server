-- Tests that work in all games. With game config of E2. 
-- Tests are under scripts/test/e2 and all files must be in scripts/test/e2/init.lua

lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
module = lunit.module
end

path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

config.rules = 'e2'

require 'eressea'
require 'eressea.xmlconf'
require 'eressea.path'
require 'tests.e2'

rng.inject(0)
eressea.settings.set("rules.food.flags", "4")
eressea.settings.set("rules.ship.damage.nocrew", "0")
eressea.settings.set("rules.ship.drifting", "0")
eressea.settings.set("rules.ship.storms", "0")
eressea.settings.set("nmr.timeout", "0")
eressea.settings.set("NewbieImmunity", "0")
rules = require('eressea.' .. config.rules)
result = lunit.main()
return result.errors + result.failed
