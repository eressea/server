function usepotion_message(u, type)
  msg = message("usepotion")
  msg:set_unit("unit", u)
  msg:set_resource("potion", type)
  return msg
end

function use_stardust(u, amount)
  local p = u.region:get_resource("peasant")
  p = math.ceil(1.5 * p)
  u.region:set_resource("peasant", p)
  local msg = usepotion_message(u, "stardust")
  msg:send_region(u.region)
end

function xmas2005()
  print(get_gamename())
  if get_gamename() == "Eressea" then
    if not get_flag("xm05") then
      print("Es weihnachtet sehr")
      set_flag("xm05", true)
      for f in factions() do
        f:add_item("stardust", 1)
        f:add_notice("santa2005")
      end
    end
  end
end

print("Ja, ist denn schon wieder Weihnachten?")
xmas2005()
