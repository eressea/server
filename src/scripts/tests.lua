local function test_rename()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r)
  u:add_item("aoh", 1)
  assert(u:get_item("ao_healing")==1)
end

local function test_blessed()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r)
  r:set_resource("peasant", 100)
  r:set_resource("money", 0)
  u:add_item("money", 1000)
  u.magic = "gwyrrd"
  u.race = "elf"
  u:set_skill("magic", 20)
  u.aura = 200
  u:add_spell("raindance")
  u:add_spell("blessedharvest")
  u:clear_orders()
  u:add_order("ZAUBERE STUFE 3 Regentanz")
  print(r:get_resource("money"))

  process_orders()
  print(r:get_resource("money"))
  u:clear_orders()
  u:add_order("ARBEITEN")
  for i=1,3 do
    process_orders()
    print(r:get_resource("money"))
  end
end

local function test_pure()
  free_game()
  local r = region.create(0, 0, "plain")
end

local function test_read_write()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r)
  u.number = 2
  local fno = f.id
  local uno = u.id
  local result = 0
  assert(r.terrain=="plain")
  result = write_game("test_read_write.dat", "binary")
  assert(result==0)
  assert(get_region(0, 0)~=nil)
  assert(get_faction(fno)~=nil)
  assert(get_unit(uno)~=nil)
  r = nil
  f = nil
  u = nil
  free_game()
  assert(get_region(0, 0)==nil)
  assert(get_faction(fno)==nil)
  assert(get_unit(uno)==nil)
  result = read_game("test_read_write.dat", "binary")
  assert(result==0)
  assert(get_region(0, 0)~=nil)
  assert(get_faction(fno)~=nil)
  assert(get_unit(uno)~=nil)
  free_game()
end

local function test_gmtool()
    free_game()
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(1, 1, "plain")
    local r3 = region.create(1, 2, "plain")
    gmtool.open()
    gmtool.select(r1, true)
    gmtool.select_at(0, 1, true)
    gmtool.select(r2, true)
    gmtool.select_at(0, 2, true)
    gmtool.select(r3, false)
    gmtool.select(r3, true)
    gmtool.select_at(0, 3, false)
    gmtool.select(r3, false)
    
    local selections = 0
    for r in gmtool.get_selection() do
        selections=selections+1
    end
    assert(selections==2)
    assert(gmtool.get_cursor()==nil)

    gmtool.close()
end

local function test_faction()
    free_game()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    assert(f)
    f.info = "Spazz"
    assert(f.info=="Spazz")
    f:add_item("donotwant", 42)
    f:add_item("stone", 42)
    f:add_item("sword", 42)
    local items = 0
    for u in f.items do
        items = items + 1
    end
    assert(items==2)
    unit.create(f, r)
    unit.create(f, r)
    local units = 0
    for u in f.units do
        units = units + 1
    end
    assert(units==2)
end

local function test_unit()
    free_game()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r)
    u.number = 20
    u.name = "Enno"
    assert(u.name=="Enno")
    u.info = "Spazz"
    assert(u.info=="Spazz")
    u:add_item("sword", 4)
    assert(u:get_item("sword")==4)
    assert(u:get_pooled("sword")==4)
    u:use_pooled("sword", 2)
    assert(u:get_item("sword")==2)
end

local function test_region()
  free_game()
  local r = region.create(0, 0, "plain")
  r:set_resource("horse", 42)
  r:set_resource("money", 45)
  r:set_resource("peasant", 200)
  assert(r:get_resource("horse") == 42)
  assert(r:get_resource("money") == 45)
  assert(r:get_resource("peasant") == 200)
  r.name = nil
  r.info = nil
  assert(r.name=="")
  assert(r.info=="")
  r.name = "Alabasterheim"
  r.info = "Hier wohnen die siebzehn Zwerge"
  assert(tostring(r) == "Alabasterheim (0,0)")
end

