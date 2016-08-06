require "lunit"

module("tests.e3.e3features", package.seeall, lunit.testcase)

local settings

local function set_rule(key, value)
    if value==nil then
        eressea.settings.set(key, settings[key])
    else
        settings[key] = settings[key] or eressea.settings.get(key)
        eressea.settings.set(key, value)
    end
end

function setup()
    eressea.game.reset()
    settings = {}
    set_rule("rules.move.owner_leave", "1")
    set_rule("rules.food.flags", "4")
    set_rule("rules.ship.drifting", "0")
    set_rule("rules.ship.storms", "0")
end

function teardown()
    set_rule("rules.move.owner_leave")
    set_rule("rules.food.flags")
    set_rule("rules.ship.drifting")
    set_rule("rules.ship.storms")
end

function disable_test_bug_1738_build_castle_e3()
    local r = region.create(0, 0, "plain")    
    local f = faction.create("bug_1738@eressea.de", "human", "de")

    local c = building.create(r, "castle")
    c.size = 228

    local u1 = unit.create(f, r, 1)
    u1:set_skill("building", 5)
    u1:add_item("stone", 10000)

    local u2 = unit.create(f, r, 32)
    u2:set_skill("building", 3)
    u2:add_item("stone", 10000)

    u1:clear_orders()
    u1:add_order("MACHE BURG " .. itoa36(c.id))
    -- castle now has size 229.
    u2:clear_orders()
    u2:add_order("MACHE BURG " .. itoa36(c.id))
    -- 32 * 3 makes 96 skill points.
    -- from size 229 to size 250 needs 21 * 3 = 63 points, rest 33.
    -- 33/4 makes 8 points, resulting size is 258.

    process_orders()
    -- resulting size should be 250 because unit 2
    -- does not have the needed minimum skill.
    assert_equal(c.size, 250)
end

function disable_test_alliance()
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
    for f in f1.alliance.factions do
        assert_true(f.id==f1.id or f.id==f2.id)
    end
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

function test_no_stealth()
    local r = region.create(0,0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)

    u:set_skill("stealth", 1)
    assert_equal(-1, u:get_skill("stealth"))
    u:clear_orders()
    u:add_order("LERNEN TARNUNG")
    process_orders()
    assert_equal(-1, u:get_skill("stealth"))
end

function test_no_teach()
    local r = region.create(0,0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)

    u1:clear_orders()
    u2:clear_orders()
    u1:set_skill("riding", 3)
    u2:add_order("LERNE Reiten")
    u1:add_order("LEHRE " .. itoa36(u2.id))
    process_orders()
    -- TODO: assert something (reflecting skills sucks!)
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
    u2:add_order("Zaubere stufe 2 'Beschwoere einen Sturmelementar' " .. itoa36(s1.id))
    u1:clear_orders()
    u1:add_order("NACH O O O O")
    process_orders()
    assert_equal(8, u2.region.x)
end

function test_fishing()
    eressea.settings.set("rules.food.flags", "0")
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
    assert_equal(r2, u1.region)
    assert_equal(90, u1:get_item("money"))

    u1:clear_orders()
    u1:add_order("NACH W")

    process_orders()
    assert_equal(r, u1.region)
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
    if r2~=u1.region then
        print(get_turn(), u1, u1.faction)
    end
    assert_equal(r2, u1.region)
    assert_not_equal(r2.id, u2.region.id)
    if r2~=u3.region then
        print(get_turn(), u3, u3.faction)
    end
    assert_equal(r2, u3.region)
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

function test_region_owner_cannot_leave_castle()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    f.id = 42
    local b1 = building.create(r, "castle")
    b1.size = 10
    local b2 = building.create(r, "lighthouse")
    b2.size = 10
    local u = unit.create(f, r, 1)
    u.building = b1
    u:add_item("money", u.number * 100)
    u:clear_orders()
    u:add_order("BETRETE BURG " .. itoa36(b2.id))
    process_orders()
    assert_equal(b1, u.building, "region owner has left the building") -- region owners may not leave
end

function reset_items(u)
  for i in u.items do
    u:add_item(i, u:get_item(i))
  end
  u:add_item("money", u.number * 10000)
end

