require "lunit"

module("tests.production", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4") -- food is free
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("study.random_progress", "0")
end

local function create_faction(race)
    return faction.create(race, race .. '@eressea.de', "de")
end

function test_laen_needs_mine()
    -- some resources require a building
    -- i.e. you cannot create laen without a mine
    local r = region.create(0, 0, "mountain")
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    r:set_resource('laen', 100)
    u:add_order("MACHE Laen")
    u:set_skill('mining', 7)
    turn_process()
    assert_equal(0, u:get_item('laen'))
    assert_equal(100, r:get_resource('laen'))
    assert_equal(1, f:count_msg_type("building_needed")) -- requires building

    u.building = building.create(u.region, "mine")
    u.building.working = true
    u.building.size = 10
    turn_process()
    assert_equal(1, u:get_item('laen'))
    assert_equal(99, r:get_resource('laen'))

    turn_end()
end

function test_mine_laen_bonus()
    -- some buildings grant a bonus on the production skill
    -- i.e. a mine adds +1 to mining
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    r:set_resource('laen', 100)
    assert_equal(100, r:get_resource('laen'))
    u:add_order("MACHE Laen")
    u:set_skill('mining', 6)
    u.building = building.create(u.region, "mine")
    u.building.working = true
    u.building.size = 10
    u.number = 2
    turn_process() -- T6 is not enough for laen
    assert_equal(0, u:get_item('laen'))
    assert_equal(100, r:get_resource('laen'))
    assert_equal(1, f:count_msg_type("manufacture_skills"))

    u:set_skill('mining', 13)
    turn_process() -- T13 is enough, the +1 produces one extra Laen
    assert_equal(4, u:get_item('laen')) -- FAIL (3)
    assert_equal(96, r:get_resource('laen'))

    turn_end()
end

function test_mine_iron_bonus()
    -- some buildings grant a bonus on the production skill
    -- i.e. a mine adds +1 to mining iron
    --
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    r:set_resource('iron', 100)
    assert_equal(100, r:get_resource('iron'))
    u:add_order("MACHE Eisen")
    u:set_skill('mining', 1)
    u.building = building.create(u.region, "mine")
    u.building.working = false
    u.building.size = 10
    u.number = 2
    turn_process() -- iron can be made without a working mine
    assert_equal(2, u:get_item('iron'))
    assert_equal(98, r:get_resource('iron'))

    u.building.working = true
    turn_process()
    assert_equal(6, u:get_item('iron'))
    assert_equal(96, r:get_resource('iron'))

    turn_end()
end

function test_quarry_bonus()
    -- a quarry grants +1 to quarrying, and saves 50% stone
    --
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    r:set_resource('stone', 100)
    assert_equal(100, r:get_resource('stone'))
    u:add_order("MACHE Stein")
    u:set_skill('quarrying', 1)
    u.number = 2
    u.building = building.create(u.region, 'quarry')
    u.building.working = false
    u.building.size = 10
    turn_process()
    assert_equal(2, u:get_item('stone'))
    assert_equal(98, r:get_resource('stone'))

    u.building.working = true
    turn_process()
    assert_equal(6, u:get_item('stone'))
    assert_equal(96, r:get_resource('stone'))

    turn_end()
end

function test_smithy_no_bonus()
-- a smithy does not give a bonus to other skills
-- five cartmakers make 5 carts, no matter what
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    u.building = building.create(u.region, 'smithy')
    u.building.working = false
    u.building.size = 10
    u.number = 5
    u:set_skill('cartmaking', 1) -- needs 1 min
    u:add_item('log', 100)
    u:add_order("MACHE Wagen")
    turn_process() -- building disabled
    assert_equal(5, u:get_item('cart'))
    assert_equal(75, u:get_item('log'))

    u.building.working = true
    turn_process() -- building active
    assert_equal(10, u:get_item('cart'))
    assert_equal(50, u:get_item('log'))

    turn_end()
end

function test_smithy_bonus_iron()
-- a smithy adds +1 to weaponsmithing, and saves 50% iron
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    u.building = building.create(u.region, 'smithy')
    u.building.working = false
    u.building.size = 10
    u:set_skill('weaponsmithing', 5) -- needs 3
    u:add_item('iron', 100)
    u:add_order("MACHE Schwert")
    turn_process() -- building disabled
    assert_equal(1, u:get_item('sword'))
    assert_equal(99, u:get_item('iron'))

    u.building.working = true
    turn_process() -- building active
    assert_equal(3, u:get_item('sword'))
    assert_equal(98, u:get_item('iron'))

    turn_end()
end

function test_smithy_bonus_mixed()
-- a smithy adds +1 to weaponsmithing, and saves 50% iron
-- it does not save any other resource, though.
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    u.building = building.create(u.region, 'smithy')
    u.building.working = false
    u.building.size = 10
    u:set_skill('weaponsmithing', 5) -- needs 3
    u:add_item('iron', 100)
    u:add_item('log', 100)
    u:add_order("MACHE Kriegsaxt")
    turn_process() -- building disabled
    assert_equal(1, u:get_item('axe'))
    assert_equal(99, u:get_item('iron'))
    assert_equal(99, u:get_item('log'))

    u.building.working = true
    turn_process() -- building active
    assert_equal(3, u:get_item('axe'))
    assert_equal(98, u:get_item('iron'))
    assert_equal(97, u:get_item('log'))

    turn_end()
end
