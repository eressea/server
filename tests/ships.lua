require "lunit"

module("tests.ships", package.seeall, lunit.testcase)

function setup()
    eressea.game.reset()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.ships.storms", "0")
    conf = [[{
        "races": {
            "human" : {},
            "insect" : {}
        },
        "ships" : {
            "boat" : {
                "construction" : {
                    "maxsize" : 5
                },
                "coasts" : [ "plain" ],
                "range" : 3
            }
        },
        "buildings" : {
            "harbour" : {}
        },
        "terrains" : {
            "ocean": { "flags" : [ "sea", "sail" ] },
            "plain": { "flags" : [ "land", "walk", "sail" ] },
            "glacier": { "flags" : [ "land", "walk" ] }
        },
        "directions" : {
            "de" : {
                "east" : "OSTEN",
                "west" : "WESTEN"
            }
        },
        "keywords" : {
            "de" : {
                "move" : "NACH"
            }
        },
        "strings" : {
            "de" : {
                "harbour" : "Hafen"
            }
        }
    }]]

    eressea.config.reset()
    assert(eressea.config.parse(conf)==0)
end

function test_sail_oceans()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("test@example.com", "human", "de")
    local u = unit.create(f, r1, 1)
    u.ship = ship.create(r1, "boat")
    u.ship.size = 5
    u:set_skill("sailing", 10)
    u:add_order("NACH O")
    process_orders()
    assert_equal(r2, u.region)
end

function test_sail_to_shore()
  local ocean = region.create(1, 0, "ocean")
  local shore = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, ocean, 1)
  u.ship = ship.create(ocean, "boat")
  u:set_skill("sailing", 10)
  u:add_order("NACH W")
  process_orders()

  assert_equal(shore, u.region)
end

function test_sail_to_forbidden_shore()
  local ocean = region.create(1, 0, "ocean")
  local shore = region.create(0, 0, "glacier")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, ocean, 1)
  u.ship = ship.create(ocean, "boat")
  u:set_skill("sailing", 10)
  u:add_order("NACH W")
  process_orders()

  assert_equal(ocean, u.region)
end

function test_sail_into_harbour()
  local ocean = region.create(1, 0, "ocean")
  local shore = region.create(0, 0, "glacier")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, ocean, 1)
  u.name = "Sailor"
  u.ship = ship.create(ocean, "boat")
  u:set_skill("sailing", 10)
  u:add_order("NACH W")
  local b = building.create(shore, "harbour")
  assert_not_nil(b)
  process_orders()

  assert_equal(shore, u.region)
end