local function test_building()
  free_game()
  local u
  local f = faction.create("noreply@eressea.de", "human", "de")
  local r = region.create(0, 0, "plain")
  local b = building.create(r, "castle")
  u = unit.create(f, r)
  u.number = 1
  u.building = b
  u = unit.create(f, r)
  u.number = 2
  -- u.building = b
  u = unit.create(f, r)
  u.number = 3
  u.building = b
  local units = 0
  for u in b.units do
      units = units + 1
  end
  assert(units==2)
  local r2 = region.create(0, 1, "plain")
  assert(b.region==r)
  b.region = r2
  assert(b.region==r2)
  assert(r2.buildings()==b)
end

local function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

local function test_message()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r)
  local msg = message.create("item_create_spell")
  msg:set_unit("mage", u)
  msg:set_int("number", 1)
  msg:set_resource("item", "sword")
  msg:send_region(r)
  msg:send_faction(f)
  
  return msg
end

local function test_hashtable()
  free_game()
  local f = faction.create("noreply@eressea.de", "human", "de")
  f.objects:set("enno", "smart guy")
  f.objects:set("age", 10)
  assert(f.objects:get("jesus") == nil)
  assert(f.objects:get("enno") == "smart guy")
  assert(f.objects:get("age") == 10)
  f.objects:set("age", nil)
  assert(f.objects:get("age") == nil)
end

function test_events()
  local fail = 1
  local function msg_handler(u, evt)
    str = evt:get(0)
    u2 = evt:get(1)
    assert(u2~=nil)
    assert(str=="Du Elf stinken")
    message_unit(u, u2, "thanks unit, i got your message: " .. str)
    message_faction(u, u2.faction, "thanks faction, i got your message: " .. str)
    message_region(u, "thanks region, i got your message: " .. str)
    fail = 0
  end

  plain = region.create(0, 0, "plain")
  skill = 8

  f = faction.create("noreply@eressea.de", "orc", "de")
  f.age = 20

  u = unit.create(f, plain)
  u.number = 1
  u:add_item("money", u.number*100)
  u:clear_orders()
  u:add_order("NUMMER PARTEI test")
  u:add_handler("message", msg_handler)
  msg = "BOTSCHAFT EINHEIT " .. itoa36(u.id) .. " Du~Elf~stinken"
  f = faction.create("noreply@eressea.de", "elf", "de")
  f.age = 20

  u = unit.create(f, plain)
  u.number = 1
  u:add_item("money", u.number*100)
  u:clear_orders()
  u:add_order("NUMMER PARTEI eviL")
  u:add_order(msg)
  process_orders()
  assert(fail==0)
end

local function test_recruit2()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r)
  u.number = 1
  u:add_item("money", 2000)
  u:clear_orders()
  u:add_order("MACHE TEMP 1")
  u:add_order("REKRUTIERE 1 Elf")
  u:add_order("REKRUTIERE 1 mensch")
  u:add_order("REKRUTIERE 1")
  process_orders()
end

local function test_guard()
  free_game()
  region.create(1, 0, "plain")
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  f1.age = 20
  local u1 = unit.create(f1, r, 1)
  u1:add_item("sword", 10)
  u1:set_skill("melee", 10)
  u1:clear_orders()
  u1:add_order("NACH O")

  local f2 = faction.create("noreply@eressea.de", "human", "de")
  f2.age = 20
  local u2 = unit.create(f2, r, 1)
  local u3 = unit.create(f2, r, 1)
  local b = building.create(r, "castle")
  b.size = 10
  u2.building = b
  u3.building = b
  update_owners()
  u2:clear_orders()
  u2:add_order("ATTACKIEREN " .. itoa36(u1.id)) -- you will die...
  u2:add_item("money", 100)
  u3:add_item("money", 100)
  process_orders()
  assert(u1.region==r)
end

local function test_owners()
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

local function test_morale()
  free_game()
  local r = region.create(0, 0, "plain")
  assert(r.morale==1)
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local u2 = unit.create(f2, r, 1)

  local b = building.create(r, "castle")
  b.size = 10
  u1.building = b
  u2.building = b
  update_owners()
  assert(r.morale==1)
  r.morale = 5
  assert(r.owner==u1.faction)
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " KOMMANDO")
  process_orders()
  u1:clear_orders()
  assert(r.owner==u2.faction)
  assert(r.morale==3) --  5-MORALE_TRANSFER
  u2.building = nil
  update_owners()
  assert(r.owner==u1.faction)
  assert(r.morale==0)
