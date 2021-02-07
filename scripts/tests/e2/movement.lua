local tcname = 'tests.e2.movement'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.ship.damage.nocrewocean", "0")
    eressea.settings.set("rules.ship.damage.nocrew", "0")
    eressea.settings.set("rules.ship.drifting", "0")
    eressea.settings.set("rules.ship.storms", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
end

function test_piracy()
    local r = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "ocean")
    local r3 = region.create(-1, 0, "ocean")
    local f = faction.create("human", "pirate@eressea.de", "de")
    local f2 = faction.create("human", "elf@eressea.de", "de")
    local u1 = unit.create(f, r2, 1)
    local u2 = unit.create(f2, r3, 1)
    
    u1.ship = ship.create(r2, "longboat")
    u2.ship = ship.create(r3, "longboat")
    u1:set_skill("sailing", 10)
    u2:set_skill("sailing", 10)

    u1:clear_orders()
    u1:add_order("PIRATERIE")
    u2:clear_orders()
    u2:add_order("NACH o")

    process_orders()

    assert_equal(r, u2.region) -- Nach Osten
    assert_equal(r, u1.region) -- Entern!
end 

function test_piracy_to_land()
    local r = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local r3 = region.create(-1, 0, "ocean")
    local f = faction.create("human", "pirate@eressea.de", "de")
    local f2 = faction.create("human", "elf@eressea.de", "de")
    local u1 = unit.create(f, r2, 1)
    local u2 = unit.create(f2, r3, 1)
    
    u1.ship = ship.create(r2, "longboat")
    u2.ship = ship.create(r3, "longboat")
    u1:set_skill("sailing", 10)
    u2:set_skill("sailing", 10)

    u1:clear_orders()
    u1:add_order("PIRATERIE")
    u2:clear_orders()
    u2:add_order("NACH o")

    process_orders()

    assert_equal(r, u2.region) -- Nach Osten
    assert_equal(r2, u1.region) -- bewegt sich nicht
end 

function test_dolphin_on_land()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u1 = unit.create(f, r1, 1)
    u1.race = "dolphin"
    u1:clear_orders()
    u1:add_order("NACH O")
    process_orders()
    assert_equal(r1, u1.region)
end

function test_dolphin_to_land()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u1 = unit.create(f, r1, 1)
    u1.race = "dolphin"
    u1:clear_orders()
    u1:add_order("NACH O")
    process_orders()
    assert_equal(r2, u1.region)
end

function test_dolphin_in_ocean()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u1 = unit.create(f, r1, 1)
    u1.race = "dolphin"
    u1:clear_orders()
    u1:add_order("NACH O")
    process_orders()
    assert_equal(r2, u1.region)
end

function test_follow()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r1, 1)
    u1:clear_orders()
    u2:clear_orders()
    u1:add_item("money", 100)
    u2:add_item("money", 100)
    u1:add_order("NACH O")
    u2:add_order("FOLGE EINHEIT " .. itoa36(u1.id))
    process_orders()
    assert_equal(u1.region, r2)
    assert_equal(u2.region, r2)
end

function test_follow_ship()
    local r1 = region.create(0, 0, "plain")
    region.create(1, 0, "ocean")
    region.create(2, 0, "ocean")
    local f = faction.create("human", "test@example.com", "de")
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r1, 1)
    u1:add_item("money", 100)
    u2:add_item("money", 100)
    u1.ship = ship.create(r1, "boat")
    assert(u1.ship)
    u1:set_skill("sailing", 2)
    u1:clear_orders()
    u1:add_order("NACH O O")
    u2.ship = ship.create(r1, "boat")
    u2:set_skill("sailing", 2)
    u2:clear_orders()
    u2:add_order("FOLGE SCHIFF " .. itoa36(u1.ship.id))
    process_orders()
    assert_equal(2, u1.region.x)
    assert_equal(2, u2.region.x)
end

function assert_nomove(text, u)
    if text == nil then text = "" else text = text .. "; " end
    local r = u.region
    u:add_order("NACH O O")
    process_orders()
    assert_equal(r, u.region, text .. "unit should never move")
end

function assert_capacity(text, u, silver, r1, r2, rx)
    if text == nil then text = "" else text = text .. "; " end
    if rx == nil then rx = r1 end
    u.region = r1
    u:add_item("money", silver-u:get_item("money"))
    u:add_order("NACH O O")
    process_orders()
    assert_equal(r2, u.region, text .. "unit should move")

    u.region = r1
    u:add_item("money", 1)
    process_orders()
    assert_equal(rx, u.region, text .. "unit should not move")
end

function test_move_to_same_region_leaves_building()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    local b = building.create(u.region, "castle")
    b.size = 2
    u.building = b
    assert_not_nil(u.building)
    u:add_item("horse", 1)
    u:set_skill("riding", 2)
    u:add_order("NACH O W")
    process_orders()
    assert_equal(r1, u.region)
    assert_nil(u.building)
end

function test_aquarians_can_swim()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "ocean")
    local r3 = region.create(2, 0, "ocean")
    local f = faction.create("aquarian")
    local u1 = unit.create(f, r2, 1)
    local u2 = unit.create(f, r2, 1)
    local sh = ship.create(r2, 'boat')
    u1.ship = sh
    u1:set_skill('sailing', 2)
    u1:add_order("NACH O")
    u2.ship = sh
    u2:add_order("NACH W")
    process_orders()
    assert_equal(r1, u2.region)
    assert_equal(r3, u1.region)
end

function test_only_aquarians_can_swim()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "ocean")
    local r3 = region.create(2, 0, "ocean")
    local f = faction.create('human')
    local u1 = unit.create(f, r2, 1)
    local u2 = unit.create(f, r2, 1)
    local sh = ship.create(r2, 'boat')
    u1.ship = sh
    u1:set_skill('sailing', 2)
    u1:add_order("NACH O")
    u2.ship = sh
    u2:add_order("NACH W")
    process_orders()
    assert_equal(r3, u2.region)
    assert_equal(r3, u1.region)
end

function test_looping_ship()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("aquarian")
    local u1 = unit.create(f, r1, 1)
    u1.ship = ship.create(r1, 'boat')
    local u2 = unit.create(f, r1, 1)
    u2.ship = ship.create(r1, 'boat')
    u1:set_skill('sailing', 2)
    u1:add_order("NACH O W")
    u2:set_skill('sailing', 2)
    u2:add_order("NACH O")
    process_orders()
    assert_equal(r1, u1.region)
    assert_equal(r2, u2.region)
end
