require "lunit"

module("tests.items", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.ship.storms", "0")
    eressea.settings.set("rules.encounters", "0")
end

function test_meow()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("aoc", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Katzenamulett")
    process_orders()
    assert_equal(1, u:get_item("aoc"))
    assert_equal(1, r:count_msg_type('meow'))
end

function test_aurapotion50()
    eressea.settings.set("magic.regeneration.enable", "0")
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("aurapotion50", 1)
    u:set_skill('magic', 10);
    u.magic = 'gwyrrd'
    u.aura = 0
    u:clear_orders()
    u:add_order("BENUTZEN 1 Auratrank")
    process_orders()
    assert_equal(0, u:get_item("aurapotion50"))
    assert_equal(1, f:count_msg_type('aurapotion50'))
    assert_equal(50, u.aura)
end

function test_bagpipe()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("bagpipeoffear", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Dudelsack")
    process_orders()
    assert_equal(1, u:get_item("bagpipeoffear"))
    assert_equal(1, f:count_msg_type('bagpipeoffear_faction'))
    assert_equal(1, r:count_msg_type('bagpipeoffear_region'))
end

function test_speedsail()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.ship = ship.create(r, "boat")
    u:add_item("speedsail", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Sonnensegel")
    process_orders()
    assert_equal(1, u:get_item("speedsail"))
    assert_equal(1, f:count_msg_type('use_speedsail'))
end

function test_skillpotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("skillpotion", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Talenttrunk")
    process_orders()
    assert_equal(1, u:get_item("skillpotion"))
    assert_equal(1, f:count_msg_type('skillpotion_use'))
end

function test_studypotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("studypotion", 2)
    u:clear_orders()
    u:add_order("LERNE Unterhaltung")
    u:add_order("BENUTZEN 1 Lerntrank")
    process_orders()
    assert_equal(1, u:get_item("studypotion"))
end

function test_antimagic()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("antimagic", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Antimagiekristall")
    process_orders()
    assert_equal(1, r:count_msg_type('use_antimagiccrystal'))
    assert_equal(1, u:get_item("antimagic"))
end

function test_ointment()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local hp = u.hp
    u.hp = 1
    u:add_item("ointment", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Wundsalbe")
    process_orders()
    assert_equal(0, u:get_item("ointment"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal(hp, u.hp)
end

function test_bloodpotion_demon()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "demon", "de")
    local u = unit.create(f, r, 1)
    u:add_item("peasantblood", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Bauernblut")
    process_orders()
    assert_equal(0, u:get_item("peasantblood"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal("demon", u.race)
end

function test_bloodpotion_other()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("peasantblood", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Bauernblut")
    process_orders()
    assert_equal(0, u:get_item("peasantblood"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal("smurf", u.race)
end

function test_foolpotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("p7", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Dumpfbackenbrot 4242")
    process_orders()
    assert_equal(1, u:get_item("p7"))
    assert_equal(1, f:count_msg_type('feedback_unit_not_found'))
    local u2 = unit.create(f, r, 1)
    
    u:clear_orders()
    u:add_order("BENUTZEN 1 Dumpfbackenbrot " .. itoa36(u2.id))
    process_orders()
    assert_equal(1, u:get_item("p7"))
    assert_equal(1, f:count_msg_type('error64'))

    u:set_skill("stealth", 1);
    process_orders()
    assert_equal(0, u:get_item("p7"))
    assert_equal(1, f:count_msg_type('givedumb'))
end

function test_snowman()
    local r = region.create(0, 0, "glacier")
    local f = faction.create("noreply@eressea.de", "human", "de")
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
