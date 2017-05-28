require "lunit"

module("tests.e2.spells", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.peasants.growth.factor", "0")
end

function test_shapeshift()
    local r = region.create(42, 0, "plain")
    local f = faction.create("demon", "noreply@eressea.de", "de")
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

function test_raindance()
    local r = region.create(0, 0, "plain")
    local f = faction.create("halfling", "noreply@eressea.de", "de")
    local u = unit.create(f, r)
    local err = 0
    r:set_resource("peasant", 100)
    r:set_resource("money", 0)
    u.magic = "gwyrrd"
    u.race = "dwarf"
    u:set_skill("magic", 20)
    u.aura = 200
    err = err + u:add_spell("raindance")
    assert_equal(0, err)
    
    u:clear_orders()
    u:add_order("ZAUBERE STUFE 1 Regentanz")
    assert_equal(0, r:get_resource("money"))

    process_orders()
    assert_equal(200, r:get_resource("money"))
    assert_equal(0, u:get_item("money"))

    u:clear_orders()
    u:add_order("ARBEITEN")
    process_orders()
    assert_equal(10, u:get_item("money")) -- only peasants benefit
    assert_equal(400, r:get_resource("money"))
    -- this is where the spell ends
    process_orders()
    process_orders()
    assert_equal(600, r:get_resource("money"))
end

function test_earn_silver()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human")
    local u = unit.create(f, r)

    eressea.settings.set("rules.food.flags", "4")
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
    r:set_resource("money", 350)
    r:set_resource("peasant", 0)
    process_orders() -- get 50 silver
    assert_equal(50, u:get_item("money"))
    assert_equal(300, r:get_resource("money"))

    u:clear_orders() -- get 100 silver
    u:add_order("ZAUBERE STUFE 2 Viehheilung")
    process_orders()
    assert_equal(150, u:get_item("money"))
    assert_equal(200, r:get_resource("money"))

    u:clear_orders() -- get 150 silver
    u:add_order("ZAUBERE STUFE 3 Viehheilung")
    process_orders()
    assert_equal(300, u:get_item("money"))
    assert_equal(50, r:get_resource("money"))

    process_orders() -- not enough
    assert_equal(350, u:get_item("money"))
    assert_equal(0, r:get_resource("money"))
end
