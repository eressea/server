local tcname = 'tests.e2.undead'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.peasants.growth", "1")
    eressea.settings.set("study.random_progress", "0")
    eressea.settings.set("GiveRestriction", "0")
end

function test_give_undead_to_self()
    -- generic undead cannot be given
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 2, "undead")
    local u2 = unit.create(f, r, 1, "undead")
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end

function test_give_self_clone_fail()
    -- disallow giving basic undead units
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 2, "clone")
    local u2 = unit.create(f, r, 1, "clone")
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    process_orders()
    assert_equal(2, u1.number)
    assert_equal(1, u2.number)
end

local function setup_give_self(race)
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 2, race)
    local u2 = unit.create(f, r, 1, race)
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    return u1, u2
end

local function setup_give_other(race)
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("human")
    local f2 = faction.create("human")
    local u1 = unit.create(f1, r, 2, race)
    local u2 = unit.create(f2, r, 1, race)
    u2:add_order("KONTAKTIERE " .. itoa36(u1.id))
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    return u1, u2
end

function test_give_other_zombie_fail()
    u1, u2 = setup_give_other("zombie")
    process_orders()
    assert_equal(2, u1.number)
    assert_equal(1, u2.number)
end

function test_give_self_zombie_okay()
    -- allow giving zombie units to own units of same race
    u1, u2 = setup_give_self("zombie")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end

function test_give_self_skeleton_okay()
    -- allow giving skeleton units to own units of same race
    u1, u2 = setup_give_self("skeleton")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end

function test_give_self_ghoul_okay()
    -- allow giving ghoul units to own units of same race
    u1, u2 = setup_give_self("ghoul")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end

function test_give_self_ghast_okay()
    -- allow giving ghast units to own units of same race
    u1, u2 = setup_give_self("ghast")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end

function test_give_self_juju_okay()
    -- allow giving juju units to own units of same race
    u1, u2 = setup_give_self("juju")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end

function test_give_self_skeletonlord_okay()
    -- allow giving skeletonlord units to own units of same race
    u1, u2 = setup_give_self("skeletonlord")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end
function test_undead_reserve_self()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human")
    local u1 = unit.create(f1, r1, 1)
    u1.race = "undead"
    u1:clear_orders()
    u1:add_item("log", 2)
    u1:add_order("RESERVIERE 1 Holz")
    u1:add_order("GIB 0 ALLES Holz")
    process_orders()
    assert_equal(1, u1:get_item("log"))
end

function test_undead_reserve_other()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human")
    local u1 = unit.create(f1, r1, 1)
    local u2 = unit.create(f1, r1, 1)
    u2:add_item("log", 2)
    u1.race = "undead"
    u1:clear_orders()
    u1:add_order("RESERVIERE 1 Holz")
    process_orders()

    if 0 ~= u1:get_item("log") then
        -- try to catch that intermittent bug:
        print(u1:show())
    end
    assert_equal(0, u1:get_item("log"))
    if 2 ~= u2:get_item("log") then
        -- try to catch that intermittent bug:
        print(u2:show())
    end
    assert_equal(2, u2:get_item("log"))
end

function test_undead_give_item()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human", "hodor@eressea.de", "de")
    local u1 = unit.create(f1, r1, 1)
    u1.race = "undead"
    u1:clear_orders()
    u1:add_item("log", 1)
    u1:add_order("GIB 0 1 Holz")
    process_orders()
    assert_equal(0, u1:get_item("log"))
end

function test_clones_dont_give_person()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human", "hodor@eressea.de", "de")
    local u1 = unit.create(f1, r1, 2)
    u1.race = "clone"
    u1:clear_orders()
    u1:add_item("log", 1)
    u1:add_order("GIB 0 1 Person")
    process_orders()
    assert_equal(2, u1.number)
end

-- bug 2504
function test_skeleton_cannot_learn()
    local r = region.create(0, 0, "plain")
    local f = faction.create("elf")
    local u = unit.create(f, r, 2)
    u.race = "skeleton"
    u:add_order("LERNE Wahrnehmung")
    process_orders()
    assert_equal(0, u:get_skill('perception'))
end
