require "lunit"

module("tests.e2.undead", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
end

function test_undead_give_item()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("hodor@eressea.de", "human", "de")
    local u1 = unit.create(f1, r1, 1)
    u1.race = "undead"
    u1:clear_orders()
    u1:add_item("log", 1)
    u1:add_order("GIB 0 1 Holz")
    process_orders()
    assert_equal(0, u1:get_item("log"))
end

function test_undead_dont_give_person()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("hodor@eressea.de", "human", "de")
    local u1 = unit.create(f1, r1, 2)
    u1.race = "undead"
    u1:clear_orders()
    u1:add_item("log", 1)
    u1:add_order("GIB 0 1 Person")
    process_orders()
    assert_equal(2, u1.number)
end
