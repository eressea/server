require "lunit"

module("tests.laws", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    conf = [[{
        "races": {
            "human" : {}
        },
        "terrains" : {
            "plain": { "flags" : [ "land", "walk", "sail" ] }
        },
        "keywords" : {
            "de": {
                "attack" : "ATTACKIERE",
                "maketemp" : "MACHETEMP",
                "end" : "ENDE",
                "recruit" : "REKRUTIERE"
            }
        },
        "buildings" : {
            "castle" : {}
        }
    }]]

    eressea.config.reset()
    eressea.config.parse(conf)
end

function test_force_leave_on()
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("human", "owner@eressea.de")
    local f2 = faction.create("human", "guest@eressea.de")
    local u1 = unit.create(f1, r, 1)
    local u2 = unit.create(f2, r, 1)
    local b1 = building.create(r, "castle")
    u1.building = b1
    u2.building = b1
    eressea.settings.set("rules.owners.force_leave", "2")
    process_orders()
    assert_equal(b1, u1.building)
    assert_equal(nil, u2.building)
end

function test_force_leave_off()
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("human", "owner@eressea.de")
    local f2 = faction.create("human", "guest@eressea.de")
    local u1 = unit.create(f1, r, 1)
    local u2 = unit.create(f2, r, 1)
    local b1 = building.create(r, "castle")
    u1.building = b1
    u2.building = b1
    eressea.settings.set("rules.owners.force_leave", "0")
    process_orders()
    assert_equal(b1, u1.building)
    assert_equal(b1, u2.building)
end

function test_make_temp()
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("human", "owner@eressea.de", "de")
    local u1 = unit.create(f1, r, 10)
    local u, u2

    u1.building = building.create(r, "castle")
    u1.status = 1
    u1:clear_orders()
    u1:add_order("MACHETEMP 1 Hodor")
    u1:add_order("REKRUTIERE 1")
    u1:add_order("ENDE")
    process_orders()
    for u in r.units do
        if u~=u1 then
            u2 = u
            break
        end
    end
    assert_not_equal(nil, u2)
    assert_not_equal(nil, u2.building)
    assert_equal(1, u2.number)
    assert_equal(1, u2.status)
    assert_equal("Hodor", u2.name)
end

function test_force_leave_postcombat()
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("human", "owner@eressea.de", "de")
    local f2 = faction.create("human", "guest@eressea.de", "de")
    local u1 = unit.create(f1, r, 10)
    local u2 = unit.create(f2, r, 10)
    local u, u3
    local b1 = building.create(r, "castle")
    u1.building = b1
    u2.building = b1
    eressea.settings.set("rules.owners.force_leave", "1")
    eressea.settings.set("NewbieImmunity", "0")
    u1:clear_orders()
    u1:add_order("ATTACKIERE " .. itoa36(u2.id))
    u2:clear_orders()
    u2:add_order("MACHETEMP 2 Hodor")
    u2:add_order("REKRUTIERE 1")
    u2:add_order("ENDE")
    process_orders()
    for u in r.units do
        if u~=u1 and u~=u2 then
            u3 = u
            break
        end
    end
    assert_not_equal(nil, u3)
    assert_equal(nil, u2.building)
    assert_equal(nil, u3.building)
    assert_equal(1, u3.number)
end
