local tcname = 'tests.shared.parser'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("rules.food.flags", "4") -- FOOD_IS_FREE
    eressea.settings.set("rules.move.owner_leave", "0")
end

function test_parser()
    local r = region.create(0, 0, "mountain")
    local f = faction.create('human')
    local u = unit.create(f, r, 1)
    local filename = "orders.txt"
    
    local file = io.open(filename, "w")
    assert_not_nil(file)
    f.password = 'Hodor'
    file:write('ERESSEA ' .. itoa36(f.id) .. ' "Hodor"\n')
    file:write('EINHEIT ' .. itoa36(u.id) .. "\n")
    file:write("BENENNEN EINHEIT 'Goldene Herde'\n")
    file:close()
    
    eressea.read_orders(filename)
    process_orders()
    os.remove(filename)
    assert_equal("Goldene Herde", u.name)
end

local function set_order(u, str)
    u:clear_orders()
    u:add_order(str)
end

function test_prefix()
    local r0 = region.create(0, 0, "plain")
    local f1 = faction.create('human')
    local u1 = unit.create(f1, r0, 1)

    set_order(u1, "PRAEFIX See")
    process_orders()
    assert_not_nil(u1:show():find("Seemensch"))

    u1.race = "elf"
    assert_not_nil(u1:show():find("Seeelf"))

    set_order(u1, "PRAEFIX Mond")
    process_orders()
    assert_not_nil(u1:show():find("Mondelf"))

    set_order(u1, "PRAEFIX")
    process_orders()
    assert_not_nil(u1:show():find("Elf"))

    set_order(u1, "PRAEFIX Erz")
    process_orders()
    assert_not_nil(u1:show():find("Erzelf"))
    u1.faction.locale = "en"
    assert_not_nil(u1:show():find("archelf"))
end

function test_recruit()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = unit.create(f, r, 1)

    u:add_item("money", 1000)
    set_order(u, "REKRUTIERE 5")
    process_orders()
    for u in f.units do
        assert_equal(6, u.number)
    end
end

function test_give_horses()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)

    r:set_resource("horse", 0)
    u:add_item("horse", 21)
    u:add_item("dolphin", 10)
    u:add_order("GIB 0 7 PFERD")
    u:add_order("GIB 0 5 DELPHIN")
    process_orders()
    assert_equal(7, r:get_resource("horse"))
    assert_equal(5, u:get_item("dolphin"))
    assert_equal(14, u:get_item("horse"))
end

function test_give_silver()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)

    r:set_resource("peasant", 0)
    r:set_resource("money", 11)
    u:clear_orders()
    u:add_item("money", 20)
    u:add_order("GIB 0 10 SILBER")
    process_orders()
    assert_equal(21, r:get_resource("money"))
    assert_equal(10, u:get_item("money"))
end

function test_give_horses()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = unit.create(f, r, 1)

    r:set_resource("horse", 0)
    u:add_item("horse", 21)
    u:add_item("dolphin", 10)
    u:add_order("GIB 0 7 PFERD")
    u:add_order("GIB 0 5 DELPHIN")
    process_orders()
    assert_equal(7, r:get_resource("horse"))
    assert_equal(5, u:get_item("dolphin"))
    assert_equal(14, u:get_item("horse"))
end

function test_give_silver()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = unit.create(f, r, 1)

    r:set_resource("peasant", 0)
    r:set_resource("money", 11)
    u:clear_orders()
    u:add_item("money", 20)
    u:add_order("GIB 0 10 SILBER")
    process_orders()
    assert_equal(21, r:get_resource("money"))
    assert_equal(10, u:get_item("money"))
end

function test_build_castle_one_stage()
    local r = region.create(0, 0, 'plain')
    local f = faction.create('human')
    local u = unit.create(f, r, 2)

    u:add_item('stone', 4)

    u:set_skill('building', 1)
    u:add_order('MACHE BURG')

    process_orders()
    assert_equal(2, u.building.size)
    assert_equal(2, u:get_item('stone'))
