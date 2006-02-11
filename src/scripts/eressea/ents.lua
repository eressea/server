
local function create_ents(r, number)
  local f = get_faction(0)
  if f~=nil and number>0 then
    u = add_unit(f, r)
    u.number = number
    u.name = "Wütende Ents"
    u:set_skill("perception", 2)
    
    msg = message("entrise")
    msg:set_region("region", r)
    msg:send_region(r)
    return u
  end
  return nil
end

-- this is old stuff. it's not active at the moment.
function spawn_ents()
  local r
  for r in regions() do
    if r:get_flag(0) then -- RF_CHAOTIC
      if r.terrain == "plain" and r:get_resource("tree")==0 then
        if math.random(3)==1 then
          u = create_ents(r, math.random(30))
          if u ~= nil then
            r:set_resource("tree", u.number)
          end
        end
      end
    end
  end
end
