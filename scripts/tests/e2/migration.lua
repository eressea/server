local tcname = 'tests.e2.migration'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.game.reset()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("rules.peasants.growth.factor", "0")
    eressea.settings.set("magic.resist.enable", "0")
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("magic.regeneration.enable", "0")
end

local function setup_mage(f, r)
    local u = unit.create(f, r)
    u.magic = 'tybied'
    u:set_skill('magic', 10)
    u:add_spell('migration')
    return u
end

function test_migration_success()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = setup_mage(f, r)
    local u2 = unit.create(faction.create('human'), r)
    u2:add_order('KONTAKTIERE ' .. itoa36(u.id))
    u:add_order('ZAUBERE STUFE 1 "Ritual der Aufnahme" ' .. itoa36(u2.id))
    u.aura = 9
    u.aura_max = 9
    process_orders()
    assert_equal(f, u2.faction)
    assert_equal(6, u.aura)
    assert_equal(8, u.aura_max)
end

function test_migration_no_familiars()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = setup_mage(f, r)
    local f2 = faction.create('human')
    local u2 = setup_mage(f2, r)
    local u3 = unit.create(f2, r)
    u3.race = 'goblin'
    u2.familiar = u3
    u.name = 'Kappa'
    u3:add_order('KONTAKTIERE ' .. itoa36(u.id))
    u:add_order('ZAUBERE STUFE 1 "Ritual der Aufnahme" ' .. itoa36(u3.id))
    u.aura = 9
    u.aura_max = 9
    process_orders()
    assert_not_equal(f, u3.faction)
    assert_equal(9, u.aura)
    assert_equal(9, u.aura_max)
end

function test_migration_no_contact()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = setup_mage(f, r)
    local u2 = unit.create(faction.create('human'), r)
    u:add_order('ZAUBERE STUFE 1 "Ritual der Aufnahme" ' .. itoa36(u2.id))
    u.aura = 9
    u.aura_max = 9
    process_orders()
    assert_not_equal(f, u2.faction)
    assert_equal(9, u.aura)
    assert_equal(9, u.aura_max)
end

function test_migration_too_many()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = setup_mage(f, r)
    local u2 = unit.create(faction.create('human'), r)
    u2:add_order('KONTAKTIERE ' .. itoa36(u.id))
    u2.number = 2
    u:add_order('ZAUBERE STUFE 1 "Ritual der Aufnahme" ' .. itoa36(u2.id))
    u.aura = 9
    u.aura_max = 9
    process_orders()
    assert_not_equal(f, u2.faction)
    assert_equal(9, u.aura)
    assert_equal(9, u.aura_max)
end

function test_migration_with_ring()
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = setup_mage(f, r)
    local u2 = unit.create(faction.create('human'), r)
    u2:add_order('KONTAKTIERE ' .. itoa36(u.id))
    u2.number = 2
    u:add_item('rop', 1)
    u:add_order('ZAUBERE STUFE 1 "Ritual der Aufnahme" ' .. itoa36(u2.id))
    u.aura = 9
    u.aura_max = 9
    process_orders()
    assert_equal(f, u2.faction)
     -- cost scales with result:
    assert_equal(3, u.aura)
    assert_equal(7, u.aura_max)
end

function test_migration_insufficient_aura()
    -- if unit cannot pay full costs, it casts at a lower level.
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = setup_mage(f, r)
    local u2 = unit.create(faction.create('human'), r)
    u2:add_order('KONTAKTIERE ' .. itoa36(u.id))
    u2.number = 2
    u:add_order('ZAUBERE STUFE 2 "Ritual der Aufnahme" ' .. itoa36(u2.id))
    u.aura = 3
    u.aura_max = 9
    process_orders()
    -- spell fails, costs nothing:
    assert_not_equal(f, u2.faction)
    assert_equal(3, u.aura)
    assert_equal(9, u.aura_max)
end

function test_migration_reduced_cost()
    -- if unit cannot pay full costs, it casts at a lower level.
    local r = region.create(0, 0, "plain")
    local f = faction.create('human')
    local u = setup_mage(f, r)
    local u2 = unit.create(faction.create('human'), r)
    u2:add_order('KONTAKTIERE ' .. itoa36(u.id))
    u:add_order('ZAUBERE STUFE 2 "Ritual der Aufnahme" ' .. itoa36(u2.id))
    u.aura = 3
    u.aura_max = 9
    process_orders()
    -- spell is cast at level 1:
    assert_equal(f, u2.faction)
    assert_equal(0, u.aura)
    assert_equal(8, u.aura_max)
end

--[[
additional tests:
- not enough aura, casting at lower level
- no aura, ring does not grant level 1
- magic tower, like ring, cumulative
]]--
