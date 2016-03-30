require "lunit"

module("tests.e3.fleets", package.seeall, lunit.testcase)

local settings

function setup()
    eressea.game.reset()
    settings = {}
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.food.flags", "4")
end

function test_fleetbasics()
    local ocean = region.create(1, 0, "ocean")
    local r = region.create(0, 0, "plain")
    local r2 = region.create(2, 0, "glacier")
    local f = faction.create("admiral@eressea.de", "human", "de")
    local f2 = faction.create("fleet2@eressea.de", "human", "de")
    local f3 = faction.create("fleet3@eressea.de", "human", "de")
    local s1 = ship.create(r, "caravel")
    local s2 = ship.create(r, "cutter")
    local s3 = ship.create(r, "cutter")
    local s4 = ship.create(r, "cutter")
    local s5 = ship.create(r, "cutter")
    local s6 = ship.create(r, "cog")
    local u1 = unit.create(f, r, 4)
    local u2 = unit.create(f, r, 26)
    local u3 = unit.create(f2, r, 10)
    local u4 = unit.create(f2, r, 1)
    local u5 = unit.create(f3, r, 120)

    s1.name = "s1"
    s2.name = "s2"
    s3.name = "s3"
    s4.name = "s4"
    s5.name = "s5"
    s6.name = "s6"

    u1:add_item("money", 1000)
    u2:add_item("money", 1000)
    u3:add_item("money", 1000)
    u4:add_item("money", 1000)
    u5:add_item("money", 1000)
    u1.ship = s1
    u2.ship = s2
    u3.ship = s3
    u4.ship = s4
    u5.ship = s3
    u1:set_skill("sailing", 10)
    u2:set_skill("sailing", 5)
    u1:clear_orders()
    u2:clear_orders()
    u3:clear_orders()
    u4:clear_orders()
    u1:add_order("NACH O O")
    u3:add_order("Kontaktiere " .. itoa36(u1.id))
    u1:add_order("flotte " .. itoa36(s1.id))
    u1:add_order("flotte " .. itoa36(s2.id))
    u1:add_order("flotte " .. itoa36(s3.id))
    u1:add_order("flotte " .. itoa36(s4.id))
    u1:add_order("flotte " .. itoa36(s5.id))
--    u1:add_order("flotte " .. itoa36(s6.id))
    process_orders()
    assert_equal(ocean.id, u1.region.id) -- cannot land in glacier without harbour, should be on the ocean
    assert_equal(ocean.id, u2.region.id) -- u2 should be on the fleet
    assert_equal(ocean.id, u3.region.id) -- u3 too
    assert_equal(r.id, u4.region.id) -- u4 not, no contact
    assert_equal(ocean.id, u5.region.id) -- u5 is on the same ship like u3
    assert_equal(ocean.id, s1.region.id) -- the plain case: okay
    assert_equal(ocean.id, s2.region.id) -- s2 should be part of the fleet
    assert_equal(ocean.id, s3.region.id) -- s3 too
    assert_equal(r.id, s4.region.id) -- s4 not, no contact
    assert_equal(ocean.id, s5.region.id) -- s5 should be part of the fleet (no owner)
--    assert_equal(r.id, s6.region.id) -- s6 not (only 4 persons in the capt'n unit -> max 4 ships)
    
end

function test_mixed_ships_speed()
    local ocean1 = region.create(1, 0, "ocean")
    local ocean2 = region.create(2, 0, "ocean")
    local ocean3 = region.create(3, 0, "ocean")
    local ocean4 = region.create(4, 0, "ocean")
    local r = region.create(0, 0, "plain")
    local f = faction.create("mixedspeed@eressea.de", "human", "de")
    local s1 = ship.create(r, "caravel")  -- speed 5
    local s2 = ship.create(r, "cutter") -- speed 3
    local u1 = unit.create(f, r, 4)
    
    u1:add_item("money", 1000)
    u1.ship = s1
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH O O O O O O")
    u1:add_order("flotte " .. itoa36(s1.id))
    u1:add_order("flotte " .. itoa36(s2.id))
    process_orders()
    assert_equal(ocean3.id, u1.region.id) -- fleet speed should be 3
    assert_equal(ocean3.id, s1.region.id) -- s1 should be part of the fleet
    assert_equal(ocean3.id, s2.region.id) -- s2 should be part of the fleet
end

function test_mixed_ships_landing()
    local ocean = region.create(1, 0, "ocean")
    local r = region.create(0, 0, "glacier")
    local r2 = region.create(2, 0, "glacier") -- ToDO: Check why a boat can not created in a ocean field (it will disappear)
    local f = faction.create("noreply@eressea.de", "human", "de")
    local s1 = ship.create(r2, "cutter")  -- can land in a glacier
    local s2 = ship.create(r2, "cog") -- can not land in a glacier
    local u1 = unit.create(f, r2, 4)
    
    u1:add_item("money", 1000)
    u1.ship = s1
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH W W")
    u1:add_order("flotte " .. itoa36(s1.id))
    u1:add_order("flotte " .. itoa36(s2.id))
    process_orders()
    assert_equal(ocean.id, u1.region.id) -- fleet should not be able to land in the glacier
    assert_equal(ocean.id, s1.region.id) -- all ships should be still at the ocean
    assert_equal(ocean.id, s2.region.id)
end

function test_boat_landing()
    local ocean = region.create(1, 0, "ocean")
    local r = region.create(0, 0, "glacier")
    local r2 = region.create(2, 0, "glacier")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local s1 = ship.create(r2, "cutter") -- can land in a glacier
    local s2 = ship.create(r2, "cutter") -- can land in a glacier
    local u1 = unit.create(f, r2, 2)
    
    u1:add_item("money", 1000)
    u1.ship = s1
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH W W")
    u1:add_order("flotte " .. itoa36(s1.id))
    u1:add_order("flotte " .. itoa36(s2.id))
    process_orders()
    assert_equal(r.id, u1.region.id) -- fleet should be able to land in the glacier
    assert_equal(r.id, s1.region.id) -- all ships should be in the glacier
    assert_equal(r.id, s2.region.id)
end
