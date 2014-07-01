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

function test_castle_names()
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("test@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  u1:add_item("money", 10000)

  local b = building.create(r, "castle")
  u1.building = b

  b.owner = u1
  assert_equal("site", b:get_typename(1))
  assert_equal("tradepost", b:get_typename(2))
  assert_equal("tradepost", b:get_typename(9))
  assert_equal("fortification", b:get_typename(10))
end
