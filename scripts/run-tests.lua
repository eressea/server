-- Basic test without loading XML Config. Test care about needed settings.
-- Tests are under scripts/test/ and all files must be in scripts/test/init.lua

path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

lunit = require 'lunit'
require 'eressea'
require 'eressea.path'
require 'tests'
result = lunit.main()
return result.errors + result.failed
