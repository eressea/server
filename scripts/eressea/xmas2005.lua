function usepotion_message(u, potion)
  msg = message.create("usepotion")
  msg:set_unit("unit", u)
  msg:set_resource("potion", potion)
  return msg
end

function use_stardust(u, amount)
  local p = u.region:get_resource("peasant")
  p = math.ceil(1.5 * p)
  u.region:set_resource("peasant", p)
  local msg = usepotion_message(u, "stardust")
  msg:send_region(u.region)
  u:use_pooled("stardust", amount)
  return 0
end

function xmas2005()
  if get_gamename() == "Eressea" then
    if not get_flag("xm05") then
      print("Es weihnachtet sehr (2005)")
      set_flag("xm05", true)
      for f in factions() do
        f:add_item("stardust", 1)
        f:add_notice("santa2005")
      end
    end
  end
end

-- xmas2005()
