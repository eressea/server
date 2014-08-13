-- create a fixed path to a specific region
local function create_path(from, to)
  local param = tostring(to.uid)
  local b = building.create(from, "portal")
  b.name = "Weltentor"
  b.size = 1
  b:add_action("tunnel_action", param)
end

-- create a wonky tunnel wth more than one exit
local function create_tunnel(from, param)
  local b = building.create(from, "portal")
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
        -- RF_CHAOTIC gets removed
        r:set_flag(0, false)
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
