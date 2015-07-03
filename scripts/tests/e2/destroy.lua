require "lunit"

module("tests.e2.destroy", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("NewbieImmunity", "0")
end

function test_dont_move_after_destroy()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u = unit.create(f, r1, 1)
    u.building = building.create(u.region, "castle")
    u:clear_orders()
    u:add_order("NACH O")
    u:add_order("ZERSTOERE " .. itoa36(u.building.id))
    process_orders()
    assert_equal(r1, u.region)
    assert_equal(nil, u.building)
end

function test_dont_destroy_after_attack()
    local r1 = region.create(0, 0, "plain")
    local u = unit.create(faction.create("one@example.com", "human", "de"), r1, 10)
    local u2 = unit.create(faction.create("two@example.com", "human", "de"), r1, 1)
    u.building = building.create(u.region, "castle")
    u:clear_orders()
    u:add_order("ATTACKIERE " .. itoa36(u2.id))
    u:add_order("ZERSTOERE " .. itoa36(u.building.id))
    process_orders()
    assert_not_nil(u.building)
end

function test_destroy_is_long()
    local r1 = region.create(0, 0, "plain")
    local u = unit.create(faction.create("one@example.com", "human", "de"), r1, 10)
    u.building = building.create(u.region, "castle")
    u:clear_orders()
    u:add_order("LERNE Unterhaltung")
    u:add_order("ZERSTOERE " .. itoa36(u.building.id))
    process_orders()
    assert_equal(0, u:get_skill("entertainment"))
    assert_equal(nil, u.building)
end
