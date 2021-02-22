-- Tests that work in E3. With game config of E3. 
-- Tests are under scripts/test/e3 and all files must be in scripts/test/e3/init.lua

config.rules = 'e3'
lunit = require 'lunit'
if _VERSION >= 'Lua 5.2' then
    module = lunit.module
end

require 'eressea.path'
require 'eressea'
require 'eressea.xmlconf'
require 'tests.e3'

rng.inject(0)
eressea.settings.set("rules.alliances", "0")
eressea.settings.set("rules.food.flags", "4")
eressea.settings.set("rules.ship.damage.nocrew", "0")
eressea.settings.set("rules.ship.drifting", "0")
eressea.settings.set("rules.ship.storms", "0")
eressea.settings.set("nmr.timeout", "0")
eressea.settings.set("NewbieImmunity", "0")
rules = require('eressea.' .. config.rules)
result = lunit.main()
return result.errors + result.failed
