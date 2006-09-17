function update_phoenix()
  local f
  for f in factions() do
    local u
    for u in f.units do
      if u.race=="phoenix" then
        print("The phoenix is in " .. u.region.name() .. "(" .. u.region.x .. "," .. u.region.y .. ")")
        return
      end
    end
  end
  print("The phoenix has not been found and needs to be regenerated")
  f = get_faction(0)
  if (f~=nil) then
    local r
    local nregions

    nregions = 0
    for r in regions() do
      nregions = nregions+1
    end
    local rno = math.random(nregions)
    for r in regions() do
      rno = rno - 1
      if rno <=1 then
        if r.plane_id==0 and r.name~=nil then
          local u = add_unit(f, r)
          u.race = "phoenix"
          u.name = "Der Phönix"
          u.number = 1
          -- TODO: generate an appropriate region message
          print("The phoenix has been generated in " .. u.region.name .. "(" .. u.region.x .. "," .. u.region.y .. ")")
          break
        end
      end
    end
  end
end
