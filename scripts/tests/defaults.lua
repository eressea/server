local tcname = 'tests.defaults'
local lunit = require("lunit")
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname , 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    conf = [[{
        "races": {
            "human" : {}
        },
        "terrains" : {
            "plain": { "flags" : [ "land", "walk", "sail" ] }
        },
        "directions" : {
            "de" : {
                "east" : "OSTEN",
                "west" : "WESTEN"
            }
        },
        "keywords" : {
            "de" : {
                "buy" : "KAUFE",
                "sell" : "VERKAUFE",
                "move" : "NACH",
                "work" : "ARBEITE",
                "default" : "DEFAULT",
                "entertain" : "UNTERHALTE",
                "guard" : "BEWACHE"
            }
        }
    }]]

    eressea.config.reset()
    eressea.config.parse(conf)
end

function test_add_order()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    assert_nil(u:get_order())
    assert_nil(u:get_orders())
    u:add_order("ARBEITE")
    assert_nil(u:get_order())
    local tbl = u:get_orders()
    assert_equal(1, #tbl)
    assert_equal("ARBEITE", tbl[1])
    process_orders()
    assert_equal("ARBEITE", u:get_order())
    assert_equal(1, #tbl)
    assert_equal("ARBEITE", tbl[1])
end

function test_set_orders()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:set_orders("ARBEITE\nBEWACHE")
    assert_nil(u:get_order())
    assert_not_nil(u:get_orders())
    local tbl = u:get_orders()
    assert_equal(2, #tbl)
    assert_equal("ARBEITE", tbl[1])
    assert_equal("BEWACHE", tbl[2])
    process_orders()
    assert_equal("ARBEITE", u:get_order())
    assert_not_nil(u:get_orders())
end

function test_replace_defaults()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    -- add_order sets the default orders as if they were loaded from save:
    u:add_order("UNTERHALTE")
    local defaults = u:get_orders()
    assert_equal(1, #defaults)
    assert_equal("UNTERHALTE", defaults[1])
    -- set_orders behaves like reading from a file.
    u:set_orders("ARBEITE\nBEWACHE")
    local orders = u:get_orders()
    assert_equal(2, #orders)
    assert_equal("ARBEITE", orders[1])
    assert_equal("BEWACHE", orders[2])
    process_orders()
    -- make sure that this unit tried to work, not entertain:
    assert_equal("ARBEITE", u:get_order())
    -- short order is not kept:
    defaults = u:get_orders()
    assert_equal(1, #defaults)
    assert_equal("ARBEITE", defaults[1])
end

function test_rewrite_abbrevs()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)

    u:set_orders("ARB\n@BEWAC")
    process_orders()
    local orders = u:get_orders()
    assert_equal("ARBEITE", orders[1])
    assert_equal("@BEWACHE", orders[2])
end

function test_default()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:set_orders('ARBEITE\nDEFAULT UNTERHALTE')
    process_orders()
    assert_equal('ARBEITE', u:get_order())
    local orders = u:get_orders()
    assert_equal("UNTERHALTE", orders[1])
end
