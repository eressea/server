local tcname = 'tests.e2.carts'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
end

function test_walk()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "pirate@eressea.de", "de")
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r1, 2)
    local u3 = unit.create(f, r1, 1)
    u1:add_item("money", 540)
    u1:add_order("NACH O")
    u2:add_item("money", 1080)
    u2:add_order("NACH O")
    u3:add_item("money", 541)
    u3:add_order("NACH O")
    process_orders()
    assert_equal(r2, u1.region)
    assert_equal(r2, u2.region)
    assert_equal(r1, u3.region)
end

function test_lead_horses()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local r3 = region.create(2, 0, "plain")
    local f = faction.create("human")
    -- walk (don't ride) a horse to the next region:
    local u1 = unit.create(f, r1, 1)
    u1:add_item("horse", 1)
    u1:add_item("money", 2540)
    u1:add_order("NACH O O")
    -- too heavy to move:
    local u2 = unit.create(f, r1, 1)
    u2:add_item("horse", 1)
    u2:add_item("money", 2541)
    u2:add_order("NACH O O")
    -- too many horses:
    local u3 = unit.create(f, r1, 1)
    u3:add_item("horse", 2)
    u3:add_item("money", 2540)
    u3:add_order("NACH O O")
    -- riders can lead 4 extra horses per level:
    local u4 = unit.create(f, r1, 1)
    u4:set_skill("riding", 1)
    u4:add_item("horse", 5)
    u4:add_item("money", 540+2000*5)
    u4:add_order("NACH O O")

    process_orders()
    assert_equal(r2, u1.region)
    assert_equal(r1, u2.region)
    assert_equal(r1, u3.region)
    assert_equal(r2, u4.region)
end

function test_ride_horses()
    local r0 = region.create(0, 0, "plain")
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    local r3 = region.create(3, 0, "plain")
    local f = faction.create("human")
    -- ride a horse two regions:
    local u1 = unit.create(f, r0, 1)
    u1:set_skill("riding", 1)
    u1:add_item("horse", 1)
    u1:add_item("money", 1000)
    u1:add_order("NACH O O O")
    -- too heavy to ride, walk the horse:
    local u2 = unit.create(f, r0, 1)
    u2:set_skill("riding", 1)
    u2:add_item("horse", 1)
    u2:add_item("money", 2001)
    u2:add_order("NACH O O O")

    process_orders()
    -- mit 20 GE beladenes Pferd kommt 2 Regionen weit:
    assert_equal(r2, u1.region)
    -- überladenes Pferd kommt nur eine Region weit, zu Fuss:
    assert_equal(r1, u2.region)
end

function test_rider_leads_horses()
    local r0 = region.create(0, 0, "plain")
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    local r3 = region.create(3, 0, "plain")
    local f = faction.create("human")
    -- lead 1 extra horse per level while riding:
    local u1 = unit.create(f, r0, 1)
    u1:set_skill("riding", 1)
    u1:add_item("horse", 2)
    u1:add_order("NACH O O O")
    -- too heavy to ride, walk the horses:
    local u2 = unit.create(f, r0, 1)
    u2:set_skill("riding", 1)
    u2:add_item("horse", 2)
    u2:add_item("money", 2000 * 2)
    u2:add_order("NACH O O")
    -- too many horses, but can walk:
    local u3 = unit.create(f, r0, 1)
    u3:set_skill("riding", 1)
    u3:add_item("horse", 5)
    u3:add_order("NACH O O")

    process_orders()
    assert_equal(r2, u1.region)
    assert_equal(r1, u2.region)
    assert_equal(r1, u3.region)
end

function test_carts()
    local r0 = region.create(0, 0, "plain")
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    local r3 = region.create(3, 0, "plain")
    local f = faction.create("human")
    -- 1. two walkers, each with two horses and a cart:
    local u1 = unit.create(f, r0, 2)
    u1:add_item("horse", 2)
    u1:add_item("cart", 1)
    u1:add_order("NACH O O O")
    -- 2. two riders, each with two horses and a cart:
    local u2 = unit.create(f, r0, 2)
    u2:set_skill("riding", 1)
    u2:add_item("horse", 4)
    u2:add_item("cart", 2)
    u2:add_order("NACH O O O")
    -- 2. two riders, each with five horses, and max carts:
    local u3 = unit.create(f, r0, 2)
    u3:set_skill("riding", 1)
    u3:add_item("horse", 10)
    u3:add_item("cart", 5)
    u3:add_order("NACH O O O")

    process_orders()
    assert_equal(r1, u1.region)
    assert_equal(r2, u2.region)
    assert_equal(r1, u3.region)
end

function test_walking_carts()
    local r0 = region.create(0, 0, "plain")
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    local r3 = region.create(3, 0, "plain")
    local f = faction.create("human")
    -- 1. ten riders walk with 50 horses and 25 carts, carry 3554 GE:
    local u1 = unit.create(f, r0, 10)
    u1:set_skill("riding", 1)
    u1:add_item("horse", 50)
    u1:add_item("cart", 25)
    u1:add_item("money", 355400)
    u1:add_order("NACH O O O")

    process_orders()
    assert_equal(r1, u1.region)
end

function test_trolls_pull_carts()
    local r0 = region.create(0, 0, "plain")
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    local r3 = region.create(3, 0, "plain")
    local f = faction.create("troll")
    -- 1. 20 trolls can pull 5 loaded carts:
    local u1 = unit.create(f, r0, 20)
    u1:add_item("cart", 5)
    -- trolls carry 10.8 GE, carts carry 100 GE:
    u1:add_item("money", 100 * (5 * 100 + 2 * 108))
    u1:add_order("NACH O O O")

    process_orders()
    assert_equal(r1, u1.region)

    u1:add_item("money", 1) -- just one wafer thin mint
    process_orders()
    assert_equal(r1, u1.region)
end


function test_trolls_ride_carts()
    local r0 = region.create(0, 0, "plain")
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    local r3 = region.create(3, 0, "plain")
    local r4 = region.create(4, 0, "plain")
    local f = faction.create("troll")
    -- 1. 20 trolls can pull 5 loaded carts:
    local u1 = unit.create(f, r0, 20)
    u1:add_item("cart", 5)
    -- but with 10 or more horses, they should ride in the cart:
    u1:set_skill("riding", 1, true)
    u1:add_item("horse", 10)
    -- trolls weigh 20 GE, horses carry 20, carts carry 100 GE:
    u1:add_item("money", 100 * (10 * 20 + 5 * 100 - u1.number * 20))

    u1:clear_orders()
    u1:add_order('NACH O O O')
    process_orders()
    assert_equal(r2, u1.region)

    u1:add_item("money", 1) -- just one wafer thin mint
    u1:clear_orders()
    u1:add_order('NACH O O O')
    process_orders()
    assert_equal(r3, u1.region) -- can still walk
end

function test_catapults_are_vehicles()
    local r0 = region.create(0, 0, "plain")
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    local f = faction.create()
    local u = unit.create(f, r0, 2)
    u:add_item("horse", 2)
    u:add_item("catapult", 1)
    u:add_item("money", 2000)
    u:set_skill("riding", 1, true)
    u:set_orders("NACH O O")
    process_orders()
    assert_equal(r2, u.region) -- can ride
    u:set_orders("NACH W W")
    u:add_item("money", 1) -- just one wafer thin mint
    process_orders()
    assert_equal(r1, u.region) -- can still walk
end

function test_trolls_pull_catapults()
    local r0 = region.create(0, 0, "plain")
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "plain")
    local f = faction.create("troll")
    local u = unit.create(f, r0, 4)
    u:add_item("catapult", 1)
    u:set_skill("riding", 1, true)
    u:set_orders("NACH O O")
    u:add_item("money", 1080 * u.number)
    process_orders()
    assert_equal(r1, u.region) -- can still walk
    u:set_orders("NACH W W")
    u:add_item("money", 1) -- just one wafer thin mint
    process_orders()
    assert_equal(r1, u.region) -- can no longer walk
end
