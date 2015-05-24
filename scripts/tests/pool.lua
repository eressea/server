require "lunit"

module("tests.eressea.pool", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.config.reset()
    eressea.settings.set("rules.economy.food", "0")
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.magic.playerschools", "")
    conf = [[{
        "races": {
            "human" : { "flags" : [ "giveitem", "getitem" ] }
        },
        "terrains" : {
            "plain": { "flags" : [ "land" ] }
        },
        "keywords" : {
            "de" : {
                "give" : "GIB",
                "contact" : "KONTAKTIERE"
            }
        },
        "strings" : {
            "de" : {
                "money" : "Silber"
            }
        }
    }]]

    assert(eressea.config.parse(conf)==0)
end

function test_give_nopool()
    local r = region.create(1, 1, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u1:add_item("money", 100)
    u1:add_order("GIB " .. itoa36(u2.id) .. " 50 SILBER")
    process_orders()
    assert_equal(50, u1:get_item("money"))
    assert_equal(50, u2:get_item("money"))
end

function test_give_from_faction()
    local r = region.create(1, 1, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    local u3 = unit.create(f, r, 1)
    u1:add_item("money", 50)
    u2:add_item("money", 50)
    u1:add_order("GIB " .. itoa36(u3.id) .. " 100 SILBER")
    process_orders()
    assert_equal(0, u1:get_item("money"))
    assert_equal(0, u2:get_item("money"))
    assert_equal(100, u3:get_item("money"))
end

function test_give_divisor()
    eressea.settings.set("rules.items.give_divisor", 2)
    eressea.settings.set("GiveRestriction", 0)
    local r = region.create(1, 1, "plain")
    local f1 = faction.create("test@example.com", "human", "de")
    local f2 = faction.create("test@example.com", "human", "de")
    local u1 = unit.create(f1, r, 1)
    local u2 = unit.create(f2, r, 1)
    u2:add_order("KONTAKTIERE " .. itoa36(u1.id))
    u1:add_item("money", 100)
    u1:add_order("GIB " .. itoa36(u2.id) .. " 100 SILBER")
    process_orders()
    assert_equal(0, u1:get_item("money"))
    assert_equal(50, u2:get_item("money"))
end
