require "lunit"

module("tests.undead", package.seeall, lunit.testcase)

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
