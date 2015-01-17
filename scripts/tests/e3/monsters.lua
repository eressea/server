require "lunit"

module("tests.monsters", package.seeall, lunit.testcase)

function setup()
    eressea.game.reset()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
end

function find_monsters()
   return get_faction(666)
end

function test_monster_attack()
    plan_monsters()
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("drac@eressea.de", "human", "de")
    local f2 = find_monsters()
    local u1 = unit.create(f1, r, 1)
    local u2 = unit.create(f2, r, 1)
    
    local oldpar = eressea.settings.get("rules.monsters.attack_chance")
    eressea.settings.set("rules.monsters.attack_chance", "1")

    u2.race = "skeleton"
    u2.number = 100
    u2:set_skill("unarmed", 10)
    plan_monsters()
    u2:add_order("BEWACHE")
    
    process_orders()
    plan_monsters()
    process_orders()
--    write_reports()
    
    assert_equal(0, u1.number)

    eressea.settings.set("rules.monsters.attack_chance", oldpar)
end
