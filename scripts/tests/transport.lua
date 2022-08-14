local tcname = 'tests.transport'
local lunit = require("lunit")
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname , 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.ships.storms", "0")
    conf = [[{
        "races": {
            "human" : {
                "speed" : 1,
                "weight" : 1000,
                "capacity" : 540,
                "flags" : [ "walk" ]
            },
            "troll" : {
                "weight" : 2000,
                "capacity" : 1080,
                "flags" : [ "walk" ]
            },
            "aquarian" : {
                "weight" : 1000,
                "capacity" : 540,
                "flags" : [ "walk", "coastal" ]
            }
        },
        "items" : {
            "horse" : {
                "capacity" : 7000,
                "weight" : 5000,
                "flags" : [ "big", "animal" ]
            }
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
                "move" : "NACH",
                "carry" : "TRANSPORTIERE",
                "ride" : "FAHRE"
            }
        }
    }]]

    eressea.config.reset()
    eressea.config.parse(conf)
end

function test_transport()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human")
    local f2 = faction.create("human")
    local u1 = unit.create(f, r2, 1)
    local u2 = unit.create(f2, r2, 1)
    u1.number = 10

    u2:add_order("FAHRE " .. itoa36(u1.id))
    u1:add_order("TRANSPORTIERE " .. itoa36(u2.id))
    u1:add_order("NACH w")

    process_orders()

    assert_equal(r1, u1.region)
    assert_equal(r1, u2.region)
end

function test_aquarian_transport()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("aquarian")
    local f2 = faction.create("aquarian")
    local u1 = unit.create(f, r2, 1)
    local u2 = unit.create(f2, r2, 1)
    u1.number = 10

    u2:add_order("FAHRE " .. itoa36(u1.id))
    u1:add_order("TRANSPORTIERE " .. itoa36(u2.id))
    u1:add_order("NACH w")

    process_orders()

    assert_equal(r1, u1.region)
    assert_equal(r1, u2.region)
end

function test_aquarian_transport_capacity()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("aquarian", "trans_mm@eressea.de", "de")
    local f2 = faction.create("aquarian", "rider_mm@eressea.de", "de")
    local u1 = unit.create(f, r2, 1)
    local u2 = unit.create(f2, r2, 1)
    u1.id = atoi36("u1")
    u2.id = atoi36("u2") -- too heavy for one carrier

    u2:add_order("FAHRE " .. itoa36(u1.id))
    u1:add_order("TRANSPORTIERE " .. itoa36(u2.id))
    u1:add_order("NACH w")

    process_orders()

    assert_not_equal(r1, u1.region)
    assert_not_equal(r1, u2.region)
end
