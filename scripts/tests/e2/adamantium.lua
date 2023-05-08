local tcname = 'tests.e2.adamantium'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
end

local function _test_create_adamantium()
    eressea.settings.set("rules.terraform.all", "1")
    local r = region.create(0,0, "mountain")
    local f1 = faction.create('human')
    local u1 = unit.create(f1, r, 1)
    r:set_resource("adamantium", 50)
    assert_equal(50, r:get_resource("adamantium"))
    return r, u1
end

function test_adamantium1()
  local r, u1 = _test_create_adamantium()
  
  u1:add_item("money", 1000)
  u1:set_skill("mining", 14)
  u1:clear_orders()
  u1:add_order("MACHEN Adamantium")
 
  process_orders()
  assert_equal(0, u1:get_item("adamantium"))
end

function test_adamantium2()
  local r, u1 = _test_create_adamantium()
  
  u1:add_item("money", 1000)
  u1:set_skill("mining", 15)
  u1:clear_orders()
  u1:add_order("MACHEN Adamantium")

  local b = building.create(r, "mine")
  b.size = 10
  u1.building = b
  local adamantium = r:get_resource("adamantium")

  process_orders()
  assert_equal(2, u1:get_item("adamantium"))
  assert_equal(adamantium - 2, r:get_resource("adamantium"))
end

