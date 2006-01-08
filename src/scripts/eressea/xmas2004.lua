function use_snomwan(u, amount)
  if u.region.terrain == "glacier" then
    local man = add_unit(u.faction, u.region)
    u.race = "snowman"
    u.number = amount
    u:add_item("snowman", -amount)
    return 0
  end
  return -4
end

function xmas2004()
  print(get_gamename())
  if get_gamename() == "Eressea" then
    if not get_flag("xm04") then
      print("Es weihnachtet sehr (2004)")
      set_flag("xm04", true)
      for f in factions() do
        f:add_item("speedsail", 1)
        f:add_notice("santa2004")
      end
    end
  end
end

xmas2004()
