function list_all_spells()
    local r = region.create(0, 0, "plain")
    local schools = {
    "gray",
    "illaun",
    "tybied",
    "cerddor",
    "gwyrrd",
    "draig",
    "common" }

    for i, school in ipairs(schools) do
      local f = faction.create("drac@eressea.de", "human", "de")
      f.name = school
      f.magic = school
      f.id = atoi36(school:sub(1,4))
      local u = unit.create(f, r, 1)
      u.race="elf"
      u:set_skill("magic", 50)
      u.magic=school
      u:add_all_spells(school)
    end

    process_orders()
    write_reports()
end


local path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

config.rules='e3'

require 'eressea'
require 'eressea.path'
require 'eressea.xmlconf'

local rules = {}
if config.rules then
    rules = require('eressea.' .. config.rules)
    eressea.log.info('loaded ' .. #rules .. ' modules for ' .. config.rules)
else
    eressea.log.warning('no rule modules loaded, specify a game in eressea.ini or with -r')
end

list_all_spells()
