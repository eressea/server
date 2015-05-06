require "lunit"

module("tests.e3.spells", package.seeall, lunit.testcase)

function setup()
    eressea.game.reset()
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.peasants.growth", "0")
end

function test_blessedharvest_lasts_n_turn()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "halfling", "de")
    local u = unit.create(f, r)
    local err = 0
    r:set_resource("peasant", 100)
    r:set_resource("money", 0)
    u:add_item("money", 1000)
    u.magic = "gwyrrd"
    u.race = "dwarf"
    u:set_skill("magic", 20)
    u.aura = 200
    err = err + u:add_spell("raindance")
    err = err + u:add_spell("blessedharvest")
    assert_equal(0, err)
    
    u:clear_orders()
    u:add_order("ZAUBERE STUFE 3 Regentanz")
    assert_equal(0, r:get_resource("money"), 0)
    
    local m = 0
    local p = 100

    process_orders()
    assert_equal(200, r:get_resource("money"))
    u:clear_orders()
    u:add_order("ARBEITEN")
    process_orders()
    process_orders()
    process_orders()
    assert_equal(800, r:get_resource("money"))
    process_orders()
    assert_equal(900, r:get_resource("money"))
end

function test_magic()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "halfling", "de")
    local u = unit.create(f, r)
    local b = building.create(r, "castle")

    u.race = "dwarf"
    u.magic = "gwyrrd"
    u:set_skill("magic", 30)
    u.aura = 300

    u:add_spell("protective_runes")
    u:add_spell("analyze_magic")
    u:clear_orders()
    u:add_order("ZAUBERE \"Runen des Schutzes\" BURG " .. itoa36(b.id));
    u.building = b
    u:add_order("ZAUBERE \"Magie analysieren\" BURG " .. itoa36(b.id));
    process_orders()
    write_reports()
end
