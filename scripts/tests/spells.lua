require "lunit"

module("tests.spells", package.seeall, lunit.testcase)

function setup()
    free_game()
    settings.set("magic.fumble.enable", "0")
end

function test_roi()
    local r = region.create(0,0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.race = "elf"
    u:set_skill("magic", 10)
    u:add_item("money", 3010)
    u.magic = "tybied"
    u.aura = 200
    u.ship = s1
    local err = u:add_spell("create_roi")
    assert_equal(0, err)
    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe einen Ring der Unsichtbarkeit' ")
    process_orders()
    write_reports()
    assert_equal(1, u:get_item("roi"))
end
