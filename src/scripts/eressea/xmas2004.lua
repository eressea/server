function use_snowman(u, amount)
  if u.region.terrain == "glacier" then
    local man = add_unit(u.faction, u.region)
    man.race = "snowman"
    man.number = amount
    man:add_item("snowman", -amount)
    return 0
  end
  return -4
end

function xmas2004()
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
