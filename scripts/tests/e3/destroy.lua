require "lunit"

module("tests.e2.destroy", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
end


function test_destroy()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 2)
    local mine = building.create(r, "mine")
    local castle = building.create(r, "castle")
    mine.size = 2
    castle.size = 100
    castle.damage = 10
    u1.building = mine
    u2.building = castle
    u2:set_skill("building", 4) -- for destroy skill +1, it is possible to destroy without skill
    u1:clear_orders()
    u2:clear_orders()
    u1:add_order("ZERSTOERE")
    u2:add_order("ZERSTOERE 20")
    process_orders()
    assert_equal(2, mine.size)
    assert_equal(20, castle.damage) -- 10 damge present + (4 skill + 1) * 2 Personen
    u1:set_skill("building", 2)
    u1:clear_orders()
    u2:clear_orders()
    u1:add_order("ZERSTOERE")
    u2:add_order("ZERSTOERE 5")
    process_orders()
    assert_equal(0, mine.size)
    assert_equal(25, castle.damage)
    assert_equal(10, u1:get_item("log"))
    assert_equal(5, u1:get_item("stone"))
end

function test_make_destroyed_castle()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    local castle = building.create(r, "castle")
    castle.size = 100
    castle.damage = 20
    u1:add_item("stone", 5)
    u1:set_skill("building", 20)
    u1.building = castle
    u1:clear_orders()
    u1:add_order("MACHE BURG")
    process_orders()
    assert_equal(0, castle.damage)
    assert_equal(3, u1:get_item("stone"))
end

function test_umlaute()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f, r, 2)
    local castle = building.create(r, "castle")
    castle.size = 100
    castle.damage = 10
    u1.building = castle
    u1:set_skill("building", 4) -- for destroy skill +1, it is possible to destroy without skill
    u1:clear_orders()
    u1:add_order("ZERSTÖRE 20")
    process_orders()
    assert_equal(20, castle.damage) -- 10 damge present + (4 skill + 1) * 2 Personen
end
