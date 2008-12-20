-- -*- coding: utf-8 -*-

function test_locales()
	local skills = { "", "herb", "kraut", "Kräute", "Kraeut", "k", "kra", "MAGIE" }
	for k,v in pairs(skills) do
		str = test.loc_skill("de", v)
		io.stdout:write(v, "\t", tostring(str), "  ", tostring(get_string("de", "skill::" .. tostring(str))), "\n")
	end
	return 0
end

function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

function run_scripts()
  scripts = { 
    "spells.lua",
    "extensions.lua",
    "familiars.lua",
  }
  for index, value in pairs(scripts) do
    loadscript(value)
  end
end
--test_locales()

function run_turn()
	plan_monsters()
	process_orders()
	spawn_dragons()
	spawn_undead()
	spawn_braineaters(0.25)
	autoseed(basepath .. "/newfactions", false)
end


function test_free()
	read_game("571.dat", "binary")
	read_orders("orders.571")
	run_turn()
	free_game()
	read_game("570.dat", "binary")
	read_orders("orders.570")
	run_turn()
	free_game()
end

function test_reorder()
  r = terraform(0, 0, "plain")
  f = faction.create("enno@ix.de", "orc", "de")
  s1 = add_ship(r, "boat")
  s1.size = 1
  s2 = add_ship(r, "boat")
  s2.size = 2
  s3 = add_ship(r, "boat")
  s3.size = 3
  b1 = add_building(r, "portal")
  b1.size = 1
  b2 = add_building(r, "portal")
  b2.size = 2
  b3 = add_building(r, "portal")
  b3.size = 3
  u = add_unit(f, r)
  u.number = 1
  u.name = "a"
  u = add_unit(f, r)
  u.number = 1
  u.name = "b"
  u.ship = s3
  u = add_unit(f, r)
  u.number = 1
  u.name = "c"
  u.building = b1
  u = add_unit(f, r)
  u.number = 1
  u.name = "d"
  u.ship = s2
  u = add_unit(f, r)
  u.name = "e"
  u.number = 1
  u.building = b1
  u = add_unit(f, r)
  u.name = "f"
  u.number = 1
  u.building = b3
  u = add_unit(f, r)
  u.name = "g"
  u.number = 1
  u.ship = s2
  u = add_unit(f, r)
  u.name = "h"
  u.number = 1
  u.building = b2
  u = add_unit(f, r)
  u.name = "i"
  u.number = 1
  u = add_unit(f, r)
  u.name = "j"
  u.number = 1
  u.building = b1
  u = add_unit(f, r)
  u.name = "k"
  u.number = 1
  u.ship = s1
  test.reorder_units(r)
  for u in r.units do
    print(u, u.building, u.ship)
  end
  io.stdin:read("*line")
end

function test_hse()
  read_game("50.dat", "binary")
  f = get_faction(atoi36("8h7f"))
  f.options = f.options + 8192
  init_reports()
  write_report(f)
end

function test_xml()
  read_game("572.dat", "binary")
  init_reports()

  f = get_faction(atoi36("ioen"))
  f.options = f.options + 8192
  write_report(f)

  f = get_faction(atoi36("a"))
  f.options = f.options + 8192
  write_report(f)

  f = get_faction(atoi36("777"))
  f.options = f.options + 8192
  write_report(f)
end

function test_realloc()
  local t1 = os.clock()
  read_game("571.4.dat", "binary")
  print(os.clock() - t1)
  free_game()
  print(os.clock() - t1)
  -- and again
  local t2 = os.clock()
  read_game("571.4.dat", "binary")
  print(os.clock() - t2)
  free_game()
  print(os.clock() - t2)
end

function test_bmark()
  local t1 = os.clock()
  read_game("566.dat", "binary")
  print(os.clock() - t1)
end

function test_md5()
  read_game("571.dat", "binary")
  -- read_orders("orders.571")
  run_turn()
  local i = test.rng_int()
  print(i)
  write_game("572.txt." .. i, "text")
  -- 648583167
  io.stdin:read("*line")
end

function test_287()
  read_game("287", "text")
  write_game("287.dat", "binary")
end

function tunnel_action(b, param)
  local r = nil
  print("Tunnel from " .. tostring(b) .. " [" .. param .. "]")

  if tonumber(param)~=nil then
    r = get_region_by_id(tonumber(param))
  end
  if r~=nil then
    local units = tunnel_travelers(b)
    for key, u in pairs(units) do
      local rto = r
      if r==nil then
        rto = get_target(param)
      end
      if rto~=nil then
        u.region = rto
        print(" - teleported " .. tostring(u) .. " to " .. tostring(rto))
      end
    end
  end
  return 1 -- return 0 to destroy
end

function action(b, param)
  print(b)
  print(param)
  return 1
end

function test_tunnels()
  r = terraform(0, 0, "glacier")
  b = add_building(r, "portal")
  b:add_action("tunnel_action", "tnnL")
  r2 = terraform(5, 5, "plain")
  r2:set_key("tnnL", true)
  process_orders()
end

loadscript("default.lua")
run_scripts()
-- go
-- test_free()
-- test_bmark()
-- test_realloc()
test_xml()
-- test_hse()
-- test_reorder()
-- test_tunnels()
-- test_md5()
-- test_287()
-- io.stdin:read("*line")
-- text: 50.574
-- bin0: 19.547
-- bin1: 18.953
-- bin1: 18.313
-- bin2: 17.938
