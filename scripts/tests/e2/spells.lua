local tcname = 'tests.e2.spells'
local lunit = require("lunit")
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, 'seeall')
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

function test_undead_cannot_enter_holyground()
    local r1 = region.create(0, 0, 'plain')
    local r2 = region.create(1, 0, 'plain')
    local f = faction.create('human')
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r2, 1)

    u2.magic = 'gwyrrd'
    u2:set_skill('magic', 100)
    u2.aura = 200
    u2:add_spell('holyground')
    u2:add_order('ZAUBERE STUFE 10 "Heiliger Boden"')

    u1.race = "skeleton"
    u1:add_order("NACH Osten")
    process_orders()
    assert_not_nil(r2:get_curse('holyground'))
    assert_equal(r1, u1.region)
end

function test_shapeshift()
    local r = region.create(42, 0, "plain")
    local f = faction.create("demon", "noreply@eressea.de", "de")
    local u1 = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u1:clear_orders()
    u1.magic = "gray"
    u1:set_skill("magic", 2)
    u1.aura = 1
    u1:add_spell("shapeshift")
    u1:add_order("ZAUBERE STUFE 1 Gestaltwandlung " .. itoa36(u2.id) .. " Goblin")
    process_orders()
    assert_equal(f.race, u2.race)
    s = u2:show()
    assert_equal("1 Goblin", string.sub(s, string.find(s, "1 Goblin")))
end

function test_raindance()
    local r = region.create(0, 0, "plain")
    local f = faction.create("halfling", "noreply@eressea.de", "de")
    local u = unit.create(f, r)
    local err = 0
    r:set_resource("peasant", 100)
    r:set_resource("money", 0)
    u.magic = "gwyrrd"
    u.race = "dwarf"
    u:set_skill("magic", 20)
    u.aura = 200
    err = err + u:add_spell("raindance")
    assert_equal(0, err)
    
    u:clear_orders()
    u:add_order("ZAUBERE STUFE 1 Regentanz")
    assert_equal(0, r:get_resource("money"))

    process_orders()
    assert_equal(200, r:get_resource("money"))
    assert_equal(0, u:get_item("money"))

    u:clear_orders()
    u:add_order("ARBEITEN")
    process_orders()
    assert_equal(10, u:get_item("money")) -- only peasants benefit
    assert_equal(400, r:get_resource("money"))
    -- this is where the spell ends
    process_orders()
    process_orders()
    assert_equal(600, r:get_resource("money"))
end

function test_earn_silver()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human")
    local u = unit.create(f, r)

    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("magic.fumble.enable", "0")
    eressea.settings.set("rules.peasants.growth", "0")
    eressea.settings.set("rules.economy.repopulate_maximum", "0")

    u.magic = "gwyrrd"
    u.race = "elf"
    u:set_skill("magic", 10)
    u.aura = 100
    local err = u:add_spell("earn_silver#gwyrrd")
    assert_equal(0, err)

    u:clear_orders()
    u:add_order("ZAUBERE STUFE 1 Viehheilung")
    r:set_resource("money", 350)
    r:set_resource("peasant", 0)
    process_orders() -- get 50 silver
    assert_equal(50, u:get_item("money"))
    assert_equal(300, r:get_resource("money"))

    u:clear_orders() -- get 100 silver
    u:add_order("ZAUBERE STUFE 2 Viehheilung")
    process_orders()
    assert_equal(150, u:get_item("money"))
    assert_equal(200, r:get_resource("money"))

    u:clear_orders() -- get 150 silver
    u:add_order("ZAUBERE STUFE 3 Viehheilung")
    process_orders()
    assert_equal(300, u:get_item("money"))
    assert_equal(50, r:get_resource("money"))

    process_orders() -- not enough
    assert_equal(350, u:get_item("money"))
    assert_equal(0, r:get_resource("money"))
end

function test_familiar_cast()
    local r = region.create(0, 0, "plain")
    r:set_resource("money", 350)
    r:set_resource("peasant", 0)
    local f = faction.create("human")
    local u = unit.create(f, r)
    u.magic = "gwyrrd"
    u:set_skill("magic", 10)
    u.aura = 200
    u:add_spell("earn_silver#gwyrrd")
    u:add_order('ARBEITE')
    local uf = unit.create(f, r)
    uf.race = "lynx"
    uf:set_skill("magic", 5)
    uf:add_order('ZAUBER STUFE 1 Viehheilung')
    u.familiar = uf
    process_orders()
    assert_equal(198, u.aura) -- Fremdzauber, Kosten verdoppelt
    assert_equal(10, u:get_item('money')) -- von ARBEITE
    assert_equal(50, uf:get_item('money')) -- von Zauber
    assert_equal(300, uf.region:get_resource("money"))
