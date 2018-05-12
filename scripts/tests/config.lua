require "lunit"

module("tests.eressea.config", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
end

function test_read_race()
    local f
    assert_not_nil(eressea.config)
    eressea.config.parse('{ "races": { "orc" : {}}}')
    f = faction.create("orc", "orc@example.com", "en")
    assert_not_nil(f)
end

function test_seed_unit()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = unit.create(f, r, 1)
    u:equip('seed_unit')
    assert_equal(20000, u:get_item('money'))
    assert_equal(50, u:get_item('log'))
    assert_equal(50, u:get_item('stone'))
    assert_equal(1, u:get_skill('melee'))
end

function test_seed_elf()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = unit.create(f, r, 1)
    -- quirk: independent of the race, seed_elf contains a fairyboot
    u:equip('seed_elf')
    assert_equal(1, u:get_item('fairyboot'))
    -- all humans start in a building:
    assert_not_nil(u.building)
    assert_equal('castle', u.building.type)
    assert_equal(10, u.building.size)
end