function market_fixture()
    local herb_multi = 500 -- from rc_herb_trade()
    local r
    local herbnames = { 'h0', 'h1', 'h2', 'h3', 'h4', 'h5' }
    local herbtable = {}
    for _, name in pairs(herbnames) do
        herbtable[name] = name
    end
    local luxurynames = { 'balm', 'jewel', 'myrrh', 'oil', 'silk', 'incense' } -- no spice in E3
    local luxurytable = {}
    for _, name in pairs(luxurynames) do
        luxurytable[name] = name
    end

    for x = -1, 1 do for y = -1, 1 do
        r = region.create(x, y, "plain")
        r:set_resource("peasant", 1) 
    end end

    r = get_region(0, 0)
    local b = building.create(r, "market")
    b.size = 10
    b.working = true

    local f = faction.create("market1@eressea.de", "human", "de")
    f.id = 42
    local u = unit.create(f, r, 1)
    u.building = b

    r.herb = herbnames[6]
    r.luxury = luxurynames[6]
    r:set_resource("peasant", herb_multi * 9 + 50) -- 10 herbs per region
    for i = 0, 4 do
        local rn = r:next(i)
        rn.name = luxurynames[i+1]
        rn.herb = herbnames[i+1]
        rn.luxury = luxurynames[i+1]
        rn:set_resource("peasant", herb_multi * 9 + 50) -- 10 herbs per region
        unit.create(f, rn, 1)
    end
  
    reset_items(u)  
    return r, u, b, herbnames, luxurynames, herbtable, luxurytable
end

local function test_items(u, names, amount)
  local len = 0
  for i in u.items do
      if names[i] ~= nil then
        len = len + 1
      end
  end
  if amount > 0 then
      assert_not_equal(0, len, "trader did not get any items")
  else
      assert_equal(0, len, "trader should not have items")
  end
  for _, name in pairs(names) do
      local n = u:get_item(name)
      if n>0 then
         if amount == 0 then
            assert_equal(0, n, 'trader should have no ' .. name)
         else
             assert_equal(amount, n, 'trader has ' .. n .. ' instead of ' .. amount .. ' ' .. name)
         end
      end
  end
end

function disable_test_market_regions()
    -- if i am the only trader around, i should be getting all the herbs from all 7 regions
    local r, u, b, herbnames, luxurynames, herbtable, luxurytable = market_fixture()


    eressea.process.markets()

    test_items(u, herbtable, 10)
    test_items(u, luxurytable, 5)
end

function disable_test_multiple_markets()
   local r, u1, b, herbnames, luxurynames, herbtable, luxurytable = market_fixture()
   local r2 = get_region(1,0)
   local f = faction.create("multim@eressea.de", "human", "de")
   local u2 = unit.create(f, r2, 1)
   local b2 = building.create(r2, "market")
   b2.size = 10
   b2.working = true
   reset_items(u2)
   u2.building = b2
   

   eressea.process.markets()
   for _, i in pairs(luxurytable) do
     assert_equal(5, u1:get_item(i)+u2:get_item(i), "not enough " .. i )
   end
   for _, i in pairs(herbtable) do
     assert_equal(10, u1:get_item(i)+u2:get_item(i), "not enough " .. i )
   end
   assert_equal(5, u1:get_item('silk')) -- uncontested
end


