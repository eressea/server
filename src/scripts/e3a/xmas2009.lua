function xmas2009()
  if not get_key("xm09") then
    print("Es weihnachtet sehr (2009)")
    set_key("xm09", true)
    for f in factions() do
      f:add_item("xmastree", 1)
      f:add_notice("santa2006")
    end
  end
end

function use_xmastree(u, amount)
  if u.region.herb~=nil then
    local trees = u.region:get_resource("tree")
    u.region:set_resource("tree", 10+trees)
    u:use_pooled("xmastree", amount)
    local msg = message.create("usepotion")
    msg:set_unit("unit", u)
    msg:set_resource("potion", "xmastree")
    msg:send_region(u.region)
    return 0
  end
end
