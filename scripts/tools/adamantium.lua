-- adamant gifts and setup for tunnels

-- use only once to hand out some items to existing factions
function adamant_gifts()
  for f in factions() do
    local i = math.fmod(test.rng_int(), 2)
    if i==0 then
      f:add_item("adamantium", 1)
      f:add_item("adamantiumplate", 1)
    else
      f:add_item("adamantium", 3)
      f:add_item("adamantiumaxe", 1)
    end
  end
end

function adamant_seeds()
  for r in regions() do
    if r:get_key("tnnL") then
      print("1 ", r:get_resource("adamantium"), r)
      test.adamantium_island(r)
      print("2 ", r:get_resource("adamantium"))
    end
  end
end
