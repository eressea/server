-- Tests that work in E3. With game config of E3. 
-- Tests are under scripts/test/e3 and all files must be in scripts/test/e3/init.lua

path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
    package.path = package.path .. ';' .. config.install .. '/lunit/?.lua'
    --needed to find lunit if not run form eressea root. Needs right [lua] install setting in eressea.ini (point to eressea root from the start folder)
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

config.rules = 'e3'

require 'eressea'
require 'eressea.path'
require 'eressea.xmlconf'
require 'tests.e3'
require 'lunit'

rng_default(0)
eressea.settings.set("rules.alliances", "0")
rules = require('eressea.' .. config.rules)
result = lunit.main()
return result.errors + result.failed
