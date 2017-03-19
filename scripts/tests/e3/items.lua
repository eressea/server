require "lunit"

module("tests.e3.items", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4") -- food is free
    eressea.settings.set("NewbieImmunity", "0")
end

function test_give_horses()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)

    r:set_resource("horse", 0)
    u:add_item("charger", 20)
    u:add_order("GIB 0 10 Streitross")
    process_orders()
    assert_equal(10, r:get_resource("horse"))
    assert_equal(10, u:get_item("charger"))
end

function test_goblins()
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("goblin", "goblin@eressea.de", "de")
    local f2 = faction.create("dwarf", "dwarf@eressea.de", "de")
    local f3 = faction.create("elf", "elf@eressea.de", "de")
    local ud = unit.create(f1, r, 1)
    local uh = unit.create(f1, r, 1)
    uh.race = "halfling"
    local u2 = unit.create(f2, r, 1)
    local u3 = unit.create(f3, r, 1)

    local restricted = {
    "towershield", "rep_crossbow", "plate", "lance",
    "mallornlance", "greatbow", "greataxe", "axe", "scale",
    "plate", "halberd", "greatsword", "rustyhalberd"
    }
    for k, v in ipairs(restricted) do
        ud:add_item(v, 1)
        uh:add_item(v, 1)
        u2:add_item(v, 1)
        u3:add_item(v, 1)
    end

    uh:add_order("ATTACKIERE " .. itoa36(u2.id))
    uh:add_order("ATTACKIERE " .. itoa36(u3.id))
    ud:add_order("ATTACKIERE " .. itoa36(u2.id))
    ud:add_order("ATTACKIERE " .. itoa36(u3.id))
    process_orders()
end

function test_make_horse()
    eressea.settings.set("rules.horses.growth", "0")
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "horses@eressea.de", "de")
    local u = unit.create(f, r, 1)
    u:set_skill("training", 4)
    r:set_resource("horse", 100)
    u:add_order("MACHE 1 PFERD")
    process_orders()
    assert_equal(1, u:get_item("horse"))
    assert_equal(99, r:get_resource("horse"))

    u:clear_orders()
    u:add_order("MACHE 1 STREITROSS")
    u:add_item("money", 200)
    u:add_item("iron", 1)
    process_orders()
    assert_equal(1, u:get_item("charger"))
    assert_equal(0, u:get_item("horse"))
    assert_equal(0, u:get_item("iron"))
    assert_equal(0, u:get_item("money"))
    assert_equal(99, r:get_resource("horse"))
end
