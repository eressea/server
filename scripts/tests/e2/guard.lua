local tcname = 'tests.e2.guard'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
end

function test_guard_unarmed()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human", "hodor@eressea.de", "de")
    local u1 = unit.create(f1, r1, 1)
    assert_equal(false, u1.guard)
    u1:clear_orders()
    u1:add_order("BEWACHE")
    process_orders()
    assert_equal(false, u1.guard)
end

function test_guard_armed()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human", "hodor@eressea.de", "de")
    local u1 = unit.create(f1, r1, 1)
    assert_equal(false, u1.guard)
    u1:add_item("sword", 1)
    u1:set_skill("melee", 2)
    u1:clear_orders()
    u1:add_order("BEWACHE")
    process_orders()
    assert_equal(true, u1.guard)
end

function test_guard_allows_move_after_combat() -- bug 1493
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f1 = faction.create("human", "bernd@eressea.de", "de")
    local u1 = unit.create(f1, r1, 10)
    local uid1 = u1.id
    local f2 = faction.create("human", "horst@eressea.de", "de")
    local u2 = unit.create(f2, r1, 1)
    u1:add_order("BEWACHE")
    u1:add_item("sword", 10)
    u1:set_skill("melee", 2)
    u1:clear_orders()
    u1:add_order("BEWACHE")
    process_orders()
    assert_equal(true, u1.guard)
    u1:clear_orders()
    u1:add_order("NACH O")
    u1:add_order("ATTACKIERE " .. itoa36(u2.id))
    process_orders()
    u1 = get_unit(uid1)
    assert_equal(r2, u1.region)
end

function test_no_guard_no_move_after_combat() -- bug 1493
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f1 = faction.create("human", "bernd@eressea.de", "de")
    local u1 = unit.create(f1, r1, 10)
    local uid1 = u1.id
    local f2 = faction.create("human", "horst@eressea.de", "de")
    local u2 = unit.create(f2, r1, 1)
    u1:add_order("BEWACHE")
    u1:add_item("sword", 10)
    u1:set_skill("melee", 2)
    assert_equal(false, u1.guard)
    u1:clear_orders()
    u1:add_order("NACH O")
    u1:add_order("ATTACKIERE " .. itoa36(u2.id))
    process_orders()
    u1 = get_unit(uid1)
    assert_equal(r1, u1.region)
end

function test_move_stops_guarding()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_item("sword", 1)
    u:set_skill("melee", 2)
    u:add_order("NACH O")
    u:add_order("BEWACHE")
    u.guard = true
    process_orders()
    assert_equal(r2, u.region)
    assert_false(u.guard)
end

function test_move_to_same_region_stops_guarding()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_item("horse", 1)
    u:add_item("sword", 1)
    u:set_skill("melee", 2)
    u:set_skill("riding", 2)
    u:add_order("NACH O W")
    u.guard = true
    process_orders()
    assert_equal(r1, u.region)
    assert_false(u.guard)
end

function test_mover_cannot_start_guarding()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_item("horse", 1)
    u:add_item("sword", 1)
    u:set_skill("melee", 2)
    u:set_skill("riding", 2)
    u:add_order("BEWACHE")
    u:add_order("NACH O")
    process_orders()
    assert_equal(r2, u.region)
    assert_false(u.guard)
end

function test_move_to_same_region_stops_guarding()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "test@example.com", "de")
    local u = unit.create(f, r1, 1)
    u:add_item("horse", 1)
    u:add_item("sword", 1)
    u:set_skill("melee", 2)
    u:set_skill("riding", 2)
    u:add_order("BEWACHE")
    u:add_order("NACH O W")
    process_orders()
    assert_equal(r1, u.region)
    assert_false(u.guard)
end
