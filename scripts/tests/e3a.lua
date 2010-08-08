require "lunit"

module( "e3", package.seeall, lunit.testcase )

function setup()
    free_game()
end

function has_attrib(u, value)
    for a in u.attribs do
        if (a.data==value) then return true end
    end
    return false
end

function test_attrib()
    local r = region.create(0,0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    data = { arr = { 'a', 'b', 'c' }, name = 'familiar', events = { die = 'familiar_died' }, data = { mage = u2 } }
    a = { 'a' }
    b = { 'a' }
    uno = u.id
    u2no = u2.id
    a = attrib.create(u, 12)
    a = attrib.create(u, "enno")
    a = attrib.create(u, u2)
    a = attrib.create(u, data)
    write_game("attrib.dat")
    free_game()
    read_game("attrib.dat")
    u = get_unit(uno)
    u2 = get_unit(u2no)
    assert_false(has_attrib(u, 42))
    assert_true(has_attrib(u, "enno"))
    assert_true(has_attrib(u, 12))

    for a in u.attribs do
        x = a.data
        if (type(x)=="table") then
            assert_equal('a', x.arr[1])
            assert_equal('familiar', x.name)
            assert_equal('familiar_died', x.events.die)
            assert_equal(u2, x.data.mage)
            break
        end
    end
end

function test_seecast()
    local r = region.create(0,0, "plain")
    for i = 1,10 do
      -- this prevents storms (only high seas have storms)
      region.create(i, 1, "plain")
    end
    for i = 1,10 do
      region.create(i, 0, "ocean")
    end
    local f = faction.create("noreply@eressea.de", "human", "de")
    local s1 = ship.create(r, "cutter")
    local u1 = unit.create(f, r, 2)
    u1:set_skill("sailing", 3)
    u1:add_item("money", 1000)
    u1.ship = s1
    local u2 = unit.create(f, r, 1)
    u2.race = "elf"
    u2:set_skill("magic", 6)
    u2.magic = "gwyrrd"
    u2.aura = 60
    u2.ship = s1
    u2:add_spell("stormwinds")
    update_owners()
    u2:clear_orders()
    u2:add_order("Zaubere stufe 2 'Beschwoere einen Sturmelementar' " .. itoa36(s1.id))
    u1:clear_orders()
    u1:add_order("NACH O O O O")
    process_orders()
    assert_equal(4, u2.region.x)

    u2:clear_orders()
    u2:add_spell("stormwinds")
    u2:add_order("Zaubere stufe 2 'Beschwoere einen Sturmelementar' " .. itoa36(s1.id))
    u1:clear_orders()
    u1:add_order("NACH O O O O")
    process_orders()
    write_reports()
    assert_equal(8, u2.region.x)
end

local function use_tree(terrain)
  local r = region.create(0,0, terrain)
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f, r, 5)
  r:set_resource("tree", 0)
  u1:add_item("xmastree", 1)
  u1:clear_orders()
  u1:add_order("BENUTZEN 1 Weihnachtsbaum")
  process_orders()
  return r
end

function test_xmas2009()
  local r = region.create(0,0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f, r, 1)
  process_orders()
  xmas2009()
  assert_equal("xmastree", f.items())
end

function test_xmastree()
  local r
  r = use_tree("ocean")
  assert_equal(0, r:get_resource("tree"))
  free_game()
  r = use_tree("plain")
  assert_equal(10, r:get_resource("tree"))
end

