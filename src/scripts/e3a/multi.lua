function multi(f)
  f.password = "doppelspieler"
  f.email = "doppelspieler@eressea.de"
  f.banner = "Diese Partei steht wegen vermuteten Doppelspiels unter Beobachtung."
  for u in f.units do
    u.race_name = "toad"
    if u.building~=nil then
      local found = False
      for u2 in u.region.units do
        if u2.faction~=u.faction then
          found = true
          break
        end
      end
      if not found then
        u.region.terrain_name = "firewall"
        u.region:set_flag(2) -- RF_BLOCKED
      end
    end
  end
end
