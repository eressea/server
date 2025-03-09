local tcname = 'tests.e2.astral'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("rules.peasants.growth.factor", "0")
    eressea.settings.set("magic.resist.enable", "0")
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("magic.regeneration.enable", "0")
end

function test_real_to_astral()
    local r = region.create(0, 0, 'plain')
    local ra = r:get_astral('fog')
    assert_equal(ra, region.create(1, 0, 'plain'):get_astral('fog'))
    assert_equal(ra, region.create(2, 0, 'plain'):get_astral('fog'))
    assert_equal(ra, region.create(0, 1, 'plain'):get_astral('fog'))
    assert_equal(ra, region.create(0, 2, 'plain'):get_astral('fog'))
    assert_equal(ra, region.create(2, 2, 'plain'):get_astral('fog'))
    assert_equal(ra, region.create(3, 3, 'plain'):get_astral('fog'))
    assert_not_equal(ra, region.create(4, 4, 'plain'):get_astral('fog'))

    local rb = region.create(-1, 0, 'plain'):get_astral('fog')
    assert_not_equal(ra, rb)
    assert_equal(rb.y, ra.y)
    assert_equal(rb.x + 1, ra.x)
    r = region.create(0, -1, 'plain')
    assert_not_equal(rb, r:get_astral('fog'))
    rb = r:get_astral('fog')
    assert_not_equal(ra, rb)
    assert_equal(rb.y + 1, ra.y)
    assert_equal(rb.x, ra.x)
end

function test_fetch_astral()
    local r = region.create(0, 0, "plain")
    local ra = r:get_astral('fog')
    local rb = region.create(ra.x + 1, ra.y, 'fog')
    local f = faction.create("human");
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    local u3 = unit.create(f, rb, 1)
    rb.plane = ra.plane
    u1.magic = "gray"
    u1:set_skill("magic", 6)
    u1.aura = 0
    u1:add_spell("fetch_astral")

    u1:clear_orders()
    u1:add_order("ZAUBERE Ruf~der~Realitaet " .. itoa36(u2.id) .. " " .. itoa36(u3.id))
    process_orders()
    assert_equal(1, f:count_msg_type('missing_components_list'), 'no components')

    u1.aura = 12 -- 2 Aura pro Stufe
    process_orders()
    assert_equal(12, u1.aura)
    assert_equal(1, f:count_msg_type('spellfail_astralonly'), 'astral space')

    u2.region = ra
    process_orders()
    assert_equal(0, u1.aura)
    assert_equal(r, u2.region)
    assert_equal(rb, u3.region)
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

    -- no region
    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf " .. itoa36(u2.id))
    process_orders()
    assert_equal(1, f:count_msg_type('error209'), 'syntax error')
    u1:clear_orders()

    -- no such region
    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf 0 1 " .. itoa36(u2.id))
    process_orders()
    assert_equal(0, f:count_msg_type('error209'), 'syntax error')
    assert_equal(1, f:count_msg_type('error194'), 'target region')

    -- no aura
    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf 0 0 " .. itoa36(u2.id))
    u1.aura = 0 -- missing components
    process_orders()
    assert_equal(0, f:count_msg_type('error194'), 'target region')
    assert_equal(1, f:count_msg_type('missing_components_list'), 'no components')

    -- not in astral space
    u1.aura = 12 -- 2 Aura pro Stufe
    process_orders()
    assert_equal(1, f:count_msg_type('spellfail_astralonly'), 'astral space')

    -- success
    u1.region = u1.region:get_astral('fog')
    assert_equal('fog', u1.region.terrain)
    process_orders()
    assert_equal(0, f:count_msg_type('spellfail_astralonly'), 'astral space')
    assert_equal(1, f:count_msg_type('send_astral'), 'astral space')
    assert_equal(u1.region, u2.region)

    -- no connection
    u1.aura = 12 -- 2 Aura pro Stufe
    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf -1 0 " .. itoa36(u2.id))
    r = region.create(-1, 0, "plain")
    u2.region = r
    assert_not_equal(u1.region, u2.region:get_astral('fog')) -- unerreichbar
    process_orders()

    assert_equal(r, u2.region)
    assert_equal(0, f:count_msg_type('send_astral'), 'astral space')
    assert_equal(1, f:count_msg_type('spellfail_distance'), 'out of range')

    -- success
    u1.aura = 12 -- 2 Aura pro Stufe
    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf 1 0 " .. itoa36(u2.id))
    r = region.create(1, 0, "plain")
    u2.region = r
    assert_equal(u1.region, u2.region:get_astral('fog'))
    process_orders()

    assert_equal(u1.region, u2.region)
    assert_equal(1, f:count_msg_type('send_astral'), 'astral space')
    assert_equal(0, f:count_msg_type('spellfail::nocontact'), 'out of range')
    assert_equal(0, f:count_msg_type('spellfail_distance'), 'out of range')


    -- connection to weird diamond area
    u1.aura = 12 -- 2 Aura pro Stufe
    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf 3 3 " .. itoa36(u2.id))
    r = region.create(3, 3, "plain")
    u2.region = r
    assert_equal(u1.region, u2.region:get_astral('fog'))
    process_orders()

    assert_equal(u1.region, u2.region)
    assert_equal(1, f:count_msg_type('send_astral'), 'astral space')
    assert_equal(0, f:count_msg_type('spellfail::nocontact'), 'out of range')

    -- no connection
    u1.aura = 12 -- 2 Aura pro Stufe
    u1:clear_orders()
    u1:add_order("ZAUBERE Astraler~Ruf 4 0 " .. itoa36(u2.id))
    r = region.create(4, 0, "plain")
    u2.region = r
    process_orders()
    assert_equal(r, u2.region)
    assert_equal(0, f:count_msg_type('send_astral'), 'astral space')
    assert_equal(1, f:count_msg_type('spellfail_distance'), 'out of range')
end
