require "lunit"

module("tests.e2.shiplanding", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
end

function test_landing1()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f2, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 1000)
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(r.id, u1.region.id) -- the plain case: okay
end

function test_landing_harbour_with_help()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "glacier")
  local harbour = building.create(r, "harbour")
  harbour.size = 25
  local f = faction.create("noreply@eressea.de", "human", "de")
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f2, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 1000)
  u2.building = harbour
  u2:clear_orders()
  u2:add_order("KONTAKTIERE " .. itoa36(u1.id))
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(r.id, u1.region.id) -- glacier with harbour and help-- okay
end

function test_landing_harbour_without_help()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "glacier")
  local harbour = building.create(r, "harbour")
  harbour.size = 25
  local f = faction.create("noreply@eressea.de", "human", "de")
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f2, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 1000)
  u2.building = harbour
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(ocean.id, u1.region.id) -- glacier with harbour and no help-- cannot land
end

function test_landing_harbour_unpaid()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "glacier")
  local harbour = building.create(r, "harbour")
  harbour.size = 25
  local f = faction.create("noreply@eressea.de", "human", "de")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 20)
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(ocean.id, u1.region.id) -- did not pay 
end

function test_landing_terrain()
    local ocean = region.create(1, 0, "ocean")
    local r = region.create(0, 0, "glacier")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local f2 = faction.create("noreply@eressea.de", "human", "de")
    local s = ship.create(ocean, "longboat")
    local u1 = unit.create(f, ocean, 1)
    local u2 = unit.create(f2, r, 1)
    assert_not_nil(u2)
    u1:add_item("money", 1000)
    u2:add_item("money", 1000)

    u1.ship = s
    u1:set_skill("sailing", 10)    u1:clear_orders()
    u1:add_order("NACH w")
    process_orders()

    assert_equal(ocean.id, u1.region.id) -- cannot land in glacier without harbour
end

function test_landing_insects()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "glacier")
  local harbour = building.create(r, "harbour")
  harbour.size = 25
  local f = faction.create("noreply@eressea.de", "insect", "de")
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f2, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 1000)
  u2.building = harbour
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(ocean.id, u1.region.id) -- insects cannot land in glaciers
end
