local tcname = 'tests.e2.battle'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("rules.tactics.formula", "1")
end

function test_complex_battle()
    local r = region.create(0, 0, 'plain')
    r.name = 'Hyboria'
    local f1 = faction.create('goblin')
    local f2 = faction.create('human')
    local f3 = faction.create('cat')
    local f4 = faction.create('orc')
    local f5 = faction.create('aquarian')
    f1.name = 'ii'
    f2.name = 'w7fo'
    f3.name = 'fkmk'
    f4.name = 'kLin'
    f5.name = '1wpy'
    f1.age = 10
    f2.age = 10
    f3.age = 10
    f4.age = 10
    f5.age = 10
    f2:set_ally(f3, 'fight', true)
    f2:set_ally(f4, 'fight', true)
    f3:set_ally(f2, 'fight', true)
    f3:set_ally(f4, 'fight', true)
    f4:set_ally(f3, 'fight', true)
    f4:set_ally(f5, 'fight', true)
    local u1 = unit.create(f1, r)
    local u2 = unit.create(f2, r)
    local u3 = unit.create(f3, r)
    local u4 = unit.create(f4, r)
    local u5 = unit.create(f5, r)
    u3:set_skill('tactics', 10)
    u1.status = 1
    u2.status = 1
    u3.status = 1
    u4.status = 1
    u5.status = 1
    u2:add_order('ATTACKIERE ' .. itoa36(u1.id))
    u3:add_order('ATTACKIERE ' .. itoa36(u1.id))
    u2:add_order('ATTACKIERE ' .. itoa36(u5.id))
    process_orders()
end
