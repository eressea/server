function use_seashell(u, amount)
-- Muschelplateau...
  local r = get_region(165,30)
  local visit = u.faction.objects:get("embassy_muschel")
  if visit~=nil and u.region~= r then
    local turns = get_turn() - visit
    local msg = message("msg_event")
    msg:set_string("string", u.name .. "(" .. itoa36(u.id) .. ") erzählt den Bewohnern von " .. u.region.name .. " von Muschelplateau, das die Partei " .. u.faction.name .. " vor " .. turns .. " Wochen besucht hat." )
    msg:send_region(u.region)
    return 0
  end
  return -4
end

function update_embassies()
-- Muschelplateau
  local r = get_region(165,30)
  if r~=nil then
    local u
    for u in r.units do
      if u.faction.objects:get("embassy_muschel")==nil then
        if (u.faction:add_item("seashell", 1)>0) then
          print(u.faction)
          u.faction.objects:set("embassy_muschel", get_turn())
        end
      end
    end
  end
end