end

function test_familiar_mage_actions()
    local r = region.create(0, 0, "plain")
    r:set_resource("money", 350)
    r:set_resource("peasant", 0)
    local f = faction.create("human")
    local u = unit.create(f, r)
    u.magic = "gwyrrd"
    u:set_skill("magic", 10)
    u.aura = 200
    u:add_spell("earn_silver#gwyrrd")
    u:add_order('ZAUBER STUFE 1 Viehheilung')
    local uf = unit.create(f, r)
    uf.race = "lynx"
    uf:set_skill("magic", 5)
    uf:add_order('ZAUBER STUFE 1 Viehheilung')
    u.familiar = uf
    process_orders()
    assert_equal(50, u:get_item('money'))
    assert_equal(50, uf:get_item('money'))
    assert_equal(250, uf.region:get_resource("money"))
    assert_equal(197, u.aura)
end

function test_familiar()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human")
    local u = unit.create(f, r)
    local uid = u.id
    u.name = 'Hodor'
    u.magic = "gwyrrd"
    u.race = "elf"
    u:set_skill("magic", 10)
    u.aura = 200
    local err = u:add_spell("summon_familiar")
    assert_equal(0, err)
    u:add_order("ZAUBERE Vertrauten~rufen")
    process_orders()
    for u in r.units do 
        if u.id ~= uid then
            assert_equal('Vertrauter von Hodor (' .. itoa36(uid) ..')', u.name)
        end
    end
end

function test_familiar_lynx()
    local r = region.create(0, 0, 'plain')
    local f = faction.create('human')
    local u = unit.create(f, r)
    u.race = 'lynx'
    u:equip('fam_lynx')
    assert_equal(1, u:get_skill('stealth'))
    assert_equal(1, u:get_skill('espionage'))
    assert_equal(1, u:get_skill('magic'))
    assert_equal(1, u:get_skill('perception'))
end

function test_bug_2480()
  local r = region.create(0, 0, "plain")
  local f = faction.create("human", "2480@eressea.de", "de")
  local u1 = unit.create(f, r, 1)
  local monster = unit.create(get_monsters(), r, 1, "wyrm")
  u1.number = 30
  u1.hp = u1.hp_max * u1.number
  monster:add_order("ATTACK " .. itoa36(u1.id))
  process_orders()
  assert_equal(0, u1.number);
end

function test_bug_2517()
  local r = region.create(0, 0, "plain")
  local f = faction.create("elf")
  local um = unit.create(f, r, 1)
  local uf = nil
  eressea.settings.set("magic.familiar.race", "lynx")
  f.magic = 'gwyrrd'
  um.magic = 'gwyrrd'
  um.race = 'elf'
  um:set_skill('magic', 10)
  um:add_spell('summon_familiar')
  um:add_spell('earn_silver#gwyrrd')
  um:add_order('ZAUBERE Vertrauten~rufen')
  um.aura = 200
  process_orders()
  uf = um.familiar
  assert_not_nil(uf)
  assert_equal('lynx', uf.race)
  assert_equal('gray', uf.magic)
  uf:add_order('LERNE Magie')
  um:clear_orders()
  um:add_order('ARBEITEN')
  process_orders()
  assert_equal('gray', uf.magic)
  uf:add_order('ZAUBERE STUFE 1 Viehheilung')
  process_orders()
  assert_equal(50, uf:get_item('money'))
end

function test_familiar_school()
    local r = region.create(0, 0, "plain")
    r:set_resource("money", 350)
    r:set_resource("peasant", 0)
    local f = faction.create("human")
    local u = unit.create(f, r)
    u.magic = "draig"
    u:set_skill("magic", 10)
    local uf = unit.create(f, r)
    uf.race = "lynx"
    u.familiar = uf

    assert_nil(uf.magic)
    uf:set_skill("magic", 5)
    assert_nil(uf.magic)
    uf.aura = 10
    assert_equal(0, uf.aura)
    assert_nil(uf.magic)
end

function test_astral_disruption()
    local r = region.create(0, 0, "plain")
    local r2 = r:get_astral('fog')
    local f = faction.create("human")
    local u = unit.create(f, r)
    local uh = unit.create(get_monsters(), r2, 1, "braineater")
    u.magic = "draig"
    u:set_skill("magic", 100) -- level 100 should beat magic resistance
    u.aura = 200
    u:add_spell("astral_disruption")
    u:add_order('ZAUBERE STUFE 1 "Stoere Astrale Integritaet"')
    process_orders()
    assert_not_nil(r2:get_curse("astralblock"))
    assert_equal(r, uh.region)
end
