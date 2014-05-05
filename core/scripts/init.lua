require(config.game .. ".modules")
require "default"
require "resources"

function run_editor()
    local turn = get_turn()
    if turn<0 then
        turn = read_turn()
        set_turn(turn)
    end
    eressea.read_game(turn .. ".dat")
    gmtool.editor()
end
    
function run_tests()
  print("running tests")
  require "lunit"
  lunit.clearstats()
  local argv = tests or {}
  local stats = lunit.main(argv)
  if stats.errors > 0 or stats.failed > 0 then
      assert(stats.errors == 0 and stats.failed == 0)
  end
  return 0
end

function run_turn()
  require(config.game .. ".main")

  local turn = get_turn()
  if turn<0 then
    turn = read_turn()
    set_turn(turn)
  end

  orderfile = orderfile or config.basepath .. '/orders.' .. turn
  print("executing turn " .. get_turn() .. " with " .. orderfile)
  local result = process(orderfile)
  if result==0 then
    dbupdate()
  end
  return result
end
