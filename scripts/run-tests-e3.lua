-- new tests 2014-06-11

path = 'scripts'
if config.source_dir ~= nil then
	path = config.source_dir .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

config.rules = 'e3'

require 'eressea'
require 'eressea.path'
require 'eressea.xmlconf'
require 'tests.e3'
require 'lunit'

eressea.settings.set("rules.alliances", "0")
rules = require('eressea.' .. config.rules)
lunit.main()
