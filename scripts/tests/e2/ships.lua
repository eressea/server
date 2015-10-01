require "lunit"

module("tests.e2.ships", package.seeall, lunit.testcase)

function setup()
    eressea.settings.set("rules.ship.damage.nocrewocean", "0")
    eressea.settings.set("rules.ship.damage.nocrew", "0")
    eressea.settings.set("rules.ship.drifting", "0")
end

function test_ship_requires_skill()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("fake@eressea.de", "human", "de")
    local u1 = unit.create(f, r1, 1)
	u1.name = "fake"
    u1.ship = ship.create(r1, "longboat")
    u1:clear_orders()
    u1:add_order("NACH O")
    process_orders()
    assert_equal(r1, u1.ship.region)
    assert_equal(r1, u1.region)
end

function no_test_ship_happy_case()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("hodor@eressea.de", "human", "de")
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