function test_fishing()
    local r = region.create(0,0, "ocean")
    local r2 = region.create(1,0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local s1 = ship.create(r, "cutter")
    local u1 = unit.create(f, r, 3)
    u1.ship = s1
    u1:set_skill("sailing", 10)
    u1:add_item("money", 100)
    u1:clear_orders()
    u1:add_order("NACH O")
    update_owners()

    process_orders()
    assert_equal(r2.id, u1.region.id)
    assert_equal(90, u1:get_item("money"))

    u1:clear_orders()
    u1:add_order("NACH W")

    process_orders()
    assert_equal(60, u1:get_item("money"))
end

function test_ship_capacity()
    local r = region.create(0,0, "ocean")
    region.create(1,0, "ocean")
    local r2 = region.create(2,0, "ocean")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local f2 = faction.create("noreply@eressea.de", "goblin", "de")

    -- u1 is at the limit and moves
    local s1 = ship.create(r, "cutter")
    local u1 = unit.create(f, r, 5)
    u1.ship = s1
    u1:set_skill("sailing", 10)
    u1:add_item("sword", 55)
    u1:clear_orders()
    u1:add_order("NACH O O")

    -- u2 has too many people
    local s2 = ship.create(r, "cutter")
    local u2 = unit.create(f, r, 6)
    u2.ship = s2
    u2:set_skill("sailing", 10)
    u2:clear_orders()
    u2:add_order("NACH O O")

    -- u3 has goblins, they weigh 40% less
    local s3 = ship.create(r, "cutter")
    local u3 = unit.create(f2, r, 8)
    u3.ship = s3
    u3:set_skill("sailing", 10)
    u3:add_item("sword", 55)
    u3:clear_orders()
    u3:add_order("NACH O O")

    -- u4 has too much stuff
    local s4 = ship.create(r, "cutter")
    local u4 = unit.create(f, r, 5)
    u4.ship = s4
    u4:set_skill("sailing", 10)
    u4:add_item("sword", 56)
    u4:clear_orders()
    u4:add_order("NACH O O")

    update_owners()
    process_orders()
    assert_equal(r2.id, u1.region.id)
    assert_not_equal(r2.id, u2.region.id)
    assert_equal(r2.id, u3.region.id)
    assert_not_equal(r2.id, u4.region.id)
end
    
function test_owners()
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
  assert_equal(1, r.morale)
  process_orders()
  assert_equal(1, r.morale)
  assert_equal(25, u:get_item("money"))
end

function test_leave()
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
  local r, idx
  local herbnames = { 'h0', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'h7', 'h8' }
  idx = 1
  for x = -1, 1 do for y = -1, 1 do
    r = region.create(x, y, "plain")
    r:set_resource("peasant", herb_multi * 9 + 50) -- 10 herbs per region
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

function test_market_gives_items()
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

function test_canoe_passes_through_land()
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
  assert_equal(u2.region.id, dst.id, "canoe could not leave coast")
end

function test_give_50_percent_of_money()
  local u1, u2 = two_units(region.create(0, 0, "plain"), two_factions())
  local r = u2.region
  u1.faction.age = 10
  u2.faction.age = 10
  u1:add_item("money", 500)
  local m1, m2 = u1:get_item("money"), u2:get_item("money")
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " 221 Silber")
  u2:clear_orders()
  u2:add_order("LERNEN Hiebwaffen")
  process_orders()
  assert(u1:get_item("money")==m1-10*u1.number)
  assert(u2:get_item("money")==m2-10*u2.number)

  m1, m2 = u1:get_item("money"), u2:get_item("money")
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " 221 Silber")
  u2:clear_orders()
  u2:add_order("HELFEN " .. itoa36(u1.faction.id) .. " GIB")
  u2:add_item("horse", 100)
  u2:add_order("GIB 0 ALLES PFERD")
  local h = r:get_resource("horse")
  process_orders()
  assert(r:get_resource("horse")>=h+100)
  assert_equal(m1-221-10*u1.number, u1:get_item("money"))
  assert_equal(m2+110-10*u2.number, u2:get_item("money"))
end

function test_give_100_percent_of_items()
  local u1, u2 = two_units(region.create(0, 0, "plain"), two_factions())
  local r = u2.region
  u1.faction.age = 10
  u2.faction.age = 10
  u1:add_item("money", 500)
  u1:add_item("log", 500)
  local m1, m2 = u1:get_item("log"), u2:get_item("log")
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " 332 Holz")
  u2:clear_orders()
  u2:add_order("LERNEN Hiebwaffen")
  u2:add_order("HELFEN " .. itoa36(u1.faction.id) .. " GIB")
  process_orders()
  assert_equal(m1-332, u1:get_item("log"))
  assert_equal(m2+332, u2:get_item("log"))
