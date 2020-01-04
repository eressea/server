require "lunit"

module("tests.e2.trees", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4") -- food is free
    eressea.settings.set("NewbieImmunity", "0")
end

function test_no_growth()
    set_turn(204)
    assert_equal('spring', get_season())
    local r = region.create(0, 0, 'plain')
    r:set_resource('seed', 0)
    r:set_resource('sapling', 0)
    r:set_resource('tree', 0)
    process_orders()
    assert_equal(0, r:get_resource('seed'))
    assert_equal(0, r:get_resource('sapling'))
    assert_equal(0, r:get_resource('tree'))
end

function x_test_plant_seeds()
    set_turn(184)
    assert_equal('summer', get_season())
    local f = faction.create('goblin')
    local r = region.create(0, 0, 'plain')
    r:set_resource('seed', 0)
    r:set_resource('sapling', 0)
    r:set_resource('tree', 0)
    local u = unit.create(f, r)
    u:set_skill('herbalism', 20)
    u:add_item('seed', 40)
    u:add_order("PFLANZE 20 Samen")
    process_orders()
    assert_equal(0, r:get_resource('sapling'))
    assert_equal(20, r:get_resource('seed'))
    assert_equal(0, r:get_resource('tree'))
    assert_equal(20, u:get_item('seed'))
end

function test_plant_seeds_spring()
    set_turn(202)
    assert_equal('spring', get_season())
    local f = faction.create('goblin')
    local r = region.create(0, 0, 'plain')
    r:set_resource('seed', 0)
    r:set_resource('sapling', 0)
    r:set_resource('tree', 0)

    process_orders()
    assert_equal(0, r:get_resource('sapling'))
    assert_equal(0, r:get_resource('seed'))
    assert_equal(0, r:get_resource('tree'))

    local u = unit.create(f, r)
    u:set_skill('herbalism', 20)
    u:add_item('seed', 40)
    u:add_order("PFLANZE 20 Samen")
    process_orders()
    assert_equal(20, r:get_resource('sapling')) -- T12+ in Spring
    assert_equal(0, r:get_resource('seed'))
    assert_equal(0, r:get_resource('tree'))
    assert_equal(20, u:get_item('seed'))
end
