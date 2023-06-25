local tcname = 'tests.e2.harbor'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("rules.ship.damage.nocrewocean", "0")
    eressea.config.parse([[
{
    "keywords": {
        "de" : {
            "move" : "NACH",
            "help" : "HELFEN",
            "contact" : "KONTAKTIEREN"
         }
    },
    "directions" : {
        "de" : {
            "east" : "OSTEN",
            "west" : "WESTEN"
        }
    },
    "parameters" : {
        "de" : {
            "EINHEIT": "EINHEIT",
            "PARTEI": "PARTEI"
        }
    },
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
    "buildings" : {
        "harbour" : {
            "maintenance": [
                { "amount" : 500, "type": "money" }
            ],
            "maxcapacity" : 25,
            "maxsize" : 25
        }
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

function test_landing_harbour_with_help()
    local ocean = region.create(1, 0, "ocean")
    local r = region.create(0, 0, "glacier")
    local harbour = building.create(r, "harbour", 25)
    local f = faction.create("human")
    local f2 = faction.create("human")
    local s = ship.create(ocean, "longboat")
    local u1 = unit.create(f, ocean, 1)
    local u2 = unit.create(f2, r, 1)
    assert_not_nil(u2)
    u1:add_item("money", 1000)
    u2:add_item("money", 1000)
    u2.building = harbour
    u2:clear_orders()
    u2:add_order("KONTAKTIERE EINHEIT " .. itoa36(u1.id))
  
    u1.ship = s
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH w")

    process_orders()
    assert_equal(0, s.damage)
    assert_equal(r, u1.region) -- glacier with harbour and help-- okay
end

function test_landing_harbour_without_help()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "glacier")
  local harbour = building.create(r, "harbour", 25)
  local f = faction.create("human")
  local f2 = faction.create("human")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f2, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 1000)
  u2.building = harbour
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(ocean, u1.region) -- glacier with harbour and no help-- cannot land
  assert_equal(0, s.damage)
end

function test_landing_harbour_unpaid()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "glacier")
  local harbour = building.create(r, "harbour", 25)
  local f = faction.create("human")
  local s = ship.create(ocean, "longboat")
  local u1 = unit.create(f, ocean, 1)
  local u2 = unit.create(f, r, 1)
  assert_not_nil(u2)
  u1:add_item("money", 1000)
  u2:add_item("money", 20)
  
  u1.ship = s
  u1:set_skill("sailing", 10)
  u1:clear_orders()
  u1:add_order("NACH w")
  process_orders()
  
  assert_equal(ocean, u1.region) -- did not pay 
  assert_equal(0, s.damage)
end

function test_no_damage_with_harbor()
    local r = region.create(0, 0, "glacier")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create('human')
    local u = unit.create(f, r2, 1)
    u:set_skill("sailing", 10)
    u.ship = ship.create(r2, "longboat")
    u:add_order('NACH W')

    -- no owner, not maintained:
    local b = building.create(r, 'harbour', 25)
    process_orders()
    assert_false(b.working)
    assert_equal(r2, u.region)
    assert_equal(0, u.ship.damage)
end

function test_sail_into_harbor()
    local r = region.create(0, 0, "glacier")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create('human')
    local u = unit.create(f, r2, 1)
    u:set_skill("sailing", 10)
    u.ship = ship.create(r2, "longboat")
    u:add_order('NACH W')

    local b = building.create(r, 'harbour', 25)
    local u2 = unit.create(f, r, 1)
    u2.building = b
    u2:add_item("money", 10000)

    process_orders()
    assert_true(b.working)
    assert_equal(0, u.ship.damage)
    assert_equal(r, u.region)
end

function test_landing_insects()
    local ocean = region.create(1, 0, "ocean")
    local r = region.create(0, 0, "glacier")
    local harbour = building.create(r, "harbour", 25)
    local f = faction.create("insect")
    local f2 = faction.create("human")
    local s = ship.create(ocean, "longboat")
    local u1 = unit.create(f, ocean, 1)
    local u2 = unit.create(f2, r, 1)
    assert_not_nil(u2)
    u1:add_item("money", 1000)
    u2:add_item("money", 1000)
    u2.building = harbour
  
    u1.ship = s
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH w")
    process_orders()
  
    assert_equal(ocean, u1.region) -- even with a harbor, insects cannot land in glaciers
    assert_equal(0, s.damage)
end
