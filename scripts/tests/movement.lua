require "lunit"

module("tests.movement", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.ships.storms", "0")
    conf = [[{
        "races": {
            "human" : {
                "speed" : 1,
                "weight" : 1000,
                "capacity" : 1500,
                "flags" : [ "walk" ]
            },
            "troll" : {}
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
                "move" : "NACH"
            }
        }
    }]]

    eressea.config.reset()
    eressea.config.parse(conf)
end

function test_walk_to_land()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "walk@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_order("NACH O")
    process_orders()
    assert_equal(r2, u.region)
end

function test_walk_into_ocean_fails()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_order("NACH O")
    process_orders()
    assert_equal(r1, u.region)
end

function test_walk_distance()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    region.create(2, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_order("NACH O O")
    process_orders()
    assert_equal(r2, u.region)
end

function test_ride_max_distance()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    region.create(1, 0, "plain")
    region.create(3, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_item("horse", 1)
    u:set_skill("riding", 2)
    u:add_order("NACH O O O")
    process_orders()
    assert_equal(r2, u.region, "should ride exactly two hexes")
end

function test_ride_over_capacity_leads_horse()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    region.create(2, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 3)
    u:add_item("horse", 1)
    u:set_skill("riding", 2)
    u:add_order("NACH O O")
    process_orders()
    assert_equal(r2, u.region)
end

function test_ride_no_skill_leads_horse()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    region.create(2, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_item("horse", 1)
    u:add_order("NACH O O")
    process_orders()
    assert_equal(r2, u.region)
end
