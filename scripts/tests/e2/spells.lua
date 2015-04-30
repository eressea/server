require "lunit"

module("tests.e2.spells", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.economy.food", "4")
end

function test_shapeshift()
    local r = region.create(42, 0, "plain")
    local f = faction.create("noreply@eressea.de", "demon", "de")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u1:clear_orders()
    u1.magic = "gray"
    u1:set_skill("magic", 2)
    u1.aura = 1
    u1:add_spell("shapeshift")
    u1:add_order("ZAUBERE STUFE 1 Gestaltwandlung " .. itoa36(u2.id) .. " Goblin")
    process_orders()
    assert_equal(f.race, u2.race)
    s = u2:show()
    assert_equal("1 Goblin", string.sub(s, string.find(s, "1 Goblin")))
end
