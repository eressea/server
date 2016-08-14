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

eressea.read_game(get_turn() .. '.dat')
ids = {2081501646, 1967748303, 1137, 2000, 1456894557, 1580742069, 1143084084, 285224813, 604912520, 296884068, 50}
p=plane.create(50, -7280, -4494, 83, 83, "Regatta")

for k,v in ipairs(ids) do
    p = plane.get(v)
    print(v, p)
    p:erase()
end
eressea.write_game(get_turn() .. '.new')
eressea.free_game()
eressea.read_game(get_turn() .. '.new')
write_reports()
eressea.write_game(get_turn() .. '.fix')
