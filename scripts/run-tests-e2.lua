-- Tests that work in all games. With game config of E2. 
-- Tests are under scripts/test/e2 and all files must be in scripts/test/e2/init.lua

path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
    package.path = package.path .. ';' .. config.install .. '/lunit/?.lua'
    --needed to find lunit if not run from eressea root. Needs right [lua] install setting in eressea.ini (point to eressea root from the start folder)
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

config.rules = 'e2'

require 'eressea'
require 'eressea.xmlconf'
require 'eressea.path'
require 'tests.e2'
require 'lunit'

rules = require('eressea.' .. config.rules)
result = lunit.main()
return result.errors
