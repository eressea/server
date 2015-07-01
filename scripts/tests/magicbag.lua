require "lunit"

module("tests.magicbag", package.seeall, lunit.testcase)

local u

function setup()
    eressea.free_game()
    u = unit.create(faction.create("test@example.com", "human", "de"), region.create(0, 0, "plain"), 1)
end

function test_magicbag_weight()
    assert_equal(1000, u.weight)
    u:add_item("log", 10)
    assert_equal(6000, u.weight)
    u:add_item("magicbag", 1)
    assert_equal(1100, u.weight)
end

function test_magicbag_no_stone()
    assert_equal(1000, u.weight)
    u:add_item("stone", 1)
    assert_equal(7000, u.weight)
    u:add_item("magicbag", 1)
    assert_equal(7100, u.weight)
end

function test_magicbag_no_carts()
    assert_equal(1000, u.weight)
    u:add_item("cart", 1)
    assert_equal(5000, u.weight)
    u:add_item("magicbag", 1)
    assert_equal(5100, u.weight)
end

function test_magicbag_no_catapult()
    assert_equal(1000, u.weight)
    u:add_item("catapult", 1)
    assert_equal(11000, u.weight)
    u:add_item("magicbag", 1)
    assert_equal(11100, u.weight)
end

function test_magicbag_no_horses()
    assert_equal(1000, u.weight)
    u:add_item("horse", 1)
    assert_equal(6000, u.weight)
    u:add_item("magicbag", 1)
    assert_equal(6100, u.weight)
end