end

function test_cannot_give_person()
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 10)
  local u2 = unit.create(f2, r, 10)
  u1.faction.age = 10
  u2.faction.age = 10
  u1:add_item("money", 500)
  u2:add_item("money", 500)
  u2:clear_orders()
  u2:add_order("GIB ".. itoa36(u1.id) .. " 1 PERSON")
  u2:add_order("HELFE ".. itoa36(f1.id) .. " GIB")
  u1:add_order("HELFE ".. itoa36(f2.id) .. " GIB")
  process_orders()
  assert_equal(10, u2.number)
  assert_equal(10, u1.number)
end

function test_cannot_give_unit()
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 10)
  local u2 = unit.create(f2, r, 10)
  u1.faction.age = 10
  u2.faction.age = 10
  u1:add_item("money", 500)
  u2:add_item("money", 500)
  u2:clear_orders()
  u2:add_order("GIB ".. itoa36(u1.id) .. " EINHEIT")
  u2:add_order("HELFE ".. itoa36(f1.id) .. " GIB")
  u1:add_order("HELFE ".. itoa36(f2.id) .. " GIB")
  process_orders()
  assert_not_equal(u2.faction.id, u1.faction.id)
end

function test_guard_by_owners()
  -- http://bugs.eressea.de/view.php?id=1756
  local r = region.create(0,0, "mountain")
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  f1.age=20
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  f2.age=20
  local u1 = unit.create(f1, r, 1)
  local b = building.create(r, "castle")
  b.size = 10
  u1.building = b
  u1:add_item("money", 100)
  
  local u2 = unit.create(f2, r, 1)
  u2:add_item("money", 100)
  u2:set_skill("mining", 3)
  u2:clear_orders()
  u2:add_order("MACHEN EISEN")
 
  process_orders()
  local iron = u2:get_item("iron")
  process_orders()
  assert_equal(iron, u2:get_item("iron"))
end

function test_market_action()
  local f = faction.create("noreply@eressea.de", "human", "de")
  local x, y, r
  for x=0,2 do
    for y=0,2 do
      r = region.create(x, y, "plain")
      r.luxury = "balm"
      r.herb = "h2"
      r:set_resource("peasant", 5000)
    end
  end
  r = get_region(1, 1)
  local u = unit.create(f, r, 1)
  b = building.create(r, "market")
  b.size = 10
  u.building = b
  update_owners()
  for r in regions() do
    market_action(r)
  end
  assert_equal(35, u:get_item("balm"))
  assert_equal(70, u:get_item("h2"))
end

-- function test_process_execute()
  -- local i = 0
  -- local f = faction.create("noreply@eressea.de", "human", "de")
  -- local r = region.create(0, 0, "plain")
  -- local u = unit.create(f, r, 1)
  -- local r1, u1

  -- function a() i = 2 end
  -- function b() i = i * 2 end
  -- function c(r) r1 = r  i = i + 1 end
  -- function d(u) u1 = u  i = i * 3 end
  -- process.execute({a, b}, {c}, {d})
  -- assert_equal(15, i)
  -- assert_equal(r, r1)
  -- assert_equal(u, u1)
-- end

-- function test_new_orders()
  -- local i = 0
  -- local f = faction.create("noreply@eressea.de", "human", "de")
  -- local r = region.create(0, 0, "plain")
  -- local u = unit.create(f, r, 1)
  -- local r1, u1

  -- function a() i = 2 end
  -- function b() i = i * 2 end
  -- function c(r) r1 = r  i = i + 1 end
  -- function d(u) u1 = u  i = i * 3 end

  -- process.setup()
  -- process.push(a)
  -- process.push(b)
  -- process.push(c, "region")
  -- process.push(d, "unit")
  -- process.start()

  -- assert_equal(15, i)
  -- assert_equal(r, r1)
  -- assert_equal(u, u1)
-- end
