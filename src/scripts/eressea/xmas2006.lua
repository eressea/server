function use_xmastree(u, amount)
  u.region:set_key("xm06", true)
  u:use_pooled("xmastree", amount)
  msg = message("usepotion")
  msg:set_unit("unit", u)
  msg:set_resource("potion", "xmastree")
  msg:send_region(u.region)
  return 0
end

function update_xmas2006()
  if get_season == "winter" then
    msg = message("xmastree_effect")
    for r in regions() do
      if r:get_key("xm06") then
        trees = r:get_resource("tree")
        if trees>0 then
          r:set_resource("tree", trees * 1.1)
          msg:send_region(r)
        end
      end
    end
  end
end

function xmas2006()
  if get_gamename() == "Eressea" then
    if not get_key("xm06") then
      print("Es weihnachtet sehr (2006)")
      set_key("xm06", true)
      for f in factions() do
        f:add_item("xmastree", 1)
        f:add_notice("santa2006")
      end
    end
  end
end

xmas2006()
