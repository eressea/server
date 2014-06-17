require "lunit"

module("tests.ships", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
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
                "range" : 3
            }
        },
        "buildings" : {
            "harbour" : {}
        },
        "terrains" : {
            "ocean": { "flags" : [ "sea", "sail", "fly" ] },
            "plain": { "flags" : [ "land", "walk", "sail", "fly" ] }
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
        }
    }]]

    eressea.config.reset()
    eressea.config.parse(conf)
    eressea.locale.create("en")
end

function test_sail()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("test@example.com", "human", "de")
    local u = unit.create(f, r1, 1)
    u.ship = ship.create(r1, "boat")
    u.ship.size = 5
    u:set_skill("sailing", 10)
    u:add_order("NACH O")
    process_orders()
--    eressea.process.movement()
    assert_equal(r2, u.region)
end

function notest_landing1()
  local ocean = region.create(1, 0, "ocean")
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "insect", "de")
  local f2 = faction.create("noreply@eressea.de", "human", "de")
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


