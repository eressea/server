-- Basic test without loading XML Config. Test care about needed settings.
-- Tests are under scripts/test/ and all files must be in scripts/test/init.lua

path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
    package.path = package.path .. ';' .. config.install .. '/lunit/?.lua' 
    --needed to find lunit if not run form eressea root. Needs right [lua] install setting in eressea.ini (point to eressea root from the start folder)
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

require 'eressea'
require 'eressea.path'
require 'tests'
require 'lunit'
result = lunit.main()
return result.errors + result.failed
