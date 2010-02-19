function process(orders)
  -- initialize starting equipment for new players
  if open_game==nil then
    print("did you load default.lua?")
    return -1
  end

  if open_game(get_turn())~=0 then
    print("could not read game")
    return -1
  end
  init_summary()

  -- run the turn:
  if read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
  end

  plan_monsters()

  local nmrs = get_nmrs(1)
  --  nmrs = 0
  maxnmrs = maxnmrs or 80
  if nmrs >= maxnmrs then
    print("Shit. More than " .. maxnmrs .. " factions with 1 NMR (" .. nmrs .. ")")
    write_summary()
    return -1
  end
  print (nmrs .. " Factions with 1 NMR")

  process_orders()

  -- create new monsters:
  spawn_dragons()
  spawn_undead()
  spawn_braineaters(0.25)
  spawn_ents()

  -- post-turn updates:
  update_xmas2006()
  update_embassies()
  update_guards()
  update_scores()

  change_locales()

  -- use newfactions file to place out new players
  autoseed(basepath .. "/newfactions", false)

  write_files(locales)

  file = "" .. get_turn() .. ".dat"
  if write_game(file, "binary")~=0 then
    print("could not write game")
    return -1
  end
end
