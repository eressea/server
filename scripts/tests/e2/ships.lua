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
