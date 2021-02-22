-- Basic test without loading XML Config. Test care about needed settings.
-- Tests are under scripts/test/ and all files must be in scripts/test/init.lua

lunit = require 'lunit'
if _VERSION >= 'Lua 5.2' then
module = lunit.module
end

require 'eressea.path'
require 'eressea'
require 'tests'
result = lunit.main()
return result.errors + result.failed
