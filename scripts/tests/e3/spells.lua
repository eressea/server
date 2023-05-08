local tcname = 'tests.e3.spells'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.game.reset()
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("rules.peasants.growth", "0")
end

function test_raindance()
    local r = region.create(0, 0, "plain")
    local f = faction.create("halfling", "noreply@eressea.de", "de")
    local u = unit.create(f, r)
    r:set_resource("peasant", 100)
    r:set_resource("money", 0)
    u.magic = "gwyrrd"
    u.race = "dwarf"
    u:set_skill("magic", 20)
    u.aura = 200
    local err = u:add_spell("raindance")
    assert_equal(0, err)
    
    u:clear_orders()
    u:add_order("ZAUBERE STUFE 3 Regentanz")
    assert_equal(0, r:get_resource("money"), 0)
    
    process_orders()
    assert_equal(200, r:get_resource("money"))
    u:clear_orders()
    u:add_order("ARBEITEN")
    process_orders()
    process_orders()
    assert_equal(500, r:get_resource("money"))
    process_orders()
    assert_equal(600, r:get_resource("money"))
end

function test_magic()
    local r = region.create(0, 0, "plain")
    local f = faction.create("halfling", "noreply@eressea.de", "de")
    local u = unit.create(f, r)
    local b = building.create(r, "castle")

    u.race = "dwarf"
    u.magic = "gwyrrd"
    u:set_skill("magic", 30)
    u.aura = 300

    u:add_spell("protective_runes")
    u:add_spell("analyze_magic")
    u:clear_orders()
    u.building = b
    u:add_order("ZAUBERE \"Magie analysieren\" BURG " .. itoa36(b.id));
    process_orders()
--  there used to be a SEGFAULT when writing reports here:
--    write_reports()
end

-- E3: earn 50 per every TWO levels of spell
function test_earn_silver()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human")
    local u = unit.create(f, r)

    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("rules.peasants.growth", "0")
    eressea.settings.set("rules.economy.repopulate_maximum", "0")

    u.magic = "gwyrrd"
    u.race = "elf"
    u:set_skill("magic", 10)
    u.aura = 100
    local err = u:add_spell("earn_silver#gwyrrd")
    assert_equal(0, err)

    u:clear_orders()
    u:add_order("ZAUBERE STUFE 1 Viehheilung")
    r:set_resource("money", 200)
    r:set_resource("peasant", 0)
    process_orders() -- get 50 silver
    assert_equal(50, u:get_item("money"))
    assert_equal(150, r:get_resource("money"))

    u:clear_orders() -- get 50 silver
    u:add_order("ZAUBERE STUFE 2 Viehheilung")
    process_orders()
    assert_equal(100, u:get_item("money"))
    assert_equal(100, r:get_resource("money"))

    u:clear_orders() -- get 75 silver
    u:add_order("ZAUBERE STUFE 3 Viehheilung")
    process_orders()
    assert_equal(175, u:get_item("money"))
    assert_equal(25, r:get_resource("money"))

    process_orders() -- not enough
    assert_equal(200, u:get_item("money"))
    assert_equal(0, r:get_resource("money"))
end
