require "lunit"

module("tests.undead", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.encounters", "0")
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
    assert_equal(2, u1.number)
    assert_equal(1, u2.number)
end

function test_give_self_undead_fail()
    -- disallow giving basic undead units
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 2, "undead")
    local u2 = unit.create(f, r, 1, "undead")
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    process_orders()
    assert_equal(2, u1.number)
    assert_equal(1, u2.number)
end

function test_give_other_zombie_fail()
    -- cannot give undead units to another faction
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("human")
    local f2 = faction.create("human")

    local u1 = unit.create(f1, r, 2, "zombie")
    local u2 = unit.create(f2, r, 1, "zombie")
    u2:add_order("KONTAKTIERE " .. itoa36(u1.id))
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    -- TODO: Migranten blockieren das derzeit, nicht Untoten-Eigenschaften
    process_orders()
    assert_equal(2, u1.number)
    assert_equal(1, u2.number)
end

function test_give_self_zombie_okay()
    -- allow giving zombie units to own units of same race
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 2, "zombie")
    local u2 = unit.create(f, r, 1, "zombie")
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end

function test_give_self_skeleton_okay()
    -- allow giving skeleton units to own units of same race
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 2, "skeleton")
    local u2 = unit.create(f, r, 1, "skeleton")
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end

function test_give_self_ghoul_okay()
    -- allow giving ghoul units to own units of same race
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 2, "ghoul")
    local u2 = unit.create(f, r, 1, "ghoul")
    u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSON")
    process_orders()
    assert_equal(1, u1.number)
    assert_equal(2, u2.number)
end
