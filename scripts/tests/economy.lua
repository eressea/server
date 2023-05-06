local tcname = 'tests.shared.economy'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    conf = [[{
        "races": {
            "human" : {
                "speed" : 1,
                "weight" : 1000,
                "capacity" : 1540,
                "player" : true
            },
            "ent": {
                "unarmedguard" : true
            },
            "lynx": {
            }
        },
        "buildings" : {
            "sawmill": {}
        },
        "ships" : {
            "caravel": {
                "construction" : {
                    "maxsize" : 250,
                    "minskill": 6,
                    "materials" : {
                        "log" : 1
                    }
                }
            }
        },
        "items" : {
            "money": {
                "weight" : 1
            },
            "catapult" : {
                "weight" : 500,
                "weapon" : {
                    "siege" : true
                }
            },
            "log" : {
                "weight" : 500,
                "limited" : true,
                "construction" : { 
                    "skill" : "forestry",
                    "minskill" : 1
                }
            },
            "iron" : {
                "material": true,
                "weight" : 5000,
                "limited" : true,
                "construction" : { 
                    "skill" : "mining",
                    "minskill" : 1
                }
            }
        },
        "terrains" : {
            "plain": {
                "size": 10000,
                "flags" : [ "land", "walk", "sail" ]
             },
            "mountain": {
                "size": 1000,
                "flags" : [ "land", "walk" ]
            }
        },
        "parameters" : {
            "de" : {
                "SCHIFF" : "SCHIFF",
                "BURG" : "BURG"
            }
        },
        "keywords" : {
            "de" : {
                "recruit" : "REKRUTIEREN",
                "guard" : "BEWACHEN",
                "work" : "ARBEITEN",
                "make" : "MACHEN",
                "forget" : "VERGESSE"
            }
        },
        "strings" : {
            "de" : {
                "skill::magic" : "Magie",
                "money" : "Silber",
                "log" : "Holz",
                "catapult" : "Katapult",
                "caravel" : "Karavelle",
                "sawmill" : "Saegewerk"
            }
        }
    }]]

    eressea.free_game()
    eressea.config.reset()
    eressea.config.parse(conf)
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("study.produceexp", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4") -- FOOD_IS_FREE
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("rules.peasants.growth.factor", "0")
end

function disable_test_forestry_no_guards()
    local r = region.create(0, 0, "plain")
    r:set_resource("tree", 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill("forestry", 1)
    u:add_order("MACHE HOLZ")
    process_orders()
    assert_equal(1, u:get_item("log"))
    process_orders()
    assert_equal(2, u:get_item("log"))
end

function test_work()
    local r = region.create(0, 0, "plain")
    r:set_resource('tree', 0)
    r:set_resource('seed', 0)
    r:set_resource('sapling', 0)
    r:set_resource('peasant', 100)
    r:set_resource('money', 0)
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order("ARBEITE")
    process_orders()
    assert_equal(10, u:get_item('money'))
    assert_equal(100, r:get_resource('money'))
end

function test_make_nothing()
    local r = region.create(0, 0, "mountain")
    r:set_resource("iron", 100)
    local level = r:get_resourcelevel("iron")
    assert_equal(1, level)
    local u = unit.create(faction.create("human"), r)
    u.number = 10
    u:set_skill("mining", 1)
    u:add_order("MACHE 0 EISEN")
    process_orders()
    assert_equal(1, u.faction:count_msg_type("error_cannotmake"))
    assert_equal(0, u:get_item("iron"))
    assert_equal(100, r:get_resource("iron"))
end

function test_make_limited_ship()
    local r = region.create(0, 0, "plain")
    local u = unit.create(faction.create("human"), r)
    u.number = 10
    u:set_skill("shipcraft", 6)
    u:add_item("log", 5)
    u:add_order("MACHE 1 Karavelle")
    process_orders()
    assert_equal(4, u:get_item("log"))
    assert_equal(1, u.ship.size)
    assert_equal("MACHEN 1 Karavelle", u:get_order())
end
