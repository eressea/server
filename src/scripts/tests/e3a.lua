require "lunit"

module( "e3", package.seeall, lunit.testcase )

function setup()
    free_game()
end

function test_capacity()
    local r = region.create(0,0, "ocean")
    region.create(1,0, "ocean")
    local r2 = region.create(2,0, "ocean")
    local f = faction.create("noreply@eressea.de", "human", "de")
    
    local s1 = ship.create(r, "cutter")
    local u1 = unit.create(f, r, 5)
    u1.ship = s1
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH O O")
    
    local s2 = ship.create(r, "cutter")
    local u2 = unit.create(f, r, 6)
    u2.ship = s2
    u2:set_skill("sailing", 10)
    u2:clear_orders()
    u2:add_order("NACH O O")

    update_owners()
    process_orders()
--    print(s.region, u.region, r2)
    assert_equal(r2.id, u1.region.id)
    assert_not_equal(r2.id, u2.region.id)
end
    
function test_owners()
  free_game()
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local u2 = unit.create(f2, r, 1)
  local u3 = unit.create(f2, r, 1)

  local b3 = building.create(r, "castle")
  b3.size = 2
  u3.building = b3
  local b1 = building.create(r, "castle")
  b1.size = 1
  u1.building = b1
  local b2 = building.create(r, "castle")
  b2.size = 2
  u2.building = b2
  
  update_owners()
  assert(r.owner==u3.faction)
  b1.size=3
  b2.size=3
  update_owners()
  assert(r.owner==u2.faction)
  b1.size=4
  update_owners()
  assert(r.owner==u1.faction)
end

function test_taxes()
  free_game()
  local r = region.create(0, 0, "plain")
  r:set_resource("peasant", 1000)
  r:set_resource("money", 5000)
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r, 1)
  u:add_item("money", u.number * 10)
  u:clear_orders()
  u:add_order("LERNE Holzfaellen") -- do not work
  local b = building.create(r, "watch")
  b.size = 10
  u.building = b
  update_owners()
  process_orders()
  assert_equal(1, r.morale)
  assert_equal(25, u:get_item("money"))
end

function test_leave()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  f.id = 42
  local b1 = building.create(r, "castle")
  b1.size = 10
  local b2 = building.create(r, "lighthouse")
  b2.size = 10
  local u = unit.create(f, r, 1)
  u.building = b1
  assert_not_equal(nil, u.building)
  u:add_item("money", u.number * 100)
  u:clear_orders()
  u:add_order("BETRETE BURG " .. itoa36(b2.id))
  update_owners()
  process_orders()
  assert_equal(u.building.id, b1.id, "region owner has left the building") -- region owners may not leave
end

function test_market()
  -- if i am the only trader around, i should be getting all the herbs from all 7 regions
  local herb_multi = 500 -- from rc_herb_trade()
  free_game()
  local r, idx
  local herbnames = { 'h0', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'h7', 'h8' }
  idx = 1
  for x = -1, 1 do for y = -1, 1 do
    r = region.create(x, y, "plain")
    r:set_resource("peasant", herb_multi * 10) -- 10 herbs per region
    r.herb = herbnames[idx]
    idx = idx+1
  end end
  r = get_region(0, 0)
  local b = building.create(r, "market")
  b.size = 10
  local f = faction.create("noreply@eressea.de", "human", "de")
  f.id = 42
  local u = unit.create(f, r, 1)
  u.building = b
  u:add_item("money", u.number * 10000)
  for i = 0, 5 do
    local rn = r:next(i)
  end
  process.markets()
  u:add_item("money", -u:get_item("money")) -- now we only have herbs
  local len = 0
  for i in u.items do
    len = len + 1
  end
  assert_not_equal(0, len, "trader did not get any herbs")
  for idx, name in pairs(herbnames) do
    local n = u:get_item(name)
    if n>0 then
      assert_equal(10, n, 'trader did not get exaxtly 10 herbs')
    end
  end
end

function jest_market_gives_items()
  free_game()
  local r
  for x = -1, 1 do for y = -1, 1 do
    r = region.create(x, y, "plain")
    r:set_resource("peasant", 5000) 
  end end
  r = get_region(0, 0)
  local b = building.create(r, "market")
  b.size = 10
  local f = faction.create("noreply@eressea.de", "human", "de")
  f.id = 42
  local u = unit.create(f, r, 1)
  u.building = b
  u:add_item("money", u.number * 10000)
  for i = 0, 5 do
    local rn = r:next(i)
  end
  process_orders()
  local len = 0
  for i in u.items do
    len = len + 1
  end
  assert(len>1)
end

