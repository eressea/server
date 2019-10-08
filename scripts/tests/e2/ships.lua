require "lunit"

module("tests.e2.ships", package.seeall, lunit.testcase)

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.ship.damage.nocrewocean", "0")
    eressea.settings.set("rules.ship.damage.nocrew", "0")
    eressea.settings.set("rules.ship.drifting", "0")
    eressea.settings.set("rules.ship.storms", "0")
end

function test_ship_requires_skill()
    local r1 = region.create(0, 0, "ocean")
    assert_not_nil(r1)
    local r2 = region.create(1, 0, "ocean")
    assert_not_nil(r2)
    local f = faction.create("human", "fake@eressea.de", "de")
    local u1 = unit.create(f, r1, 1)
    u1.name = "fake"
    u1.ship = ship.create(r1, "longboat")
    u1:clear_orders()
    u1:add_order("NACH O")
    process_orders()
    assert_equal(r1, u1.ship.region)
    assert_equal(r1, u1.region)
end

function test_ship_happy_case()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("human")
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r1, 1)
    u1.ship = ship.create(r1, "longboat")
    u2.ship = u1.ship
    u1:clear_orders()
    u1:add_order("NACH O")
    u1:set_skill("sailing", 1) -- cptskill = 1
    u2:set_skill("sailing", 9) -- sumskill = 10
    process_orders()
    assert_equal(r2, u1.ship.region)
    assert_equal(r2, u1.region)
    assert_equal(r2, u2.region)
end

function test_speedy_ship_slow()
    local r1 = region.create(0, 0, 'ocean')
    local f = faction.create("human")
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r1, 2)
    for x = 1, 10 do
        region.create(x, 0, 'ocean')
    end
    u1.ship = ship.create(r1, "dragonship")
    u2.ship = u1.ship
    u1:set_skill("sailing", 2) -- cptskill = 2^1
    u2:set_skill("sailing", 24) -- sumskill = 50
    u1:add_order("NACH O O O O O O O O O O")
    process_orders()
    assert_equal(5, u1.region.x)
end

function test_speedy_ship_fast()
    local r1 = region.create(0, 0, 'ocean')
    local f = faction.create("human")
    local u1 = unit.create(f, r1, 1)
    for x = 1, 10 do
        region.create(x, 0, 'ocean')
    end
    u1.ship = ship.create(r1, "dragonship")
    u1:set_skill("sailing", 54) -- cptskill = 2*3^3
    u1:add_order("NACH O O O O O O O O O O")
    process_orders()
    assert_equal(8, u1.region.x)
end

function test_ship_convoy_capacity()
    local r1 = region.create(1, 0, 'ocean')
    local r2 = region.create(2, 0, 'ocean')
    local f = faction.create("human")
    local u = unit.create(f, r1, 1)

    u:add_order('NACH O')
    u:set_skill('sailing', 2, true)
    u:add_item('jewel', 40)
    u.ship = ship.create(r1, 'boat')
    assert_equal(1, u.ship.number)
    process_orders()
    u:clear_orders()
    assert_equal(r2, u.region)

    u:add_order('NACH W')
    u:add_item('jewel', 1)
    u:set_skill('sailing', 2, true)
    process_orders()
    u:clear_orders()
    assert_equal(r2, u.region) -- too heavy

    u:add_order('NACH W')
    u:add_item('jewel', 39)
    u.ship.number = 2
    u.number = 2
    u:set_skill('sailing', 2, true)
    process_orders()
    u:clear_orders()
    assert_equal(r1, u.region) -- double capacity

    u:add_order('NACH O')
    u.ship.number = 2
    u.name = 'Bolgrim'
    u:add_item('jewel', 1) -- too heavy again
    u:set_skill('sailing', 2, true)
    process_orders()
    u:clear_orders()
    assert_equal(r1, u.region)
end

function test_ship_convoy_crew()
    local r1 = region.create(1, 0, 'ocean')
    local r2 = region.create(2, 0, 'ocean')
    local f = faction.create("human")
    local u = unit.create(f, r1, 1)
    u.ship = ship.create(r1, 'boat')
    u.ship.number = 2

    u:set_skill('sailing', 4, true)
    u:add_order('NACH O')
    process_orders()
    u:clear_orders()
    assert_equal(r1, u.region) -- not enough captains

    u.number = 2
    u:set_skill('sailing', 2, true)
    u:add_order('NACH O')
    process_orders()
    u:clear_orders()
    assert_equal(r2, u.region)
end

function test_ship_convoy_skill()
    local r1 = region.create(1, 0, 'ocean')
    local r2 = region.create(2, 0, 'ocean')
    local r3 = region.create(3, 0, 'ocean')
    local f = faction.create("human")
    local u = unit.create(f, r1, 1)

    u:set_skill('sailing', 2, true)
    u.ship = ship.create(r1, 'boat')
    assert_equal(1, u.ship.number)
    u:add_order('NACH O')
    process_orders()
    assert_equal(r2, u.region)

    u.ship.number = 2
    u:set_skill('sailing', 4, true)
    process_orders()
    assert_equal(r2, u.region) -- not enough captains

    u.number = 2
    u:set_skill('sailing', 2, true)
    process_orders()
    assert_equal(r3, u.region)
