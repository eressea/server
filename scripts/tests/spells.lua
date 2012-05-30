require "lunit"

module("tests.spells", package.seeall, lunit.testcase)

function setup()
    free_game()
    settings.set("magic.fumble.enable", "0")
    settings.set("magic.regeneration.enable", "0")
    settings.set("rules.economy.food", "4")
end

function test_spell_payment()
    local r = region.create(0,0, "plain")
    local f = faction.create("spell_payment@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:set_skill("magic", 12)
    u.magic = "gwyrrd" -- calls create_mage
    u:add_spell("create_roi")

    local permaura = u:get_pooled("permaura")
    u:add_item("money", 3000)
    u.aura = 50
    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe einen Ring der Unsichtbarkeit' ")
    process_orders()

    assert_equal(1, u:get_item("roi"))
    assert_equal(0, u:get_item("money"))
    assert_equal(0, u.aura)
    assert_equal(permaura-1, u:get_pooled("permaura"))
end

function test_spell_not_found_no_payment()
    local r = region.create(0,0, "plain")
    local f = faction.create("spell_fails_no_payment@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 12)

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

function test_create_firesword()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_firesword@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 12)
    u:add_spell("create_firesword")

    u:cast_spell('create_firesword', 1)
    assert_equal(1, u:get_item("firesword"))
end

function test_create_roi()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_roi@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_roi')
    u:cast_spell('create_roi')
    assert_equal(1, u:get_item("roi"))
end

function test_create_roqf()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_roqf@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_roqf')
    u:cast_spell('create_roqf')
    assert_equal(1, u:get_item("roqf"))
end

function test_create_aots()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_aots@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_aots')
    u:cast_spell('create_aots')
    assert_equal(1, u:get_item("aots"))
end

function test_create_ror()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_ror@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_ror')
    u:cast_spell('create_ror')
    assert_equal(1, u:get_item("ror"))
end

function test_create_magicherbbag()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_magicherbbag@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_magicherbbag')
    u:cast_spell('create_magicherbbag')
    assert_equal(1, u:get_item("magicherbbag"))
end

function test_create_trollbelt()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_trollbelt@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_trollbelt')
    u:cast_spell('create_trollbelt')
    assert_equal(1, u:get_item("trollbelt"))
end

function test_create_dreameye()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_dreameye@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_dreameye')
    u:cast_spell('create_dreameye')
    assert_equal(1, u:get_item("dreameye"))
end

function test_create_antimagic()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_antimagic@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_antimagic')
    u:cast_spell('create_antimagic')
    assert_equal(1, u:get_item("antimagic"))
end

function test_create_runesword()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_runesword@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gwyrrd" -- calls create_mage
    u:set_skill("magic", 10)

    u:add_spell('create_runesword')
    u:cast_spell('create_runesword')
    assert_equal(1, u:get_item("runesword"))
end

function test_create_rop()
    local r = region.create(0,0, "plain")
    local f = faction.create("create_rop@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:set_skill("magic", 10)

    u:add_spell('create_rop')
    u:cast_spell('create_rop')
    assert_equal(1, u:get_item("rop"))
end