function test_spells()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r, 1)
  u.race = "elf"
  u:clear_orders()
  u:add_item("money", 10000)
  u:set_skill("magic", 5)
  u:add_order("LERNE MAGIE Illaun")
  process_orders()
  local sp
  local nums = 0
  if f.spells~=nil then
    for sp in f.spells do
      nums = nums + 1
    end
    assert(nums>0)
    for sp in u.spells do
      nums = nums - 1
    end
    assert(nums==0)
  else
    for sp in u.spells do
      nums = nums + 1
    end
    assert(nums>0)
  end
end

function test_alliance()
  free_game()
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  u1:add_item("money", u1.number * 100)
  local f2 = faction.create("info@eressea.de", "human", "de")
  local u2 = unit.create(f2, r, 1)
  u2:add_item("money", u2.number * 100)
  assert(f1.alliance==nil)
  assert(f2.alliance==nil)
  u1:clear_orders()
  u2:clear_orders()
  u1:add_order("ALLIANZ NEU pink")
  u1:add_order("ALLIANZ EINLADEN " .. itoa36(f2.id))
  u2:add_order("ALLIANZ BEITRETEN pink")
  process_orders()
  assert(f1.alliance~=nil)
  assert(f2.alliance~=nil)
  assert(f2.alliance==f1.alliance)
  u1:clear_orders()
  u2:clear_orders()
  u1:add_order("ALLIANZ KOMMANDO " .. itoa36(f2.id))
  process_orders()
  assert(f1.alliance~=nil)
  assert(f2.alliance~=nil)
  assert(f2.alliance==f1.alliance)
  u1:clear_orders()
  u2:clear_orders()
  u2:add_order("ALLIANZ AUSSTOSSEN " .. itoa36(f1.id))
  process_orders()
  assert(f1.alliance==nil)
  assert(f2.alliance~=nil)
  u1:clear_orders()
  u2:clear_orders()
  u2:add_order("ALLIANZ NEU zing")
  u1:add_order("ALLIANZ BEITRETEN zing") -- no invite!
  process_orders()
  assert(f1.alliance==nil)
  assert(f2.alliance~=nil)
  u1:clear_orders()
  u2:clear_orders()
  u1:add_order("ALLIANZ NEU zack")
  u1:add_order("ALLIANZ EINLADEN " .. itoa36(f2.id))
  u2:add_order("ALLIANZ BEITRETEN zack")
  process_orders()
  assert(f1.alliance==f2.alliance)
  assert(f2.alliance~=nil)
end

function test_morale()
  free_game()
  local r = region.create(0, 0, "plain")
  assert_equal(1, r.morale)
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local u2 = unit.create(f2, r, 1)

  local b = building.create(r, "castle")
  b.size = 10
  u1.building = b
  u2.building = b
  update_owners()
  assert_equal(1, r.morale)
  r.morale = 5
  assert_equal(r.owner, u1.faction)
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " KOMMANDO")
  process_orders()
  u1:clear_orders()
  assert_equal(u2.faction, r.owner)
  assert_equal(3, r.morale) --  5-MORALE_TRANSFER
  u2.building = nil
  update_owners()
  assert_equal(r.owner, u1.faction)
  assert_equal(0, r.morale)
end

function test_canoe_passes_through_land()
  free_game()
  local f = faction.create("noreply@eressea.de", "human", "de")
  local src = region.create(0, 0, "ocean")
  local land = region.create(1, 0, "plain")
  region.create(2, 0, "ocean")
  local dst = region.create(3, 0, "ocean")
  local sh = ship.create(src, "canoe")
  local u1, u2 = two_units(src, f, f)
  u1.ship = sh
  u2.ship = sh
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH O O O")
  process_orders()
  assert_equal(u2.region.id, land.id, "canoe did not stop at coast")
  u1:add_order("NACH O O O")
  process_orders()
  write_reports()
  assert_equal(u2.region.id, dst.id, "canoe could not leave coast")
end

function test_give_only_a_third_of_items()
  free_game()
  local u1, u2 = two_units(region.create(0, 0, "plain"), two_factions())
  local r = u2.region
  u1.faction.age = 10
  u2.faction.age = 10
  u1:add_item("money", 500)
  local m1, m2 = u1:get_item("money"), u2:get_item("money")
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " 332 Silber")
  u2:clear_orders()
  u2:add_order("LERNEN Hiebwaffen")
  process_orders()
  assert(u1:get_item("money")==m1-10*u1.number)
  assert(u2:get_item("money")==m2-10*u2.number)

  m1, m2 = u1:get_item("money"), u2:get_item("money")
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " 332 Silber")
  u2:clear_orders()
  u2:add_order("HELFE " .. itoa36(u1.faction.id) .. " GIB")
  u2:add_item("horse", 100)
  u2:add_order("GIB 0 ALLES PFERD")
  local h = r:get_resource("horse")
  process_orders()
  assert(r:get_resource("horse")>=h+100)
  assert_equal(m1-332-10*u1.number, u1:get_item("money"))
  assert_equal(m2+110-10*u2.number, u2:get_item("money"))
end
