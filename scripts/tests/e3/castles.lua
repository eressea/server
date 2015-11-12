require "lunit"

module("tests.e3.castles", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4")
end

function teardown()
    eressea.settings.set("rules.food.flags", "0")
end

function test_small_castles()
	local r = region.create(0, 0, "plain")
	local f1 = faction.create("noreply@eressea.de", "human", "de")
	local u1 = unit.create(f1, r, 1)
	local f2 = faction.create("noreply@eressea.de", "halfling", "de")
	local u2 = unit.create(f2, r, 1)
	u1:add_item("money", 10000)

	local b = building.create(r, "castle")
	u2.building = b
	u1.building = b

	b.owner = u2
	assert_equal("site", b:get_typename(7))
	assert_equal("fortification", b:get_typename(8))
	b.owner = u1
	assert_equal("site", b:get_typename(9))
	assert_equal("fortification", b:get_typename(10))
end

function test_build_normal()
	local r = region.create(0, 0, "plain")
	local f = faction.create("noreply@eressea.de", "human", "de")
	local u = unit.create(f, r, 1)
    u:clear_orders()
    u:add_item("stone", 10)
    u:set_skill("building", 10)
    u:add_order("MACHE BURG")
    process_orders()
    assert_not_nil(u.building)
    assert_equal(10, u.building.size)
end

function test_build_packice()
	local r = region.create(0, 0, "packice")
	local f = faction.create("noreply@eressea.de", "human", "de")
	local u = unit.create(f, r, 1)
    u:clear_orders()
    u:add_item("stone", 10)
    u:set_skill("building", 10)
    u:add_order("MACHE BURG")
    process_orders()
    assert_equal(nil, u.building)
end

function test_build_castle_stages()
    local r = region.create(0,0, "plain")
    local f = faction.create("castle@eressea.de", "human", "de")
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
