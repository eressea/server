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

function test_appeasement_can_move()
    local u1, u2, r1, r2, uno
    r1 = region.create(0, 0, 'plain')
    r2 = region.create(1, 0, 'plain')
    u2 = unit.create(faction.create('human'), r1, 1)
    u2.race = 'elf'
    u2.name = 'Angsthase'
    u2.magic = 'gwyrrd'
    u2:set_skill('magic', 5)
    u2.aura = 10
    u2:add_spell('appeasement')
    u2:add_order('NACH O')
    u2:add_order('KAMPFZAUBER STUFE 1 Friedenslied')
    uno = u2.id
    u1 = unit.create(faction.create('human'), r1, 1)
    u1:set_skill('polearm', 5)
    u1:add_order('ATTACKIERE ' .. itoa36(uno))
    process_orders()
    u2 = get_unit(uno)
    assert_not_nil(u2)
    assert_equal(r2, u2.region)
    assert_equal(5, u2.status)
end

function test_appeasement_break_guard()
    local u1, u2, r1, r2, uno
    r1 = region.create(0, 0, 'plain')
    r2 = region.create(1, 0, 'plain')
    u2 = unit.create(faction.create('human'), r1, 1)
    u2.race = 'elf'
    u2.name = 'Angsthase'
    u2.magic = 'gwyrrd'
    u2.guard = true
    u2.status = 1
    u2:set_skill('magic', 5)
    u2.aura = 10
    u2:add_spell('appeasement')
    u2:add_order('BEWACHE')
    u2:add_order('KAMPFZAUBER STUFE 1 Friedenslied')
    uno = u2.id
    u1 = unit.create(faction.create('human'), r1, 1)
    u1:set_skill('polearm', 5)
    u1:add_order('ATTACKIERE ' .. itoa36(uno))
    process_orders()
    u2 = get_unit(uno)
    assert_not_nil(u2)
    assert_equal(r1, u2.region)
    assert_equal(5, u2.status)
    assert_equal(false, u2.guard)
end
