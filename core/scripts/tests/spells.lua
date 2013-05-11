require "lunit"

module("tests.eressea.spells", package.seeall, lunit.testcase)

local r, f, u

function setup()
    eressea.free_game()
    eressea.settings.set("magic.regeneration.enable", "0")
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("rules.economy.food", "4")

    r = region.create(0, 0, "plain")
    f = faction.create("spell_payment@eressea.de", "elf", "de")
    u = unit.create(f, r, 1)
    u.magic = "gray"
    u:set_skill("magic", 12)
end

function test_spell_payment()
    u:add_spell("create_roi")

    local permaura = u:get_pooled("permaura")
    u:add_item("money", 3000)
    u.aura = 50
    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe einen Ring der Unsichtbarkeit' ")
    process_orders()
    write_reports()
    assert_equal(1, u:get_item("roi"))
    assert_equal(0, u:get_item("money"))
    assert_equal(0, u.aura)
    assert_equal(permaura-1, u:get_pooled("permaura"))
end

function test_spell_not_found_no_payment()
    local permaura = u:get_pooled("permaura")
    u:add_item("money", 3000)
    u.aura = 50

    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe einen Ring der Unsichtbarkeit' ")
    process_orders()

    assert_equal(0, u:get_item("roi"))
    assert_equal(3000, u:get_item("money"))
    assert_equal(50, u.aura)
    assert_equal(permaura, u:get_pooled("permaura"))
end

function test_create_roi()
    u:add_spell('create_roi')
    u:cast_spell('create_roi')
    assert_equal(1, u:get_item("roi"))
end

function test_create_roqf()
    u:add_spell('create_roqf')
    u:cast_spell('create_roqf')
    assert_equal(1, u:get_item("roqf"))
end

function test_create_aots()
    u:add_spell('create_aots')
    u:cast_spell('create_aots')
    assert_equal(1, u:get_item("aots"))
end

function test_create_ror()
    u:add_spell('create_ror')
    u:cast_spell('create_ror')
    assert_equal(1, u:get_item("ror"))
end

function test_create_trollbelt()
    u:add_spell('create_trollbelt')
    u:cast_spell('create_trollbelt')
    assert_equal(1, u:get_item("trollbelt"))
end

function test_create_dreameye()
    u:add_spell('create_dreameye')
    u:cast_spell('create_dreameye')
    assert_equal(1, u:get_item("dreameye"))
end

function test_create_antimagic()
    u:add_spell('create_antimagic')
    u:cast_spell('create_antimagic')
    assert_equal(1, u:get_item("antimagic"))
end

function test_create_rop()
    u:add_spell('create_rop')
    u:cast_spell('create_rop')
    assert_equal(1, u:get_item("rop"))
end
