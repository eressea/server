require "lunit"

module("tests.e2.items", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.ship.storms", "0")
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("magic.regeneration.enable", "0")
end

function test_water_of_life()
    local r = region.create(0, 0, "plain")
    r:set_flag(1, false) -- no mallorn
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    local trees = r:get_resource('sapling')
    u:add_item('p2', 1)
    u:add_item('log', 10)
    u:add_order("BENUTZE 1 'Wasser des Lebens'")
    process_orders()
    assert_equal(0, u:get_item('p2'))
    assert_equal(0, u:get_item('log'))
    assert_equal(trees+10, r:get_resource('sapling'))
end

function test_nestwarmth_insect()
    local r = region.create(0, 0, "plain")
    local f = faction.create("insect", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    local flags = u.flags
    u:add_item("nestwarmth", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Nestwaerme")
    turn_begin()
    turn_process()
    assert_equal(flags+64, u.flags) -- UFL_WARMTH
    assert_equal(1, u:get_item("nestwarmth"))
    assert_equal(1, f:count_msg_type('usepotion'))
    turn_end()
end

function test_nestwarmth_other()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    local flags = u.flags
    u:add_item("nestwarmth", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Nestwaerme")
    turn_begin()
    turn_process()
    assert_equal(flags, u.flags) -- nothing happens
    assert_equal(2, u:get_item("nestwarmth"))
    assert_equal(1, f:count_msg_type('error163'))
    turn_end()
end

function test_meow()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    u:add_item("aoc", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Katzenamulett")
    turn_begin()
    turn_process()
    assert_equal(1, u:get_item("aoc"))
    assert_equal(1, r:count_msg_type('meow'))
    turn_end()
end

function test_aurapotion50()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    u:add_item("aurapotion50", 1)
    u:set_skill('magic', 10);
    u.magic = 'gwyrrd'
    u.aura = 0
    u:clear_orders()
    u:add_order("BENUTZEN 1 Auratrank")
    turn_begin()
    turn_process()
    assert_equal(0, u:get_item("aurapotion50"))
    assert_equal(1, f:count_msg_type('aurapotion50'))
    assert_equal(50, u.aura)
    turn_end()
end

function test_bagpipe()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    turn_begin()
    u:add_item("bagpipeoffear", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Dudelsack")
    assert_equal(nil, r:get_curse('depression'))
    turn_process()
    assert_equal(0, r:get_curse('depression'))
    assert_equal(1, u:get_item("bagpipeoffear"))
    assert_equal(1, f:count_msg_type('bagpipeoffear_faction'))
    assert_equal(1, r:count_msg_type('bagpipeoffear_region'))
    turn_end()
    -- duration is variable, but at least 4
    assert_equal(0, r:get_curse('depression'))
end

function test_speedsail()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    
    turn_begin()
    u.ship = ship.create(r, "boat")
    u:add_item("speedsail", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Sonnensegel")
    assert_equal(nil, u.ship:get_curse('shipspeed'))
    turn_process()
    assert_equal(1, u.ship:get_curse('shipspeed'))
    assert_equal(1, u:get_item("speedsail"))
    assert_equal(1, f:count_msg_type('use_speedsail'))
    turn_end()
    assert_equal(1, u.ship:get_curse('shipspeed')) -- effect stays forever
end

function disable_test_foolpotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    turn_begin()
    u:add_item('p7', 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Dumpfbackenbrot 4242")
    turn_process()
    assert_equal(2, u:get_item('p7'))
    assert_equal(1, f:count_msg_type('feedback_unit_not_found'))
    local u2 = unit.create(f, r, 1)
    
    u:clear_orders()
    u:add_order("BENUTZEN 2 Dumpfbackenbrot " .. itoa36(u2.id))
    turn_process()
    assert_equal(2, u:get_item('p7'))
    assert_equal(1, f:count_msg_type('error64'))

    u:set_skill("stealth", 1)
    u2:set_skill('crossbow', 1)
    turn_process()
    assert_equal(0, u:get_item('p7'))
    assert_equal(0, u2:effect('p7'))
    assert_equal(0, u2:get_skill('crossbow'))
    assert_equal(1, f:count_msg_type('givedumb'))
    turn_end()
end

function test_snowman()
    local r = region.create(0, 0, "glacier")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    u:add_item("snowman", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Schneemann")
    process_orders()
    for u2 in r.units do
        if u2.id~=u.id then
            assert_equal("snowman", u2.race)
            assert_equal(1000, u2.hp)
            u = nil
            break
        end
    end
    assert_equal(nil, u)
end
