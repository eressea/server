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

-- hebalism < T6 cannot plant
function test_plant_fail()
    set_turn(184)
    assert_equal('summer', get_season())
    local f = faction.create('goblin')
    local r = region.create(0, 0, 'plain')
    r:set_resource('seed', 0)
    r:set_resource('sapling', 0)
    r:set_resource('tree', 0)
    local u = unit.create(f, r)
    u:set_skill('herbalism', 5)
    u:add_item('seed', 40)
    u:add_order("PFLANZE 20 Samen")
    process_orders()
    assert_equal(0, r:get_resource('seed'))
    assert_equal(0, r:get_resource('sapling'))
    assert_equal(0, r:get_resource('tree'))
    assert_equal(40, u:get_item('seed'))
end

-- T6+ herbalism allows planting seeds at 1:1 rates
function test_plant_summer()
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
    assert_equal(20, r:get_resource('seed'))
    assert_equal(0, r:get_resource('sapling'))
    assert_equal(0, r:get_resource('tree'))
    assert_equal(20, u:get_item('seed'))
end

-- in spring, herbalism >= T12 plants saplings at 10:1 rate
function test_plant_spring_saplings()
    set_turn(204)
    assert_equal('spring', get_season())
    local f = faction.create('goblin')
    local r = region.create(0, 0, 'plain')
    r:set_resource('seed', 0)
    r:set_resource('sapling', 0)
    r:set_resource('tree', 0)
    local u = unit.create(f, r)
    u:set_skill('herbalism', 12)
    u:add_item('seed', 20)
    u:add_order("PFLANZE 20 Samen") -- limited by herbalism
    process_orders()
    assert_equal(1, r:get_resource('sapling'))
    assert_equal(2, r:get_resource('seed'))
    assert_equal(0, r:get_resource('tree'))
    assert_equal(8, u:get_item('seed'))
end

-- herbalism < T12 means we are still planting seeds at 1:1
function test_plant_spring_seeds()
    set_turn(204)
    assert_equal('spring', get_season())
    local f = faction.create('goblin')
    local r = region.create(0, 0, 'plain')
    r:set_resource('seed', 0)
    r:set_resource('sapling', 0)
    r:set_resource('tree', 0)
    local u = unit.create(f, r)
    u:set_skill('herbalism', 10)
    u:add_item('seed', 40)
    u:add_order("PFLANZE 10 Samen")
    process_orders()
    assert_equal(0, r:get_resource('sapling'))
    assert_equal(10, r:get_resource('seed'))
    assert_equal(0, r:get_resource('tree'))
    assert_equal(30, u:get_item('seed'))
end
