local tcname = 'tests.e2.production'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4") -- food is free
    eressea.settings.set("NewbieImmunity", "0")
end

local function create_faction(race)
    return faction.create(race, race .. '@example.com', "de")
end

function test_produce_multi()
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)
    -- sword needs skill=3, iron=1
    u:set_skill('weaponsmithing', 15)
    u:add_item('iron', 5)

    turn_begin()
    u:add_order("MACHE 6 Schwert")
    
    turn_process()
    assert_equal(5, u:get_item('sword'))
    assert_equal(0, u:get_item('iron'))
end

function test_greatbow_needs_elf()
-- only elves can build a greatbow
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    u:set_skill('weaponsmithing', 5)
    u:add_item('mallorn', 2)
    u:add_order("MACHE Elfenbogen")
    turn_process() -- humans cannot do it
    assert_equal(1, f:count_msg_type("error117"))
    assert_equal(0, u:get_item('greatbow'))
    assert_equal(2, u:get_item('mallorn'))

    u.race = 'elf'
    turn_process() -- but elves can
    assert_equal(1, u:get_item('greatbow'))
    assert_equal(0, u:get_item('mallorn'))
end

function test_troll_quarrying_bonus()
-- Von Trollen abgebaute Steine werden nur zu 75% vom "Regionsvorrat" abgezogen. 
-- Dieser Effekt ist kumulativ zu einem Steinbruch.
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    r:set_resource("stone", 100)
    u:set_skill('quarrying', 4)
    u:add_order("MACHE Steine")
    turn_process() -- humans get no bonus
    assert_equal(4, u:get_item('stone'))
    assert_equal(96, r:get_resource('stone'))

    u.race = 'troll'
    u:set_skill('quarrying', 2)
    turn_process() -- trolls have +2 to quarrying, and save 25%
    assert_equal(8, u:get_item('stone'))
    assert_equal(93, r:get_resource('stone'))
end

function test_dwarf_mining_bonus()
-- Von Zwergen abgebautes Eisen wird nur zu 60% vom "Regionsvorrat" abgezogen. 
-- Dieser Effekt ist kumulativ zu einem Bergwerk (siehe hier und hier).
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    r:set_resource('iron', 100)
    u:set_skill('mining', 10)
    u:add_order('MACHE Eisen')
    turn_process() -- humans get no bonus
    assert_equal(10, u:get_item('iron'))
    assert_equal(90, r:get_resource('iron'))

    u.race = 'dwarf'
    u:set_skill('mining', 8)
    turn_process() -- dwarves have +2 to mining, and save 40%
    assert_equal(20, u:get_item('iron'))
    assert_equal(84, r:get_resource('iron'))
end

function test_build_boat_low_skill()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "build@example.com")
    local u = unit.create(f, r, 1)
    u:set_skill("shipcraft", 3) -- humans get +1
    u:add_item("log", 10)
    u:add_order("MACHE BOOT")
    process_orders()
    assert_not_nil(u.ship)
    assert_equal(4, u.ship.size)
    assert_equal(6, u:get_item('log'))
end

function test_build_boat_high_skill()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:set_skill("shipcraft", 5) -- humans get +1
    u:add_item("log", 10)
    u:add_order("MACHE BOOT")
    process_orders()
    assert_not_equal(nil, u.ship)
    assert_equal(5, u.ship.size)
    assert_equal(5, u:get_item('log'))
end

function test_work()
    eressea.settings.set("rules.peasants.growth.factor", "0")
    local r = region.create(0, 0, "plain")
    r:set_resource('peasant', 1)
    r:set_resource('tree', 0)
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order('ARBEITE')
    r:set_resource('money', 0)

    process_orders()
    assert_equal(10, u:get_item('money'))
    assert_equal(1, r:get_resource('money'))
    b = building.create(r, 'castle')

    b.size = 2
    r:set_resource('money', 0)
    u:add_item('money', -u:get_item('money'))
    process_orders()
    assert_equal(10, u:get_item('money'))
    assert_equal(1, r:get_resource('money'))

    b.size = 10
    r:set_resource('money', 0)
    u:add_item('money', -u:get_item('money'))
    process_orders()
    assert_equal(11, u:get_item('money'))
    assert_equal(2, r:get_resource('money'))

    b.size = 50
    r:set_resource('money', 0)
    u:add_item('money', -u:get_item('money'))
    process_orders()
    assert_equal(12, u:get_item('money'))
    assert_equal(3, r:get_resource('money'))

    r:set_resource('money', 0)
    u:add_item('money', -u:get_item('money'))
    b.size = 250
    process_orders()
    assert_equal(13, u:get_item('money'))
    assert_equal(4, r:get_resource('money'))

    r:set_resource('money', 0)
    u:add_item('money', -u:get_item('money'))
    b.size = 1250
    process_orders()
    assert_equal(14, u:get_item('money'))
    assert_equal(5, r:get_resource('money'))

    r:set_resource('money', 0)
    u:add_item('money', -u:get_item('money'))
    b.size = 6250
    process_orders()
    assert_equal(15, u:get_item('money'))
    assert_equal(6, r:get_resource('money'))
end

function test_blessed_harvest()
    eressea.settings.set("rules.peasants.growth.factor", "0")
    local r = region.create(0, 0, "plain")
    r:set_resource('peasant', 1)
    r:set_resource('tree', 0)
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order('ARBEITE')
    r:set_resource('money', 0)
    r:add_curse('blessedharvest', nil, 2, 1, 1) -- duration, force, effect

    process_orders()
    assert_equal(10, u:get_item('money')) -- only peasants benefit
    assert_equal(2, r:get_resource('money')) -- peasants work +1

    b = building.create(r, 'castle')
    b.size = 6250
    r:set_resource('money', 0)
    u:add_item('money', -u:get_item('money'))
    assert_equal(1, r:get_curse('blessedharvest'))
    process_orders()
    assert_equal(15, u:get_item('money')) -- only peasants get +1
    assert_equal(7, r:get_resource('money')) -- peasants get +1
end
