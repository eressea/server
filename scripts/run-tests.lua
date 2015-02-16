-- new tests 2014-06-11

path = 'scripts'
if config.source_dir ~= nil then
	path = config.source_dir .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

require 'eressea'
require 'eressea.path'
require 'tests'
require 'lunit'
result = lunit.main()
return result.errors
