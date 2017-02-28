require "lunit"

module("tests.e3.items", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4") -- food is free
    eressea.settings.set("NewbieImmunity", "0")
end

function test_goblins()
    local r = region.create(0, 0, "plain")
    assert(r)
    local f1 = faction.create("goblin@eressea.de", "goblin", "de")
    local f2 = faction.create("dwarf@eressea.de", "dwarf", "de")
    local f3 = faction.create("elf@eressea.de", "elf", "de")
    local u1 = unit.create(f1, r, 1)
    local u2 = unit.create(f2, r, 1)
    local u3 = unit.create(f3, r, 1)

    local restricted = {
    "towershield", "rep_crossbow", "plate", "lance",
    "mllornlance", "greatbow", "greataxe", "axe", "scale",
    "plate", "rustyhalberd", "halberd", "greatsword"
    }
    for k, v in ipairs(restricted) do
        u1:add_item(v, 1)
        u2:add_item(v, 1)
        u3:add_item(v, 1)
    end

    u1:add_order("ATTACKIERE " .. itoa36(u2.id))
    u1:add_order("ATTACKIERE " .. itoa36(u3.id))
    process_orders()
end
