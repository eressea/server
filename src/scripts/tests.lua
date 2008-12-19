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

function test_unit()
	free_game()
	local r = region.create(0, 0, "plain")
	local f = faction.create("enno@eressea.de", "human", "de")
	local u = unit.create(f, r)
	u.number = 20
	u.name = "Enno"
	assert(u.name=="Enno")
	u:add_item("sword", 4)
	assert(u:get_item("sword")==4)
	assert(u:get_pooled("sword")==4)
	u:use_pooled("sword", 2)
	assert(u:get_item("sword")==2)
end

function test_region()
	free_game()
	local r = region.create(0, 0, "plain")
	r:set_resource("horse", 42)
	r:set_resource("money", 45)
	r:set_resource("peasant", 200)
	assert(r:get_resource("horse") == 42)
	assert(r:get_resource("money") == 45)
	assert(r:get_resource("peasant") == 200)
end

function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

function test_message()
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

loadscript("extensions.lua")
test_read_write()
test_region()
test_unit()
test_message()
