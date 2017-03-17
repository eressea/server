require "lunit"

module("tests.e3.spells-e2", package.seeall, lunit.testcase)

local r, f, u

function setup()
    eressea.free_game()
    eressea.settings.set("magic.regeneration.enable", "0")
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("rules.food.flags", "4")

    r = region.create(0, 0, "plain")
    f = faction.create("elf", "spell_payment@eressea.de", "de")
    u = unit.create(f, r, 1)
    u.magic = "gray"
    u:set_skill("magic", 12)
end

function test_create_magicherbbag()
    u:add_spell('create_magicherbbag')
    u:cast_spell('create_magicherbbag')
    assert_equal(1, u:get_item("magicherbbag"))
end

function test_create_runesword()
    u:add_spell('create_runesword')
    u:cast_spell('create_runesword')
    assert_equal(1, u:get_item("runesword"))
end

function test_create_firesword()
    u:add_spell("create_firesword")
    u:cast_spell('create_firesword', 1)
    assert_equal(1, u:get_item("firesword"))
end

