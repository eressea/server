require "lunit"

module("tests.e3.production", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4") -- food is free
    eressea.settings.set("NewbieImmunity", "0")
end

local function create_faction(race)
    return faction.create(race, race .. '@example.com', "de")
end

function test_greatbow_needs_elf()
-- only elves can build a greatbow
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    u:set_skill('weaponsmithing', 5)
    u:add_item('mallorn', 2)
    u:add_order("MACHE Elfenbogen")
    turn_process() -- humans cannot do it
    assert_equal(1, f:count_msg_type("error118"))
    assert_equal(0, u:get_item('greatbow'))
    assert_equal(2, u:get_item('mallorn'))

    u.race = 'elf'
    turn_process() -- but elves can
    assert_equal(1, u:get_item('greatbow'))
    assert_equal(0, u:get_item('mallorn'))
end

function test_troll_no_quarrying_bonus()
-- Trolle kriegen keinen Rohstoffbonus wie in E2
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('troll')
    local u = unit.create(f, r, 1)

    turn_begin()
    r:set_resource("stone", 100)
    u:set_skill('quarrying', 2) -- +2 Rassenbonus
    u:add_order("MACHE Steine")
    turn_process()
    assert_equal(4, u:get_item('stone'))
    assert_equal(96, r:get_resource('stone'))
end

function test_dwarf_no_mining_bonus()
-- E3: Zwerge verlieren den Eisenabbaubonus
    local r = region.create(0, 0, 'mountain')
    local f = create_faction('dwarf')
    local u = unit.create(f, r, 1)

    turn_begin()
    r:set_resource('iron', 100)
    u:set_skill('mining', 8) -- +2 skill bonus
    u:add_order('MACHE Eisen')
    turn_process()
    assert_equal(10, u:get_item('iron'))
    assert_equal(90, r:get_resource('iron'))
end

function test_dwarf_towershield()
-- Zwerge können als einzige Rasse Turmschilde, Schuppenpanzer 
-- und Repetierarmbrüste bauen.
    local r = region.create(0, 0, 'plain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    u:set_skill('armorer', 4)
    u:add_item('iron', 1)
    u:add_order("MACHE Turmschild")
    turn_process() -- humans cannot do it
    assert_equal(1, f:count_msg_type("error118"))
    assert_equal(0, u:get_item('towershield'))
    assert_equal(1, u:get_item('iron'))

    u.race = 'dwarf'
    u:set_skill('armorer', 2) -- dwarf has bonus +2
    turn_process() -- but dwarves can
    assert_equal(1, u:get_item('towershield'))
    assert_equal(0, u:get_item('iron'))
    
end

function test_dwarf_scale()
-- Zwerge können als einzige Rasse Turmschilde, Schuppenpanzer 
-- und Repetierarmbrüste bauen.
    local r = region.create(0, 0, 'plain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    u:set_skill('armorer', 5)
    u:add_item('iron', 2)
    u:add_order("MACHE Schuppenpanzer")
    turn_process() -- humans cannot do it
    assert_equal(1, f:count_msg_type("error118"))
    assert_equal(0, u:get_item('scale'))
    assert_equal(2, u:get_item('iron'))

    u.race = 'dwarf'
    u:set_skill('armorer', 3) -- dwarf has bonus +2
    turn_process() -- but dwarves can
    assert_equal(1, u:get_item('scale'))
    assert_equal(0, u:get_item('iron'))
end

function test_dwarf_rep_xbow()
-- Zwerge können als einzige Rasse Turmschilde, Schuppenpanzer 
-- und Repetierarmbrüste bauen.
    local r = region.create(0, 0, 'plain')
    local f = create_faction('human')
    local u = unit.create(f, r, 1)

    turn_begin()
    u:set_skill('weaponsmithing', 5)
    u:add_item('iron', 1)
    u:add_item('log', 1)
    u:add_order("MACHE Repetierarmbrust")
    turn_process() -- humans cannot do it
    assert_equal(1, f:count_msg_type("error118"))
    assert_equal(0, u:get_item('rep_crossbow'))
    assert_equal(1, u:get_item('iron'))
    assert_equal(1, u:get_item('log'))

    u.race = 'dwarf'
    u:set_skill('weaponsmithing', 3) -- dwarf has bonus +2
    turn_process() -- but dwarves can
    assert_equal(1, u:get_item('rep_crossbow'))
    assert_equal(0, u:get_item('iron'))
    assert_equal(0, u:get_item('log'))
end
