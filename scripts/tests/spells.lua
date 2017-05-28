require 'lunit'
module("tests.spells", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("magic.regeneration.enable", "0")
end

function test_create_bogus()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u.race = "elf"
    u:set_skill("magic", 10)
    u.magic = 'gwyrrd'
    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe Katastrophe'")
    process_orders()
    assert_equal(f.messages[3], 'error173') -- HACKity HACK
end

function test_create_roi()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u.race = "elf"
    u:set_skill("magic", 10)
    u.magic = 'gwyrrd'
    u.aura = 100
    u:add_item("money", 3000)
    u:add_spell("create_roi")
    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe einen Ring der Unsichtbarkeit'")
    local amax = u.aura_max
    process_orders()
    assert_equal(1, u:get_item("roi"))
    assert_equal(50, u.aura)
    assert_equal(amax - 1, u.aura_max)
end

function test_create_aots()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u.race = "elf"
    u:set_skill("magic", 10)
    u.magic = 'gwyrrd'
    u.aura = 100
    u:add_item("money", 3000)
    u:add_spell("create_aots")
    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe ein Amulett des wahren Sehens'")
    local amax = u.aura_max
    process_orders()
    assert_equal(1, u:get_item("aots"))
    assert_equal(50, u.aura)
    assert_equal(amax - 1, u.aura_max)
end

function test_create_dreameye()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u.race = "elf"
    u:set_skill("magic", 10)
    u.magic = 'gwyrrd'
    u.aura = 100
    u:add_item("dragonhead", 1)
    u:add_spell("create_dreameye")
    u:clear_orders()
    u:add_order("ZAUBERE 'Erschaffe ein Traumauge'")
    local amax = u.aura_max
    process_orders()
    assert_equal(1, u:get_item("dreameye"))
    assert_equal(100, u.aura)
    assert_equal(amax - 5, u.aura_max)
end

