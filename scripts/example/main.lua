-- Enno was here
require "multis"

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

  if nmr_check(config.maxnmrs or 80)~=0 then
    return -1
  end

  process_orders()

  -- create new monsters:
  spawn_braineaters(0.25)

  -- post-turn updates:
  update_guards()
  update_scores()

  local localechange = { de = { "ii" } }
  change_locales(localechange)

  write_files(config.locales)

  file = "" .. get_turn() .. ".dat"
  if write_game(file, "binary")~=0 then
    print("could not write game")
    return -1
  end
  return 0
end