end

local function test_recruit()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r)
  u.number = 1
  local n = 3
  r:set_resource("peasant", 200)
  u:clear_orders()
  u:add_item("money", 110*n+20)
  u:add_order("REKRUTIERE " .. n)
  process_orders()
  assert(u.number == n+1)
  local p = r:get_resource("peasant")
  assert(p<200 and p>=200-n)
  -- assert(u:get_item("money")==10)
end

local function test_spells()
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

local function test_produce()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r, 1)
  u:clear_orders()
  u:set_skill("weaponsmithing", 3)
  u:add_item("iron", 2)
  u:add_item("money", u.number * 10)
  u:add_order("MACHE Schwert")
  process_orders()
  assert(u:get_item("iron")==1)
  assert(u:get_item("sword")==1)
end

local function test_alliance()
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

local function spells_csv()
  local f = io.open("spells.csv", "w")
  for sp in spells() do
    f:write('"' .. sp.name .. '",' .. sp.level .. ',' .. sp.school .. ',"' .. sp.text .. '"\n')
  end
  f:close()
  fail = 1
end

function test_taxes()
  free_game()
  local r = region.create(0, 0, "plain")
  r.peasants = 1000
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
  assert(r.morale==1)
  assert(u:get_item("money")==25)
end

function test_market()
  free_game()
  local r
  for x = -1, 1 do for y = -1, 1 do
    r = region.create(x, y, "plain")
    r.peasants = 5000
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

function test_work()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  f.id = 42
  local u = unit.create(f, r, 1)
  u:add_item("money", u.number * 10) -- humans cost 10
  u:set_skill("herbalism", 5)
  u:clear_orders()
  u:add_order("ARBEITEN")
  process_orders()
  assert(u:get_item("money")>=10)
end

function test_upkeep()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  f.id = 42
  local u = unit.create(f, r, 5)
  u:add_item("money", u.number * 11)
  u:clear_orders()
  u:add_order("LERNE Waffenbau")
  process_orders()
  assert(u:get_item("money")==u.number)
end

function test_id()
  free_game()
  local r = region.create(0, 0, "plain")

  local f = faction.create("noreply@eressea.de", "human", "de")
  f.id = atoi36("42")
  assert(get_faction(42)~=f)
  assert(get_faction("42")==f)
  assert(get_faction(atoi36("42"))==f)

  local u = unit.create(f, r, 1)
  u.id = atoi36("42")
  assert(get_unit(42)~=u)
  assert(get_unit("42")==u)
  assert(get_unit(atoi36("42"))==u)

  local b = building.create(r, "castle")
  -- <not working> b.id = atoi36("42")
  local fortytwo = itoa36(b.id)
  assert(get_building(fortytwo)==b)
  assert(get_building(atoi36(fortytwo))==b)

  local s = ship.create(r, "canoe")
  if (s==nil) then
    s = ship.create(r, "boat")
  end
  -- <not working> s.id = atoi36("42")
  local fortytwo = itoa36(s.id)
  assert(get_ship(fortytwo)==s)
  assert(get_ship(atoi36(fortytwo))==s)
end

function test_herbalism()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  f.id = 42
  local u = unit.create(f, r, 1)
  u:add_item("money", u.number * 100)
  u:set_skill("herbalism", 5)
  u:clear_orders()
  u:add_order("MACHE Samen")
  process_orders()
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
  u:add_item("money", u.number * 100)
  u:clear_orders()
  u:add_order("BETRETE BURG " .. itoa36(b2.id))
  update_owners()
  process_orders()
  assert(u.building==b1)
end