function disable_test_market()
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("market2@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
	
  local b = building.create(r, "market")

  eressea.settings.set("rules.peasants.growth", "0")

  b.size = 10
  u1.building = b
  u1:add_item("money", 10000)
  r.herb = "h0"
  r.luxury = "balm"
  r:set_resource("peasant", 1050)
  process_orders()
  assert_equal(3, u1:get_item("h0"))
  assert_equal(2, u1:get_item("balm"))

  local function reset_items()
    for i in u1.items do
      u1:add_item(i, -1 * u1:get_item(i))
    end
    u1:add_item("money", u1.number * 10000)
--    eressea.game.reset()
    f1.lastturn=get_turn()
    assert_not_equal(nil, factions())
    local idx = 0
    for f in factions() do
      assert_equal(1,1,"fac".. f.email)
      idx = idx + 1
    end    
    assert_not_equal(0, idx)
  end
  reset_items()
  b.size = 1  
  eressea.process.markets()

  assert_equal(0, u1:get_item("h0"))
  b.size = 10 


  reset_items()
  r:set_resource("peasant", 2100)
  eressea.process.markets()
  
  assert_equal(5, u1:get_item("h0"))
  assert_equal(3, u1:get_item("balm"))

  reset_items()
  r:set_resource("peasant", 1049)
  eressea.process.markets()
  assert_equal(2, u1:get_item("h0"))
  assert_equal(1, u1:get_item("balm"))

  reset_items()
  r:set_resource("peasant", 550)
  eressea.process.markets()
  assert_equal(2, u1:get_item("h0"))
  assert_equal(1, u1:get_item("balm"))

  reset_items()
  r:set_resource("peasant", 549)
  eressea.process.markets()
  assert_equal(1, u1:get_item("h0"))
  assert_equal(1, u1:get_item("balm"))

  reset_items()
  r:set_resource("peasant", 50)
  eressea.process.markets()
  assert_equal(1, u1:get_item("h0"))
  assert_equal(1, u1:get_item("balm"))

  reset_items()
  r:set_resource("peasant", 49)
  eressea.process.markets()
  assert_equal(0, u1:get_item("h0"))
  r:set_resource("peasant", 1050)

  reset_items()
  u1:add_item("money", -1 * u1:get_item("money"))
  assert_equal(0, u1:get_item("money"))

  process_orders() -- process_orders to update maintenance
  assert_equal(0, u1:get_item("h0"))

  process_orders()
  eressea.settings.set("rules.peasants.growth", "1")
end

function disable_test_market_gives_items()
    local r
    for x = -1, 1 do for y = -1, 1 do
        r = region.create(x, y, "plain")
        r:set_resource("peasant", 5000) 
    end end
    r = get_region(0, 0)
    local b = building.create(r, "market")
    b.size = 10
    local f = faction.create("market0@eressea.de", "human", "de")
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
    elseif u.spells~=nil then
        for sp in u.spells do
            nums = nums + 1
        end
        assert(nums>0)
    end
end

function test_canoe_passes_through_land()
    local f = faction.create("noreply@eressea.de", "human", "de")
    local src = region.create(0, 0, "ocean")
    local land = region.create(1, 0, "plain")
    region.create(2, 0, "ocean")
    local dst = region.create(3, 0, "ocean")
    local sh = ship.create(src, "canoe")
    local u1 = unit.create(f, src, 1)
    local u2 = unit.create(f, src, 1)
    u1.ship = sh
    u2.ship = sh
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH O O O")
    process_orders()
    assert_equal(land, u2.region, "canoe did not stop at coast")
    process_orders()
    assert_equal(dst, sh.region, "canoe could not leave coast")
    assert_equal(dst, u1.region, "canoe could not leave coast")
    assert_equal(dst, u2.region, "canoe could not leave coast")
end

function test_give_50_percent_of_money()
    local r = region.create(0, 0, "plain")
    local u1 = unit.create(faction.create("noreply@eressea.de", "human", "de"), r, 1)
    local u2 = unit.create(faction.create("noreply@eressea.de", "orc", "de"), r, 1)
    u1.faction.age = 10
    u2.faction.age = 10
    u1:add_item("money", 500)
    u2:add_item("money", 500)
    local m1, m2 = u1:get_item("money"), u2:get_item("money")
    u1:clear_orders()
    u1:add_order("GIB " .. itoa36(u2.id) .. " 221 Silber")
    u2:clear_orders()
    u2:add_order("LERNEN Hiebwaffen")
    process_orders()
    assert_equal(m1, u1:get_item("money"))
    assert_equal(m2, u2:get_item("money"))

    m1, m2 = u1:get_item("money"), u2:get_item("money")
    u1:clear_orders()
    u1:add_order("GIB " .. itoa36(u2.id) .. " 221 Silber")
    u2:clear_orders()
    u2:add_order("HELFEN " .. itoa36(u1.faction.id) .. " GIB")
    u2:add_item("horse", 100)
    u2:add_order("GIB 0 ALLES PFERD")
    local h = r:get_resource("horse")
    process_orders()
    assert_true(r:get_resource("horse")>=h+100)
    assert_equal(m1-221, u1:get_item("money"))
    assert_equal(m2+110, u2:get_item("money"))
end

function test_give_100_percent_of_items()
    r = region.create(0, 0, "plain")
    local u1 = unit.create(faction.create("noreply@eressea.de", "human", "de"), r, 1)
    local u2 = unit.create(faction.create("noreply@eressea.de", "orc", "de"), r, 1)
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

local function setup_packice(x, onfoot)
    local f = faction.create("noreply@eressea.de", "human", "de")
    local plain = region.create(0,0, "plain")
    local ice = region.create(1,0, "packice")
    local ocean = region.create(2,0, "ocean")
    local u = unit.create(f, get_region(x, 0), 2)
    if not onfoot then
        local s = ship.create(u.region, "cutter")
        u:set_skill("sailing", 3)
        u.ship = s
    end
    u:add_item("money", 400)
    return u
end

function test_no_sailing_through_packice()
    local u = setup_packice(0)
    u:clear_orders()
    u:add_order("NACH O O")
    process_orders()
    assert_equal(0, u.region.x)
end

function test_can_sail_from_packice_to_ocean()
    local u = setup_packice(1)

    u:clear_orders()
    u:add_order("NACH W")
    process_orders()
    assert_equal(1, u.region.x)

    u:clear_orders()
    u:add_order("NACH O")
    process_orders()
    assert_equal(2, u.region.x)
end

function test_can_sail_into_packice()
    local u = setup_packice(2)
    u:clear_orders()
    u:add_order("NACH W W")
    process_orders()
    assert_equal(1, u.region.x)
end

function test_can_walk_into_packice()
    local u = setup_packice(0, true)
    u:clear_orders()
    u:add_order("NACH O")
    process_orders()
    assert_equal(1, u.region.x)
end

function test_cannot_walk_into_ocean()
    local u = setup_packice(1, true)
    u:clear_orders()
    u:add_order("NACH O")
    process_orders()
    assert_equal(1, u.region.x)
end

function test_p2()
    local f = faction.create("noreply@eressea.de", "human", "de")
    local r = region.create(0, 0, "plain")
    local u = unit.create(f, r, 1)
    r:set_resource("tree", 0)
    u:clear_orders()
    u:add_order("BENUTZE 'Wasser des Lebens'")
    u:add_item("p2", 1)
    u:add_item("log", 10)
    u:add_item("mallorn", 10)
    process_orders()
    assert_equal(5, r:get_resource("tree"))
    assert_equal(0, u:get_item("p2"))
    assert_equal(15, u:get_item("log") + u:get_item("mallorn"))
end

function test_p2_move()
    -- http://bugs.eressea.de/view.php?id=1855
    local f = faction.create("noreply@eressea.de", "human", "de")
    local r = region.create(0, 0, "plain")
    region.create(1, 0, "plain")
    local u = unit.create(f, r, 1)
    r:set_resource("tree", 0)
    u:clear_orders()
    u:add_order("BENUTZE 'Wasser des Lebens'")
    u:add_order("NACH OST")
    u:add_item("horse", 1)
    u:add_item("p2", 1)
    u:add_item("log", 1)
    u:add_item("mallorn", 1)
    process_orders()
    assert_equal(1, u.region.x)
    assert_equal(1, r:get_resource("tree"))
end

function test_golem_use_four_iron()
    local r0 = region.create(0, 0, "plain")
    local f1 = faction.create("noreply@eressea.de", "halfling", "de")
    local u1 = unit.create(f1, r0, 3)
    u1.race = "irongolem"
    u1:set_skill("weaponsmithing", 1)
    u1:set_skill("armorer", 1)
    u1:clear_orders()
    u1:add_order("Mache 4 Turmschild")

    process_orders()

    assert_equal(2, u1.number)
    assert_equal(4, u1:get_item("towershield"))
end

function test_silver_weight_stops_movement()
    local r1 = region.create(1, 1, "plain")
    local r2 = region.create(2, 1, "plain")
    region.create(3, 1, "plain")
    local f1 = faction.create("noreply@eressea.de", "human", "de")    
    local u1 = unit.create(f1, r1, 1)
    u1:clear_orders()
    u1:add_order("NACH OST")
    u1:add_item("money", 540)
    assert_equal(1540, u1.weight)
    process_orders()
    assert_equal(r2, u1.region)
    u1:add_item("money", 1)
    process_orders()
    assert_equal(r2, u1.region)
end

function test_silver_weight_stops_ship()
    local r1 = region.create(1, 1, "ocean")
    local r2 = region.create(2, 1, "ocean")
    region.create(3, 1, "ocean")
    local f1 = faction.create("noreply@eressea.de", "human", "de")    
    local u1 = unit.create(f1, r1, 1)
    u1:set_skill("sailing", 3)
    local s1 = ship.create(r1, "canoe")
    u1.ship = s1
    u1:clear_orders()
    u1:add_order("NACH OST")
    u1:add_item("money", 2000)
    process_orders()
    assert_equal(r2, u1.region)
    u1:add_item("money", 1)
    process_orders()
    assert_equal(r2, u1.region)
end

function test_building_owner_can_enter_ship()
    local r1 = region.create(1, 2, "plain")    
    local f1 = faction.create("noreply@eressea.de", "human", "de")
    local b1 = building.create(r1, "castle")    
    b1.size = 10    
    local s1 = ship.create(r1, "cutter")        

    local u1 = unit.create(f1, r1, 10)    
    u1.building = b1    
    u1:add_item("money", u1.number * 100)    
    u1:clear_orders()    
    u1:add_order("VERLASSEN")    
    u1:add_order("BETRETE SCHIFF " .. itoa36(s1.id))    

    local u2 = unit.create(f1, r1, 10)    
    u2.ship = s1    
    u2:add_item("money", u1.number * 100)    
    u2:clear_orders()    
    process_orders()
    assert_equal(s1, u1.ship)
    assert_equal(null, u1.building, "owner of the building can not go into a ship")
end

function test_only_building_owner_can_set_not_paid()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f, r, 1)
  local u2 = unit.create(f, r, 1)
  local mine = building.create(r, "mine")
  mine.size = 2
  u1:add_item("money", 500)
  u1.building = mine
  u2.building = mine
  u1:clear_orders()
  u2:clear_orders()
