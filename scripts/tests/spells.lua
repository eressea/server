require "lunit"

module("tests.spells", package.seeall, lunit.testcase)

function setup()
    free_game()
end

function test_roi()
    local r = region.create(0,0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.race = "elf"
    u:set_skill("magic", 10)
    u:add_item("money", 3010)
    u.magic = "tybied"
    u.aura = 200
    u.ship = s1
    u:add_spell("create_roi")
    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe einen Ring der Unsichtbarkeit' ")
    process_orders()
    write_reports()
    assert_equal(1, u:get_item("roi"))
end

function test_blessedharvest_lasts_n_turn()
  free_game()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "halfling", "de")
  local u = unit.create(f, r)
  r:set_resource("peasant", 100)
  r:set_resource("money", 0)
  u:add_item("money", 1000)
  u.magic = "gwyrrd"
  u.race = "dwarf"
  u:set_skill("magic", 20)
  u.aura = 200
  u:add_spell("raindance")
  u:add_spell("blessedharvest")
  u:clear_orders()
  local level = 5
  u:add_order("ZAUBERE STUFE " .. level .. " Regentanz")
  assert_equal(0, r:get_resource("money"), 0)

  local m = 0
  local p = 100
  for i=1,level+2 do
    process_orders()
    local income = p * 12
    p = r:get_resource("peasant")
    income = income - p * 10
    m = m + income
    -- print(i, m, p, r:get_resource("money"))
    if (i>level+1) then
      assert_not_equal(m, r:get_resource("money"))
    else
      assert_equal(m, r:get_resource("money"))
    end
    u:clear_orders()
    u:add_order("ARBEITEN")
--    u:add_spell("raindance")
  end
end
