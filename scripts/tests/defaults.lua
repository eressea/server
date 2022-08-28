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
            "human" : {
                "flags": [ "walk" ]
            }
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
                "//" : "//",
                "buy" : "KAUFE",
                "sell" : "VERKAUFE",
                "move" : "NACH",
                "work" : "ARBEITE",
                "default" : "DEFAULT",
                "entertain" : "UNTERHALTE",
                "give": "GIB",
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
    assert_nil(u.orders)
    u:add_order("ARBEITE")
    assert_nil(u:get_order())
    assert_equal(1, #u.orders)
    assert_equal("ARBEITE", u.orders[1])
    process_orders()
    assert_equal("ARBEITE", u:get_order())
    assert_equal(1, #u.orders)
    assert_equal("ARBEITE", u.orders[1])
end

function test_set_orders()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:set_orders("ARBEITE\nBEWACHE")
    assert_nil(u:get_order())
    assert_not_nil(u.orders)
    assert_equal(2, #u.orders)
    assert_equal("ARBEITE", u.orders[1])
    assert_equal("BEWACHE", u.orders[2])
    process_orders()
    assert_equal("ARBEITE", u:get_order())
    assert_not_nil(u.orders)
end

function test_replace_defaults()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    -- add_order sets the default orders as if they were loaded from save:
    u:add_order("UNTERHALTE")
    assert_equal(1, #u.orders)
    assert_equal("UNTERHALTE", u.orders[1])
    -- set_orders behaves like reading from a file.
    u:set_orders("ARBEITE\nBEWACHE")

    assert_equal(2, #u.orders)
    assert_equal("ARBEITE", u.orders[1])
    assert_equal("BEWACHE", u.orders[2])
    process_orders()
    -- make sure that this unit tried to work, not entertain:
    assert_equal("ARBEITE", u:get_order())
    -- short order is not kept:
    assert_equal(1, #u.orders)
    assert_equal("ARBEITE", u.orders[1])
end

function test_rewrite_abbrevs()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)

    u:set_orders("ARB\n@BEWAC")
    process_orders()
    assert_equal("ARBEITE", u.orders[1])
    assert_equal("@BEWACHE", u.orders[2])
end

function test_default()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:set_orders('ARBEITE\nDEFAULT UNTERHALTE')
    process_orders()
    assert_equal('ARBEITE', u:get_order())
    assert_equal("UNTERHALTE", u.orders[1])
end

function test_default_move()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order('ARBEITE')
    u:add_order('@GIB 0 1 Silber')
    u:set_orders('DEFAULT "NACH OSTEN"\nARBEITE\n@GIB 0 2 Silber')
    process_orders()
    assert_equal("NACH OSTEN", u.orders[1])
    assert_equal("@GIB 0 2 Silber", u.orders[2])
    process_orders()
    assert_equal("@GIB 0 2 Silber", u.orders[1])
end

function test_default_empty()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order('ARBEITE')
    u:add_order('// nix')
    u:set_orders('DEFAULT\nARBEITE')
    process_orders()
    assert_equal("ARBEITE", u.orders[1])
    assert_equal(1, #u.orders)
end

function test_default_default()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order('ARBEITE')
    u:set_orders('DEFAULT "DEFAULT UNTERHALTE"')
    process_orders()
    assert_equal("ARBEITE", u.orders[1])
    assert_equal("DEFAULT UNTERHALTE", u.orders[2])
    assert_equal(2, #u.orders)
    
end

function test_movement_does_not_replace_default()
    local r = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order('ARBEITE')
    u:set_orders('NACH OSTEN\n@BEWACHE')
    process_orders()
    assert_nil(u:get_order()) -- no long order because of movement
    assert_equal("ARBEITE", u.orders[1])
    assert_equal("@BEWACHE", u.orders[2])
    assert_equal(r2, u.region)
end