local tcname = 'tests.shared.recruit'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.peasants.growth", "0")
end

function test_bug_1795_limit()
  local r = region.create(0, 0, "plain")    
  local f = faction.create('human')
  local u1 = unit.create(f, r, 1)
  u1:add_item("money", 100000000)
  u1:add_order("REKRUTIEREN 9999")
  r:set_resource("peasant", 2000) -- no fractional growth!
  local peasants = r:get_resource("peasant")
  local limit,frac = math.modf(peasants/40) -- one day this should be a parameter
  
  process_orders()
  assert_equal(limit+1, u1.number, u1.number .. "!=" .. (limit+1))
  local np = r:get_resource("peasant")
  assert_equal(peasants-limit, np)
end

function test_bug_1795_demons()
  local r = region.create(0, 0, "plain")    
  local f = faction.create('demon')
  local u1 = unit.create(f, r, 1)
  r:set_resource("peasant", 2000)
  local peasants = r:get_resource("peasant")
  local limit,frac = math.modf(peasants/40) 

  u1:add_item("money", 100000000)
  u1:add_order("REKRUTIEREN 9999")
  
  process_orders()
  
  assert_equal(limit+1, u1.number, u1.number .. "!=" .. (limit+1))
  assert_equal(peasants, r:get_resource("peasant"))
end