-- Test that Bezahle nicht is working
  u1:add_order("Bezahle nicht")
  process_orders()
  assert_equal(500, u1:get_item("money"))
  u1:clear_orders()
-- Test that bug fix 0001976 is working 
-- Bezahle nicht is not working
  u2:add_order("Bezahle nicht")
  process_orders()
  assert_equal(0, u1:get_item("money"))
end

function test_spyreport_message()
    local r1 = region.create(1, 2, "plain")    
    local f1 = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f1, r1, 1)
    local u2 = unit.create(f1, r1, 1)
    msg = message.create("spyreport")
    msg:set_unit("spy", u1)
    msg:set_unit("target", u2)
    msg:set_string("status", "hodor")
    assert_not_equal("", msg:render("de"))
    assert_not_equal("", msg:render("en"))
end

function test_volcanooutbreak_message()
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(1, 1, "plain")
    msg = message.create("volcanooutbreak")
    msg:set_region("regionn", r1)
    msg:set_region("regionv", r2)
    assert_not_equal("", msg:render("de"))
    assert_not_equal("", msg:render("en"))
end

function test_bug2083()
    local herb_multi = 500 -- from rc_herb_trade()
    local r = region.create(0,0,"plain")
    r:set_resource("peasant", 2000) 
    r.luxury = "balm"

    local f = faction.create("2083@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:set_skill("building", 8)
    u:add_item("stone", 100)
    u:add_item("log", 100)
    u:add_item("iron", 100)
    u:add_item("money", 10000)
    u:clear_orders()
    u:add_order("MACHE Markt")
    process_orders()

    -- this is a bit weird, but the bug was caused by market code
    -- being called in two places. We want to make sure this doesn't happen
    for k, v in pairs(rules) do
        set_key("xm09", true)
    	if 'table' == type(v) then
           cb = v['update']
	   if 'function' == type(cb) then
              cb()
	   end
        end
     end

    assert_equal(0, u:get_item("balm"))
end

function test_no_uruk()
  local f1 = faction.create("noreply@eressea.de", "uruk", "de")
  assert_equal(f1.race, "orc")
end

function test_bug2187()
  set_rule("rules.food.flags", "0")

  local r = region.create(0,0,"plain")
  local f = faction.create("2187@eressea.de", "goblin", "de")
  local u = unit.create(f, r, 1)
  u.race = "demon"
  u.hp = u.hp_max * u.number

  r:set_resource("peasant", 0)
  u:add_item("money", 500)

  hp = u.hp
  process_orders()
  assert_equal(hp, u.hp)
--  init_reports()
--  write_report(f)
      
  set_rule("rules.food.flags", "4")
end
