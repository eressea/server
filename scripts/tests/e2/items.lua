local tcname = 'tests.e2.items'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("magic.regeneration.enable", "0")
    eressea.settings.set("rules.grow.formula", "0")
    eressea.settings.set("study.random_progress", "0")
end

function test_use_mistletoe()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item('mistletoe', 3)
    u:add_order("BENUTZEN 2 Mistelzweig")
    process_orders()
    assert_equal(2, u:effect('mistletoe'))
    assert_equal(1, u:get_item('mistletoe'))
    assert_equal(1, f:count_msg_type('use_item'))
end

function test_mistletoe_survive()
    local r = region.create(0, 0, "plain")
    local u = unit.create(faction.create("human"), r, 1)
    local u2 = unit.create(faction.create("human"), r, 1)
    local uno = u.id
    u:add_item('mistletoe', 2)
    u:add_order("BENUTZEN 2 Mistelzweig")
    u2:add_order('ATTACKIERE ' .. itoa36(uno))
    process_orders()
    u = get_unit(uno)
    assert_not_nil(u)
    assert_equal(1, u:effect('mistletoe'))
end

function test_dreameye()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("dreameye", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Traumauge")
    assert_nil(u:get_curse('skillmod'))
    turn_begin()
    turn_process()
    assert_not_nil(u:get_curse('skillmod'))
    assert_equal(1, u:get_item("dreameye"))
    assert_equal(1, f:count_msg_type('use_tacticcrystal'))
    turn_end()
    assert_nil(u:get_curse('skillmod'))
end

function test_manacrystal()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("manacrystal", 2)
    u:clear_orders()
    u.magic = "gwyrrd"
    u:set_skill('magic', 1)
    u.aura = 0
    u:add_order("BENUTZEN 1 Astralkristall")
    turn_begin()
    turn_process()
    assert_equal(1, u:get_item("manacrystal"))
    assert_equal(25, u.aura)
    assert_equal(1, f:count_msg_type('manacrystal_use'))
    turn_end()
end

function test_skillpotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
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
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    turn_begin()
    u:add_item("studypotion", 2)
    u:clear_orders()
    u:add_order("LERNE Unterhaltung")
    u:add_order("BENUTZEN 1 Lerntrank")
    turn_process()
    -- cannot sense the "learning" attribute, because study_cmd
    -- removes it during processing :(
    assert_equal(1, u:get_item("studypotion"))
    turn_end()
end

function test_antimagic()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)

    turn_begin()
    u:add_item("antimagic", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Antimagiekristall")
    assert_equal(nil, r:get_curse('antimagiczone'))
    turn_process()
    assert_equal(5, r:get_curse('antimagiczone'))
    assert_equal(1, r:count_msg_type('use_antimagiccrystal'))
    assert_equal(1, u:get_item("antimagic"))
    turn_end()
    assert_equal(5, r:get_curse('antimagiczone')) -- haelt zwei wochen
    turn_end() -- hack: age the curse again
    assert_equal(nil, r:get_curse('antimagiczone'))
end

function test_use_healing_potion()
    -- Heiltrank kann (auch) mit BENUTZE eingesetzt werden
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 30)
    assert_equal(600, u.hp)
    u.hp = 100
    turn_begin()
    u:add_item("healing", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Heiltrank")
    turn_process()
    assert_equal(530, u.hp)
    assert_equal(0, u:get_item("healing"))
    assert_equal(1, f:count_msg_type('use_item'))
    turn_end()
end

function test_use_healing_potion_multi_units()
    -- Heiltrank kann mehrere Einheiten heilen
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u1 = unit.create(f, r, 30)
    local u = unit.create(f, r, 30)
    assert_equal(600, u1.hp)
    assert_equal(600, u.hp)
    u.hp = 400
    u1.hp = 400
    turn_begin()
    u:add_item("healing", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Heiltrank")
    turn_process()
    assert_equal(600, u.hp)
    assert_equal(600, u1.hp)
    assert_equal(0, u:get_item("healing"))
    turn_end()
end

function test_use_multiple_healing_potions()
    -- Einheit kann mehr als einen Heiltrank benutzen
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 60)
    assert_equal(1200, u.hp)
    u.hp = 400
    turn_begin()
    u:add_item("healing", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 2 Heiltrank")
    turn_process()
    assert_equal(1200, u.hp)
    assert_equal(0, u:get_item("healing"))
    turn_end()
end

function test_use_elixir()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 10)
    assert_equal(200, u.hp)
    u:add_item("p13", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Elixier~der~Macht")
    process_orders()
    -- potion makes hp 1000, monthly_healing takes away 400:
    assert_equal(600, u.hp)
    assert_equal(0, u:get_item("p13"))
    assert_equal(1, f:count_msg_type('use_item'))
end

function test_use_ointment()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 30)
    assert_equal(600, u.hp)
    u.hp = 100
    u:add_item("ointment", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Wundsalbe")
    process_orders()
    assert_equal(530, u.hp)
    assert_equal(0, u:get_item("ointment"))
    assert_equal(1, f:count_msg_type('use_item'))
end

function test_use_domore()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("p3", 1)
    u:add_order("BENUTZEN 1 Schaffenstrunk")
    process_orders()
    assert_equal(10, u:effect("p3"))
    assert_equal(0, u:get_item("p3"))
    assert_equal(1, f:count_msg_type('use_item'))
    u:clear_orders()
    u:set_skill('weaponsmithing', 3)
    u:add_item("iron", 2)
    u:add_order("MACHEN Schwert")
    process_orders()
    assert_equal(9, u:effect("p3"))
    assert_equal(0, u:get_item("iron"))
    assert_equal(2, u:get_item("sword"))
end

function test_make_greatbow()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "greatbow@eressea.de", "de")
    local u = unit.create(f, r, 1)
    turn_begin()
    u:add_item('mallorn', 2)
    u:set_skill('weaponsmithing', 5)
    u:clear_orders()
    u:add_order("MACHE 1 Elfenbogen")
    turn_process()
    assert_equal(2, u:get_item('mallorn'))
    assert_equal(0, u:get_item('greatbow'))
    assert_equal(1, f:count_msg_type('error117'))

    u.race='elf'
    turn_process()
    assert_equal(0, u:get_item('mallorn'))
    assert_equal(1, u:get_item('greatbow'))
    turn_end()
end

function test_bloodpotion_demon()
    local r = region.create(0, 0, "plain")
    local f = faction.create("demon")
    local u = unit.create(f, r, 1)
    u:add_item("peasantblood", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Bauernblut")
    process_orders()
    assert_equal(100, u:effect('peasantblood'))
    assert_equal(0, u:get_item("peasantblood"))
    assert_equal(1, f:count_msg_type('use_item'))
    assert_equal("demon", u.race)
end

function test_bloodpotion_other()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("peasantblood", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Bauernblut")
    process_orders()
    assert_equal(0, u:effect('peasantblood'))
    assert_equal(0, u:get_item("peasantblood"))
    assert_equal(1, f:count_msg_type('use_item'))
    assert_equal("smurf", u.race)
end

function test_water_of_life()
    eressea.settings.set("rules.grow.formula", 0) -- no tree growth
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
    local f = faction.create("insect")
    local u = unit.create(f, r, 1)
    local flags = u.flags
    u:add_item("nestwarmth", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Nestwaerme")
    turn_begin()
    turn_process()
    assert_equal(flags+64, u.flags) -- UFL_WARMTH
    assert_equal(1, u:get_item("nestwarmth"))
    assert_equal(1, f:count_msg_type('use_item'))
    turn_end()
end

function test_nestwarmth_other()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
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
    local f = faction.create("human")
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
    local f = faction.create("human")
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
    assert_equal(1, f:count_msg_type('aurapotion50_effect'))
    assert_equal(50, u.aura)
    turn_end()
end

function test_bagpipe()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
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

function test_monthly_healing()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 30)
    assert_equal(600, u.hp)
    u.hp = 100
    process_orders()
    assert_equal(130, u.hp)
end

function test_speedsail()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    
    turn_begin()
    u.ship = ship.create(r, "boat")
    u:add_item("speedsail", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Sonnensegel")
    assert_equal(nil, u.ship:get_curse('shipspeedup'))
    turn_process()
    assert_equal(1, u.ship:get_curse('shipspeedup'))
    assert_equal(1, u:get_item("speedsail"))
    assert_equal(1, f:count_msg_type('use_speedsail'))
    turn_end()
    assert_equal(1, u.ship:get_curse('shipspeedup')) -- effect stays forever
end

function test_use_foolpotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    turn_begin()
    u:add_item('p7', 3)
    u:set_orders("BENUTZEN 1 Dumpfbackenbrot 4242")
    turn_process()
    assert_equal(3, u:get_item('p7'))
    assert_equal(1, f:count_msg_type('feedback_unit_not_found'))
    local u2 = unit.create(f, r, 1)
    
    u:set_orders("BENUTZEN 2 Dumpfbackenbrot " .. itoa36(u2.id))
    turn_process()
    assert_equal(3, u:get_item('p7'))
    assert_equal(1, f:count_msg_type('error64'))

    u:set_skill("stealth", 1)
    u2:set_skill('crossbow', 1)
    turn_process()
    assert_equal(2, u:get_item('p7'))
    assert_equal(1, f:count_msg_type('givedumb'))
    turn_end()
    assert_equal(9, u2:effect('p7'))
end

function test_snowman()
    local r = region.create(0, 0, "glacier")
    local f = faction.create("human")
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
