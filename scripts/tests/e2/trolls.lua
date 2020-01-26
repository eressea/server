require "lunit"

module("tests.e2.trolls", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
end

function test_trolls_with_horses()
    local r0 = region.create(0, 0, 'plain')
    local r1 = region.create(1, 0, 'plain')
    local r2 = region.create(2, 0, 'plain')
    local r3 = region.create(3, 0, 'plain')
    local r4 = region.create(4, 0, 'plain')
    local r5 = region.create(5, 0, 'plain')

    local f = faction.create('troll')
    -- 1. 20 trolls can pull 5 loaded carts:
    assert_not_nil(r0)
    local u1 = unit.create(f, r0, 20)
    u1:add_item('cart', 5)
    -- trolls carry 10.8 GE, carts carry 100 GE:
    u1:add_item('money', 100 * (5 * 100 + 2 * 108))
    u1:add_order('NACH O O O')

    process_orders()
    assert_equal(r1, u1.region)

    --  20 trolls can also lead 20 horses
    u1:add_item('horse', 20)
    u1:add_item('money', 100 * 20 * 20)
    
    process_orders()
    assert_equal(r2, u1.region)

  -- test if trolls are still "lazy". If yes they should still manage 10 full carts behind the 20 horses (5 more)
    u1:add_item('cart', 5)
    u1:add_item('money', 100 * 5 * 100)
 
    process_orders()
    assert_equal(r3, u1.region)

  -- test if trolls are still "lazy". If not they should manage 15 full carts, 5 behind trolls and 10 behind 20 horses (again 5 more)
    u1:add_item('cart', 5)
    u1:add_item('money', 100 * 5 * 100)
 
    process_orders()
    assert_equal(r4, u1.region)

end