end

function test_build_castle()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = unit.create(f, r, 1)

    u:add_item('stone', 1)
    u:set_skill('building', 1)
    u:add_order("MACHE BURG")
    process_orders()
    assert_not_nil(u.building)
    assert_equal(1, u.building.size)
    assert_equal(u.building.name, "Burg")
end

function test_route()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "route@example.com")
    local u = unit.create(f, r1, 1)
    u:add_order("ROUTE O W P")
    process_orders()
    assert_equal("ROUTE West PAUSE Ost", u:get_order(0))
    assert_equal(r2, u.region)
end

function test_route_horse()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "route@example.com")
    local u = unit.create(f, r1, 1)
    u:add_order("ROUTE O P W P")
    u:add_item('horse', 1)
    u:set_skill('riding', 1)
    process_orders()
    assert_equal("ROUTE West PAUSE Ost PAUSE", u:get_order(0))
    assert_equal(r2, u.region)
end

function test_route_pause()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "route@example.com")
    local u = unit.create(f, r1, 1)
    u:add_order("ROUTE P O W")
    process_orders()
    assert_equal("ROUTE P O W", u:get_order(0))
    assert_equal(r1, u.region)
end

function test_region_keys()
    local r = region.create(0, 0, 'plain')
    assert_nil(r:get_key('test'))
    assert_nil(r:get_key('more'))
    r:set_key('test', 42)
    r:set_key('more') -- default is 1
    assert_equal(42, r:get_key('test'))
    assert_equal(1, r:get_key('more'))
end

function test_faction_keys()
    local f = faction.create('human')
    assert_nil(f:get_key('test'))
    assert_nil(f:get_key('more'))
    f:set_key('test', 42)
    f:set_key('more') -- default is 1
    assert_equal(42, f:get_key('test'))
    assert_equal(1, f:get_key('more'))
end

function test_cartmaking()
    local f = faction.create('human')
    local r = region.create(0, 0, 'plain')
    local u = unit.create(f, r)
    u:set_skill('cartmaking', 1)
    u:add_item('log', 10)
    u:add_order('MACHE Wagen')
    process_orders()
    assert_equal(1, u:get_item('cart'))
    assert_equal(5, u:get_item('log'))
end

function test_promote_after_recruit()
    local f = faction.create('human')
    local r1 = region.create(0, 0, 'plain')
    local r2 = region.create(1, 0, 'plain')
    local offset = eressea.settings.get('rules.heroes.offset') or 0
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r2, 55 + offset)
    u2:add_order('REKRUTIERE 1')
    u1:add_order('BEFOERDERE')
    u1:add_item('money', 57 + offset)
    u2:add_item('money', 150)
    local fl = u1.flags
    process_orders()
    assert_equal(offset + 56, u2.number)
    assert_equal(fl + 128, u1.flags) -- UFL_HERO
    assert_equal(0, u1:get_item('money'))
    assert_equal(fl + 128, u1.flags) -- UFL_HERO
end

function test_defaults_make_temp()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order("ARBEITE")
    local filename = "orders.txt"
    -- suppress NMR check:
    f.flags = f.flags + 16777216
    
    local file = io.open(filename, "w")
    assert_not_nil(file)
    f.password = 'Hodor'
    file:write('ERESSEA ' .. itoa36(f.id) .. ' "Hodor"\n')
    file:write('EINHEIT ' .. itoa36(u.id) .. "\n")
    file:write("MACHE TEMP 1\nTREIBE\nENDE\nLERNE Taktik\n@RESERVIERE 1 Schwert")
    file:close()
    
    eressea.read_orders(filename)
    process_orders()
    os.remove(filename)
    assert_equal("LERNE Taktik", u:get_order(0))
    assert_equal("@RESERVIERE 1 Schwert", u:get_order(1))
    -- should get only the LERNE error:
    assert_equal('error65', f.messages[1])
end
