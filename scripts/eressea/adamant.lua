-- adamant gifts and setup for tunnels

-- use only once to hand out some items to existing factions
function adamant_gifts()
  for f in factions() do
    local i = math.mod(test.rng_int(), 2)
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

-- create a fixed path to a specific region
local function create_path(from, to)
  local param = tostring(to.uid)
  local b = add_building(from, "portal")
  b.name = "Weltentor"
  b.size = 1
  b:add_action("tunnel_action", param)
end

-- create a wonky tunnel wth more than one exit
local function create_tunnel(from, param)
  local b = add_building(from, "portal")
  b.name = "Weltentor"
  b.size = 1
  b:add_action("tunnel_action", param)
end

-- make a tunnel from the cursor to the first selected region
function mktunnel()
  local from = gmtool.get_cursor()
  local to = gmtool.get_selection()()
  if to~=nil then
    region.create(from.x, from.y, "glacier")
    create_tunnel(from, to)
    gmtool.select(to, 0)
    gmtool.highlight(to, 1)
  end
end

-- turn all selected regions into targets for a wonky tunnel ("tnnL")
function mkanchors()
  for r in gmtool.get_selection() do
    if not r:get_key("tnnL") then
      r:set_key("tnnL", true)
      if r:get_flag(0) then
        -- RF_CHAOTIC
        r:set_flag(0, true)
      end
      r:set_resource("peasant", r:get_resource("peasant") + 1)
    end
  end
end

-- region.create and prepare all hell-regions to become wonky gates
function mkgates()
  for r in regions() do
    if r.plane_id==0 and r.terrain=="hell" then
      create_tunnel(r, "tnnL")
      region.create(r.x, r.y, "glacier")
    end
  end
end
