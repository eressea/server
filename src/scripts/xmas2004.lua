function xmas2004()
  print(get_gamename())
  if get_gamename() == "Eressea" then
    if not get_flag("xm04") then
      print("Es weihnachtet sehr")
      set_flag("xm04", true)
      for f in factions() do
        f:add_item("speedsail", 1)
        f:add_notice("santa2004")
        print(f)
      end
    end
  end
end

print("Ja, ist denn schon Weihnachten?")
xmas2004()
