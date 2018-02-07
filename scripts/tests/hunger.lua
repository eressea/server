require "lunit"

module("tests.hunger", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    conf = [[{
        "races": {
            "demon": {
                "maintenance": 10,
                "hp" : 50
            },
            "insect": {
                "maintenance": 10,
                "hp" : 50
            }
        },
        "terrains" : {
            "plain": { "flags" : [ "land", "walk" ] },
            "glacier": { "flags" : [ "arctic", "land", "walk" ] }
        }
    }]]

    eressea.config.reset()
    eressea.config.parse(conf)

    eressea.settings.set("rules.peasants.growth.factor", "0")
    eressea.settings.set("rules.peasantluck.growth.factor", "0")
    eressea.settings.set("hunger.damage", "10")
end

function test_maintenance()
    local r = region.create(0, 0, "plain")
    local f = faction.create("insect")
    local u = unit.create(f, r, 2)
    local flags = u.flags
    u:add_item('money', 100)

    process_orders()
    assert_equal(flags, u.flags, "should not be hungry")
    assert_equal(80, u:get_item('money'), "should pay maintenance")
    assert_equal(100, u.hp, "should not take damage")
end

function test_demons_eat_peasants()
    local r = region.create(0, 0, "plain")
    local f = faction.create("demon")
    local u = unit.create(f, r, 10)
    local flags = u.flags
    u:add_item('money', 120)

    r.peasants = 3
    process_orders()
    assert_equal(20, u:get_item('money'))
    assert_equal(2, r.peasants)
    assert_equal(flags, u.flags)
    assert_equal(500, u.hp)
end

function test_demons_need_peasants()
    local r = region.create(0, 0, "plain")
    local f = faction.create("demon")
    local u = unit.create(f, r, 1)
    local flags = u.flags
    u:add_item('money', 100)

    eressea.settings.set("hunger.demon.peasant_tolerance", "0")
    r.peasants = 0
    process_orders()
    assert_equal(90, u:get_item('money')) -- use money even if no peasants
    assert_equal(flags+2048, u.flags)
    assert_equal(40, u.hp)

    eressea.settings.set("hunger.demon.peasant_tolerance", "1")
    u.flags = flags
    u.hp = 50
    process_orders()
    assert_equal(80, u:get_item('money')) -- use money even if no peasants
    assert_equal(flags+2048, u.flags)
    assert_equal(50, u.hp)
    assert_equal(0, r.peasants)
end

function test_insects_hunger_in_glacier()
    -- bug 2389
    local r = region.create(0, 0, "plain")
    local f = faction.create("insect")
    local u = unit.create(f, r, 1)
    local flags = u.flags
    u:add_item('money', 1000)

    -- eressea.settings.set("hunger.insect.cold", "1") -- default
    process_orders()
    assert_equal(flags, u.flags)
    assert_equal(50, u.hp)

    r.terrain = 'glacier'
    process_orders()
    assert_equal(flags+2048, u.flags)
    assert_equal(40, u.hp)
    
    u.flags = u.flags-2048
    u.hp = 50
    eressea.settings.set("hunger.insect.cold", "0")
    process_orders()
    assert_equal(flags, u.flags)
    assert_equal(50, u.hp)
end 
