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
