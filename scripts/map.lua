local path = 'scripts'
if config.install then
	path = config.install .. '/' .. path
end
package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path

require 'eressea.path'
require 'eressea'
require 'eressea.xmlconf'

eressea.read_game(get_turn() .. ".dat")
gmtool.editor()
