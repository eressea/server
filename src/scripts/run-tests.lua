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

function test_hse()
	read_game("50", "text")
	write_game("50.dat", "binary")
	write_game("50.txt.1", "text")
end

function test_realloc()
  local t1 = os.clock()
  read_game("571.dat.2", "binary")
  print(os.clock() - t1)
  free_game()
  print(os.clock() - t1)
  -- and again
  local t2 = os.clock()
  read_game("571.dat.2", "binary")
  print(os.clock() - t2)
  free_game()
  print(os.clock() - t2)
end

function test_bmark()
  local t1 = os.clock()
  read_game("566.dat", "binary")
  print(os.clock() - t1)
end

loadscript("default.lua")
run_scripts()
-- go
local now = os.clock()
-- test_free()
test_bmark()
-- test_hse()
-- test_realloc()
local elapsed = os.clock() - now
print(elapsed)
-- text: 50.574
-- bin0: 19.547
-- bin1: 18.953
-- io.stdin:read("*line")