function test_mallorn()
  free_game()
  local r = region.create(0, 0, "plain")
  r:set_flag(1, false) -- not mallorn
  r:set_resource("tree", 100)
  assert(r:get_resource("tree")==100)
  local m = region.create(0, 0, "plain")
  m:set_flag(1, true) -- not mallorn
  m:set_resource("tree", 100)
  assert(m:get_resource("tree")==100)
  
  local f = faction.create("noreply@eressea.de", "human", "de")

  local u1 = unit.create(f, r, 1)
  u1:add_item("money", u1.number * 100)
  u1:set_skill("forestry", 2)
  u1:clear_orders()
  u1:add_order("MACHE HOLZ")

  local u2 = unit.create(f, m, 1)
  u2:add_item("money", u2.number * 100)
  u2:set_skill("forestry", 2)
  u2:clear_orders()
  u2:add_order("MACHE HOLZ")

  local u3 = unit.create(f, m, 1)
  u3:add_item("money", u3.number * 100)
  u3:set_skill("forestry", 2)
  u3:clear_orders()
  u3:add_order("MACHE Mallorn")
  process_orders()
  assert(u1:get_item("log")==2)
  assert(u2:get_item("log")==2)
  assert(u3:get_item("mallorn")==1)
end

local function two_units(r, f1, f2)
  local u1, u2
  u1 = unit.create(f1, r, 1)
  u2 = unit.create(f2, r, 1)
  u1:add_item("money", u1.number * 100)
  u2:add_item("money", u2.number * 100)
  return u1, u2
end

local function two_factions()
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  f1.id = 1
  local f2 = faction.create("noreply@eressea.de", "orc", "de")
  f2.id = 2
  return f1, f2
end

function test_canoe()
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
  assert(u2.region.id==land.id)
  u1:add_order("NACH O O O")
  process_orders()
  assert(u2.region.id==dst.id)
end

function test_control()
  free_game()
  local u1, u2 = two_units(region.create(0, 0, "plain"), two_factions())
  local r = u1.region
  local b = building.create(r, "castle")
  u1.building = b
  u2.building = b
  update_owners()
  assert(b.owner==u1)
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " KOMMANDO")
  u1:add_order("VERLASSE")
  process_orders()
  assert(b.owner==u2)
end

function test_storage()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  f.id = 42
  local u = unit.create(f, r, 1)
  u:add_item("money", u.number * 100)
  store = storage.create("test.unit.dat", "wb")
  assert(store)
  store:write_unit(u)
  store:close()
  free_game()
  -- recreate world:
  r = region.create(0, 0, "plain")
  f = faction.create("noreply@eressea.de", "human", "de")
  f.id = 42
  store = storage.create("test.unit.dat", "rb")
  assert(store)
  u = store:read_unit()
  store:close()
  assert(u)
  assert(u:get_item("money") == u.number * 100)
end

loadscript("extensions.lua")
tests = {
    ["alliance"] = test_alliance,
    ["pure"] = test_pure,
    ["read_write"] = test_read_write,
    ["control"] = test_control,
    ["faction"] = test_faction,
    ["region"] = test_region,
    ["building"] = test_building,
    ["unit"] = test_unit,
    ["message"] = test_message,
    ["hashtable"] = test_hashtable,
    ["gmtool"] = test_gmtool,
    ["leave"] = test_leave,
    ["events"] = test_events,
    ["produce"] = test_produce,
    ["rename"] = test_rename,
    ["recruit"] = test_recruit,
    ["recruit2"] = test_recruit2,
    ["spells"] = test_spells,
    ["herbalism"] = test_herbalism,
    ["storage"] = test_storage,
    ["taxes"] = test_taxes,
    ["mallorn"] = test_mallorn,
    ["upkeep"] = test_upkeep,
    ["id"] = test_id,
    ["work"] = test_work,
    ["morale"] = test_morale,
    ["owners"] = test_owners,
    ["canoe"] = test_canoe,
    ["guard"] = test_guard,
    ["market"] = test_market
}
mytests = {
    ["guard"] = test_guard,
    ["owners"] = test_owners
}
fail = 0
for k, v in pairs(mytests) do
    local status, err = pcall(v)
    if not status then
        fail = fail + 1
        print("[FAIL] " .. k .. ": " .. err)
    else
        print("[OK] " .. k)
    end
end

-- spells_csv()

if fail > 0 then
    print(fail .. " tests failed.")
    io.stdin:read()
end
