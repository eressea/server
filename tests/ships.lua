require "lunit"

module("tests.ships", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    conf = [[{
        "races": {
            "human" : {},
            "insect" : {}
        },
        "ships" : {
            "boat" : {},
            "longboat" : {}
        },
        "buildings" : {
            "harbour" : {}
        },
        "terrains" : {
            "ocean": {},
            "plain": {}
        }
    }]]

    eressea.config.parse(conf)
end

function test_landing1()
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


