require "lunit"

module("tests.eressea.pool", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
    eressea.config.reset();
    eressea.settings.set("rules.economy.food", "0")
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    conf = [[{
        "races": {
            "human" : {}
        },
        "terrains" : {
            "plain": { "flags" : [ "land" ] }
        },
        "keywords" : {
            "de" : {
                "give" : "GIB"
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
    u1:add_order("GIB " .. itoa36(u2.id) .. " 100 SILBER")
    process_orders()
    assert_equal(0, u1:get_item("money"))
    assert_equal(100, u2:get_item("money"))
end
