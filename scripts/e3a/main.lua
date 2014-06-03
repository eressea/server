require "multis"
require "e3a.frost"

function process(orders)
  local confirmed_multis = { }
  local suspected_multis = { }
  
  if open_game(get_turn())~=0 then
    print("could not read game")
    return -1
  end
  init_summary()

  -- kill multi-players (external script)
  kill_multis(confirmed_multis, false)
  mark_multis(suspected_multis, false)

  -- run the turn:
  if read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
  end

  -- plan_monsters()
  local mon = get_faction(666)
  if mon ~= nil then
    mon.lastturn = get_turn()
  end

  if nmr_check(config.maxnmrs or 30)~=0 then
    return -1
  end

  process_orders()
  if xmas2009~=nil then
    xmas2009()
  end

  -- create new monsters:
  spawn_dragons()
  spawn_undead()
  -- spawn_braineaters(0.25)
  -- spawn_ents()

  -- post-turn updates:
  update_guards()
  update_scores()
  frost.update()

  local localechange = { en = { "L46o" } }
  change_locales(localechange)

  -- use newfactions file to place out new players
  -- autoseed(config.basepath .. "/newfactions", false)

  write_files(config.locales)

  file = "" .. get_turn() .. ".dat"
  if eressea.write_game(file)~=0 then
    print("could not write game")
    return -1
  end
  return 0
end