end

function test_give_ship()
    local r = region.create(1, 0, 'ocean')
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u1.ship = ship.create(r, 'boat')
    u1.ship.number = 2
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 SCHIFF")
    process_orders()
    assert_equal(1, u1.ship.number)
    assert_equal(1, u2.ship.number)
end

function test_give_ship_only_to_captain()
    local r = region.create(1, 0, 'ocean')
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    local u3 = unit.create(f, r, 1)
    u1.ship = ship.create(r, 'boat')
    u1.ship.number = 2
    u2.ship = ship.create(r, 'boat')
    u3.ship = u2.ship
    u1:add_order("GIB " .. itoa36(u3.id) .. " 1 SCHIFF")
    process_orders()
    assert_equal(2, u1.ship.number)
    assert_equal(1, u2.ship.number)
end

function test_give_ship_only_from_captain()
    local r = region.create(1, 0, 'ocean')
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    local u3 = unit.create(f, r, 1)
    u2.ship = ship.create(r, 'boat')
    u1.ship = ship.create(r, 'boat')
    u1.ship.number = 2
    u3.ship = u1.ship
    u3:add_order("GIB " .. itoa36(u2.id) .. " 1 SCHIFF")
    process_orders()
    assert_equal(2, u1.ship.number)
    assert_equal(1, u2.ship.number)
end

function test_give_ship_merge()
    local r = region.create(1, 0, 'ocean')
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u2.ship = ship.create(r, 'boat')
    u1.ship = ship.create(r, 'boat')
    u1.ship.number = 2
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 SCHIFF")
    process_orders()
    assert_equal(1, u1.ship.number)
    assert_equal(2, u2.ship.number)
end

function test_give_ship_only_same()
    local r = region.create(1, 0, 'ocean')
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u2.ship = ship.create(r, 'boat')
    u1.ship = ship.create(r, 'longboat')
    u1.ship.number = 2
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 SCHIFF")
    process_orders()
    assert_equal(2, u1.ship.number)
    assert_equal(1, u2.ship.number)
end

function test_give_ship_scale()
    local r = region.create(1, 0, 'plain')
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    local sh = ship.create(r, 'boat')
    sh.number = 3
    sh.damage = 9
    sh.size = 12
    u1.ship = sh
    u1:add_order("GIB " .. itoa36(u2.id) .. " 2 SCHIFF")
    process_orders()
    assert_equal(1, u1.ship.number)
    assert_equal(3, u1.ship.damage)
    assert_equal(4, u1.ship.size)
    assert_equal(2, u2.ship.number)
    assert_equal(6, u2.ship.damage)
    assert_equal(8, u2.ship.size)
end

function test_give_ship_all_ships()
    local r = region.create(1, 0, 'plain')
    local f = faction.create("human", 'noreply@vg.no')
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u1.ship = ship.create(r, 'boat')
    u1.ship.damage = 2
    u1.ship.number = 2
    u2.ship = ship.create(r, 'boat')
    u2.ship.number = 1
    u1:add_order("GIB " .. itoa36(u2.id) .. " 2 SCHIFF")
    process_orders()
    write_reports()
    assert_equal(3, u2.ship.number)
    assert_equal(u2.ship, u1.ship)
end

function test_give_ship_self_only()
    local r = region.create(1, 0, 'plain')
    local f1 = faction.create("human")
    local f2 = faction.create("human")
    local u1 = unit.create(f1, r, 1)
    local u2 = unit.create(f2, r, 1)
    local sh = ship.create(r, 'boat')
    sh.number = 2
    u1.ship = sh
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 SCHIFF")
    process_orders()
    assert_equal(2, u1.ship.number)
    assert_equal(nil, u2.ship)
end

function test_give_ship_not_cursed()
    local r = region.create(1, 0, 'plain')
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    local sh = ship.create(r, 'boat')
    u.ship = sh
    u:add_item("speedsail", 1)
    u:add_order("BENUTZE 1 Sonnensegel")
    process_orders()
    u:clear_orders()
    assert_equal(1, sh:get_curse('shipspeedup'))

    u:add_order("GIB " .. itoa36(u2.id) .. " 1 SCHIFF")
    process_orders()
    assert_equal(nil, u2.ship)
end

function test_speedsail_on_ship()
    local r = region.create(1, 0, 'plain')
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    local sh = ship.create(r, 'boat')
    u.ship = sh
    u:add_item("speedsail", 1)
    u:add_order("BENUTZE 1 Sonnensegel")
    process_orders()
    assert_equal(1, sh:get_curse('shipspeedup'))
end

function test_no_speedsail_on_convoy()
    local r = region.create(1, 0, 'plain')
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    local sh = ship.create(r, 'boat')
    u.ship = sh
    sh.number = 2
    u:add_item("speedsail", 2)
    u:add_order("BENUTZE 2 Sonnensegel")
    process_orders()
    assert_equal(nil, sh:get_curse('shipspeedup'))
end
