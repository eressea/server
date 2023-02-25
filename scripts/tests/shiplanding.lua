local tcname = 'tests.shiplanding'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.config.parse([[
{
    "terrains": {
        "ocean" : {
            "flags": [ "sea", "sail" ]
        },
        "plain" : {
            "flags": [ "land", "walk", "sail" ]
        },
        "glacier" : {
            "flags": [ "arctic", "land", "walk", "sail" ]
        }
    },
    "races": {
        "human": {},
        "insect": {}
    },
    "ships": {
        "boat": {
            "coasts" : [ "plain", "glacier" ],
            "range" : 2,
            "construction" : {
                "maxsize" : 5
            },
            "cargo" : 5000,
            "minskill" : 1,
            "captain" : 1,
            "skills" : 2
        },
        "longboat": {
            "coasts" : [ "plain" ],
            "range" : 2,
            "construction" : {
                "maxsize" : 50
            },
            "cargo" : 50000,
            "minskill" : 1,
            "captain" : 1,
            "skills" : 10
        }
    }}
]])
end

function test_ship_coasts()
    local r = region.create(0, 0, "glacier")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create('human')
    local u = unit.create(f, r2, 1)
    u:set_skill("sailing", 10)
    u.ship = ship.create(r2, "boat")
    u:add_order('NACH W')

    process_orders()
    assert_equal(r, u.region)
    assert_equal(0, u.ship.damage)
    assert_equal(directions.EAST, u.ship.coast)
end

function test_crash_on_shore_damage()
    local r = region.create(0, 0, "glacier")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create('human')
    local u = unit.create(f, r2, 1)
    u:set_skill("sailing", 10)
    u.ship = ship.create(r2, "longboat")
    u:add_order('NACH W')

    u.ship.name = 'hodor'
    process_orders()
    assert_equal(r2, u.region)
    assert_equal(500, u.ship.damage)
end

function test_landing1()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "plain")
  local f = faction.create("human")
  local f2 = faction.create("human")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f2, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 1000)
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(r, u1.region) -- the plain case: okay
end

function test_landing_terrain()
    local ocean = region.create(1, 0, "ocean")
    local r = region.create(0, 0, "glacier")
    local f = faction.create("human")
    local f2 = faction.create("human")
    local s = ship.create(ocean, "longboat")
    local u1 = unit.create(f, ocean, 1)
    local u2 = unit.create(f2, r, 1)
    assert_not_nil(u2)
    u1:add_item("money", 1000)
    u2:add_item("money", 1000)

    u1.ship = s
    u1:set_skill("sailing", 10)    u1:clear_orders()
    u1:add_order("NACH w")
    process_orders()

    assert_equal(ocean, u1.region) -- cannot land in glacier without harbour
end

function test_landing_insects()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "glacier")
  local f = faction.create("insect")
  local f2 = faction.create("human")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f2, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 1000)
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(ocean, u1.region) -- insects cannot land in glaciers
end
