require "lunit"

module("tests.castles", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.config.reset()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.ships.storms", "0")
    conf = [[{
        "races": {
            "human" : {},
            "halfling" : {}
        },
         "buildings" : {
            "castle" : {
                "name" : "castle_name",
                "construction" : [
                    { "maxsize" : 2 },
                    { "maxsize" : 10 },
                    { "maxsize" : 50 },
                    { "maxsize" : 250 }
                ]
            }
        },
        "terrains" : {
            "plain": { "flags" : [ "land" ] }
        },
        "keywords" : {
            "de" : {
                "make" : "MACHEN"
            }
        }
    }]]
    assert(eressea.config.parse(conf)==0)
end

function test_small_castles()
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("test@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  local f2 = faction.create("test@eressea.de", "halfling", "de")
  local u2 = unit.create(f2, r, 1)
  u1:add_item("money", 10000)

  local b = building.create(r, "castle")
  u2.building = b
  u1.building = b

  b.owner = u2
  assert_equal("site", b:get_typename(7))
  assert_equal("fortification", b:get_typename(8))
  b.owner = u1
  assert_equal("site", b:get_typename(9))
  assert_equal("fortification", b:get_typename(10))
end
