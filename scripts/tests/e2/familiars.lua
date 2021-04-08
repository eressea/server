local tcname = 'tests.e2.familiars'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.game.reset()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.peasants.growth.factor", "0")
    eressea.settings.set("magic.resist.enable", "0")
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("magic.regeneration.enable", "0")
end

local function setup_familiars(f, r)
    f.magic = 'gwyrrd'
    local uf = unit.create(f, r)
    uf.magic = 'gray'
    local u = unit.create(f, r)
    u.magic = 'gwyrrd'
    u:set_skill('magic', 9)
    u.familiar = uf
    return u, uf
end

function test_moneyspell()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local um, uf = setup_familiars(f, r)
    um.aura = 9
    um:add_order('ZAUBERE STUFE 9 Viehheilung')
    process_orders()
    assert_equal(0, um.aura)
    assert_equal(450, um:get_item('money'))
end

function test_moneyspell_through_familiar()
    -- casting magician's spell with the familiar: double cost
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local um, uf = setup_familiars(f, r)
    um.aura = 12
    uf:add_order('ZAUBERE STUFE 3 Viehheilung')
    process_orders()
    assert_equal(0, uf:get_skill('magic'))
    assert_equal(12, um.aura) -- cannot cast, familiar needs magic skill
    assert_equal(0, uf:get_item('money'))
    assert_equal(0, um:get_item('money'))

    uf:set_skill('magic', 2) -- can cast no higher than level 2
    process_orders()
    assert_equal(8, um.aura) -- double the cost
    assert_equal(100, uf:get_item('money'))
    assert_equal(0, um:get_item('money'))

    um:set_skill('magic', 4) -- use at most half of skill
    uf:set_skill('magic', 1) -- too low for level 2 spell, cast at level 1
    process_orders()
    assert_equal(6, um.aura) -- double cost of level 1
    assert_equal(150, uf:get_item('money'))
    assert_equal(0, um:get_item('money'))
end

function test_moneyspell_as_familiar()
    -- familiar has the spell and has magic skills: regular spellcasting rules apply
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local um, uf = setup_familiars(f, r)
    um.aura = 9
    uf.aura = 20
    uf:set_skill('magic', 10)
    uf:add_spell('earn_silver#gwyrrd')
    uf:add_order('ZAUBERE STUFE 10 Viehheilung')
    process_orders()
    assert_equal(500, uf:get_item('money'))
    assert_equal(10, uf.aura)
    assert_equal(9, um.aura)
end
