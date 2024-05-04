local tcname = 'tests.laws'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    conf = [[{
        "races": {
            "human" : { "flags" : [ "player", "giveperson", "getitem" ] }
        },
        "terrains" : {
            "plain": { "flags" : [ "land", "walk", "sail" ] }
        },
        "parameters" : {
            "de" : {
                "ALLES": "ALLES",
                "EINHEIT": "EINHEIT",
                "PERSONEN": "PERSONEN"
            }
        },
        "keywords" : {
            "de": {
                "attack" : "ATTACKIERE",
                "guard" : "BEWACHE",
                "steal" : "BEKLAUE",
                "maketemp" : "MACHETEMP",
                "end" : "ENDE",
                "use" : "BENUTZEN",
                "forget" : "VERGISS",
                "give" : "GIB",
                "recruit" : "REKRUTIERE"
            }
        },
        "buildings" : {
            "castle" : {}
        },
        "skills" : {
            "de": {
                "tactics" : "Taktik",
                "alchemy" : "Alchemie",
                "crossbow" : "Armbrust"
            }
        },
        "items" : {
            "sword" : {
                "weapon" : {
                    "skill" : "melee",
                    "damage": "1d9+2"
                }
            }
        }
    }]]

    eressea.config.reset()
    eressea.config.parse(conf)
end

function test_newbie_immunity()
    eressea.settings.set("NewbieImmunity", "2")
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("human", "newb@eressea.de")
    local u1 = unit.create(f1, r, 1)
    u1:add_item("sword", 1)
    u1:set_skill("melee", 2)
    local f2 = faction.create("human")
    local u2 = unit.create(f2, r, 1)
    u1:add_order("@BEWACHE")
    u2:add_order("@ATTACKIERE " .. itoa36(u1.id))
    process_orders()
    assert_equal(1, f2:count_msg_type("newbie_immunity_error"))
    assert_false(u1.guard)
    process_orders()
    assert_equal(1, f2:count_msg_type("newbie_immunity_error"))
    assert_true(u1.guard)
    process_orders()
    assert_equal(0, f2:count_msg_type("newbie_immunity_error"))
end

function test_force_leave_on()
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("human")
    local f2 = faction.create("human")
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
    local f1 = faction.create("human")
    local f2 = faction.create("human")
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
    local f1 = faction.create("human", "temp@eressea.de", "de")
    local r = region.create(0, 0, "plain")
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
    local f1 = faction.create("human", "owner@eressea.de")
    local f2 = faction.create("human", "guest@eressea.de")
    local u1 = unit.create(f1, r, 10)
    local u2 = unit.create(f2, r, 10)
    local u, u3
    local b1 = building.create(r, "castle")
    u1.building = b1
    u2.building = b1
    eressea.settings.set("rules.owners.force_leave", "1")
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

function test_give_and_forget()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local uno = u1.id
    local u2 = unit.create(f, r, 1)
    u1:set_skill('alchemy', 1)
    u1:set_skill('crossbow', 1)
    u2:set_skill('alchemy', 1)
    u1:set_orders("GIB " .. itoa36(u2.id) .. " 1 PERSON\nVERGISS Armbrust")
    process_orders()
    assert_nil(get_unit(uno))
    assert_equal(2, u2.number)
    assert_equal(0, u2:get_skill('crossbow'))
    assert_equal(1, u2:get_skill('alchemy'))
end

-- u1 is poor, so we steal from u2
function test_steal_from_pool()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r)
    local u2 = unit.create(f, r)
    u2:add_item('money', 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill('stealth', 1)
    u:set_orders("BEKLAUE " .. itoa36(u1.id))
    u.name = 'Xolgrim'
    process_orders()
    assert_equal(50, u:get_item('money'))
    assert_equal(50, u2:get_item('money'))
end

function test_thief_must_see_target()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r)
    u1:set_skill('stealth', 1)
    u1:add_item('money', 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill('stealth', 1)
    u:set_orders("BEKLAUE " .. itoa36(u1.id))
    process_orders()
    assert_equal(0, u:get_item('money'))
    assert_equal(100, u1:get_item('money'))
end

function test_temp_cannot_avoid_theft()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r)
    u1:add_item('money', 100)
    u1:set_orders("MACHETEMP 1\nENDE\nGIB TEMP 1 ALLES PERSON\nGIB TEMP 1 ALLES")
    local u = unit.create(faction.create("human"), r)
    u:set_skill('stealth', 1)
    u:set_orders("BEKLAUE " .. itoa36(u1.id))
    process_orders()
    assert_equal(50, u:get_item('money'))
    local u2 = f.units()
    assert_equal(50, u2:get_item('money'))
end
