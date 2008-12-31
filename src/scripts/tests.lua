local function test_pure()
    free_game()
    local r = region.create(0, 0, "plain")
end

local function test_read_write()
    free_game()
    local r = region.create(0, 0, "plain")
    local f = faction.create("enno@eressea.de", "human", "de")
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
    local f = faction.create("enno@eressea.de", "human", "de")
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
    local f = faction.create("enno@eressea.de", "human", "de")
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
  local f = faction.create("enno@eressea.de", "human", "de")
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
  local f = faction.create("enno@eressea.de", "human", "de")
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
  local f = faction.create("enno@eressea.de", "human", "de")
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

  f = faction.create("enno@eressea.de", "orc", "de")
  f.age = 20

  u = unit.create(f, plain)
  u.number = 1
  u:add_item("money", u.number*100)
  u:clear_orders()
  u:add_order("NUMMER PARTEI test")
  u:add_handler("message", msg_handler)
  msg = "BOTSCHAFT EINHEIT " .. itoa36(u.id) .. " Du~Elf~stinken"
  f = faction.create("enno@eressea.de", "elf", "de")
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

local function test_recruit()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("enno@eressea.de", "human", "de")
  local u = unit.create(f, r)
  u.number = 1
  local n = 3
  r:set_resource("peasant", 200)
  u:clear_orders()
  u:add_item("money", 85*n+20)
  u:add_order("REKRUTIERE " .. n)
  process_orders()
  assert(u.number == n+1)
  local p = r:get_resource("peasant")
  assert(p<200 and p>=200-n)
  assert(u:get_item("money")==10)
end

loadscript("extensions.lua")
tests = {
    ["pure"] = test_pure,
    ["read_write"] = test_read_write,
    ["faction"] = test_faction,
    ["region"] = test_region,
    ["building"] = test_building,
    ["unit"] = test_unit,
    ["message"] = test_message,
    ["hashtable"] = test_hashtable,
    ["gmtool"] = test_gmtool,
    ["events"] = test_events,
    ["recruit"] = test_recruit
}

fail = 0
for k, v in pairs(tests) do
    local status, err = pcall(v)
    if not status then
        fail = fail + 1
        print("[FAIL] " .. k .. ": " .. err)
    else
        print("[OK] " .. k)
    end
end

if fail > 0 then
    print(fail .. " tests failed.")
    io.stdin:read()
end
