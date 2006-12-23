function use_xmastree(u, amount)
  print("NOT IMPLEMENTED: use_xmastree")
  -- u:use_pooled("xmastree", amount)
  return -1
end

function xmas2006()
  if get_gamename() == "Eressea" then
    if not get_flag("xm06") then
      print("Es weihnachtet sehr (2006)")
      set_flag("xm06", true)
      for f in factions() do
        f:add_item("xmastree", 1)
        f:add_notice("santa2006")
      end
    end
  end
end

xmas2006()
