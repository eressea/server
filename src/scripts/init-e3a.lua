require "e3a.rules"
require "e3a.multi"
require "default"
require "spells"
require "extensions"

function run_editor()
    local turn = get_turn()
    if turn==0 then
        turn = read_turn()
        set_turn(turn)
    end
    read_game(turn .. ".dat")
    gmtool.editor()
end
    
function run_tests()
  print("running tests")
  require "lunit"
  lunit.clearstats()
  local argv = tests or {}
  local stats = lunit.main(argv)
  if stats.errors > 0 or stats.failed > 0 then
    return 1
  end
  return 0
end

function run_turn()
  require "run-e3a"

  -- the locales that this gameworld supports.
  local locales = { "de", "en" }
  local confirmed_multis = { 
  }
  local suspected_multis = { 
  "odin"
  }
  
  local turn = get_turn()
  if turn==0 then
    turn = read_turn()
    set_turn(turn)
  end

  orderfile = orderfile or basepath .. '/orders.' .. turn
  print("executing turn " .. get_turn() .. " with " .. orderfile)
  local result = process(orderfile, confirmed_multis, suspected_multis, locales)
  if result==0 then
    dbupdate()
  end
  return result
end
