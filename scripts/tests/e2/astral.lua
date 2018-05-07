require "lunit"

module("tests.e2.astral", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.peasants.growth.factor", "0")
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("magic.regeneration.enable", "0")
end

local function dump_messages(f)
    for k, v in ipairs(f.messages) do
        print(v)
    end
end

function test_fetch_astral()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u1.magic = "gray"
    u1:set_skill("magic", 6)
    u1.aura = 0
    u1:add_spell("fetch_astral")

    u1:clear_orders()
    u1:add_order("ZAUBERE Ruf~der~Realitaet " .. itoa36(u2.id))
    process_orders()
    assert_equal(1, f:count_msg_type('missing_components_list'), 'no components')

    u1.aura = 12 -- 2 Aura pro Stufe
    process_orders()
    assert_equal(12, u1.aura)
    assert_equal(1, f:count_msg_type('spellfail_astralonly'), 'astral space')

    u2.region = u2.region:get_astral('fog')
    process_orders()
    assert_equal(0, u1.aura)
    assert_equal(u1.region, u2.region)
end

function test_pull_astral()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u1.magic = "gray"
    u1:set_skill("magic", 6)
    u1.aura = 0
    u1:add_spell("pull_astral")

    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf " .. itoa36(u2.id))
    process_orders()
    assert_equal(1, f:count_msg_type('error209'), 'syntax error')
    u1:clear_orders()

    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf 1 0 " .. itoa36(u2.id))
    process_orders()
    assert_equal(0, f:count_msg_type('error209'), 'syntax error')
    assert_equal(1, f:count_msg_type('error194'), 'target region')

    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf 0 0 " .. itoa36(u2.id))
    u1.aura = 0 -- missing components
    process_orders()
    assert_equal(0, f:count_msg_type('error194'), 'target region')
    assert_equal(1, f:count_msg_type('missing_components_list'), 'no components')

    u1.aura = 12 -- 2 Aura pro Stufe
    process_orders()
    assert_equal(1, f:count_msg_type('spellfail_astralonly'), 'astral space')

    u1.region = u1.region:get_astral('fog')
    assert_equal('fog', u1.region.terrain)
    process_orders()
    assert_equal(0, f:count_msg_type('spellfail_astralonly'), 'astral space')
    assert_equal(1, f:count_msg_type('send_astral'), 'astral space')

    assert_equal(u1.region, u2.region)
end

