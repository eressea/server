require "lunit"

module("tests.e2.buildings", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4")
end

function teardown()
    eressea.settings.set("rules.food.flags", "0")
end

function test_castle_names()
    local r = region.create(0, 0, "plain")
    local b = building.create(r, "castle")

    assert_equal("site", b:get_typename(1))
    assert_equal("tradepost", b:get_typename(2))
    assert_equal("tradepost", b:get_typename(9))
    assert_equal("fortification", b:get_typename(10))
    assert_equal("fortification", b:get_typename(49))
    assert_equal("tower", b:get_typename(50))
    assert_equal("tower", b:get_typename(249))
    assert_equal("castle", b:get_typename(250))
    assert_equal("castle", b:get_typename(1249))
    assert_equal("fortress", b:get_typename(1250))
    assert_equal("fortress", b:get_typename(6249))
    assert_equal("citadel", b:get_typename(6250))
end

function test_build_castle_stages()
    local r = region.create(0,0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1000)
    local b = building.create(r, "castle")

    u:add_item("stone", 1000)

    u:set_skill("building", 1)
    u:clear_orders()

    u:add_order("MACHE BURG " .. itoa36(b.id))
    process_orders()
    assert_equal(10, b.size)
    
    u:set_skill("building", 3)
    u:clear_orders()
    
    u:add_order("MACHE BURG " .. itoa36(b.id))
    process_orders()
    assert_equal(250, b.size)
end
