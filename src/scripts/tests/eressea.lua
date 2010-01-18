require "lunit"

module( "e2", package.seeall, lunit.testcase )

function setup()
    free_game()
end

function test_unit_limit_is_1500()
  free_game()
  local r = region.create(0,0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  for i = 1,1500 do
    unit.create(f, r, 1)
  end
  local u = unit.create(f, r, 0)
  u:add_item("money", 20000)
  u:clear_orders()
  u:add_order("REKRUTIEREN 1")
  process_orders()
  assert_equal(1, u.number)
end
