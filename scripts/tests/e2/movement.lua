require "lunit"

module("tests.e2.movement", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
end

 function test_piracy()
    local r = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local r3 = region.create(-1, 0, "ocean")
    local f = faction.create("pirate@eressea.de", "human", "de")
    local f2 = faction.create("elf@eressea.de", "human", "de")
    local u1 = unit.create(f, r2, 1)
    local u2 = unit.create(f2, r3, 1)
    
    u1.ship = ship.create(r2, "longboat")
    u2.ship = ship.create(r3, "longboat")
    u1:set_skill("sailing", 10)
    u2:set_skill("sailing", 10)

    u1:clear_orders()
    u1:add_order("PIRATERIE")
    u2:clear_orders()
    u2:add_order("NACH o")

    process_orders()

-- write_reports()
    assert_equal(r2, u1.region) -- should pass, but fails!!!
end 

function test_dolphin_on_land()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f, r1, 1)
    u1.race = "dolphin"
    u1:clear_orders()
    u1:add_order("NACH O")
    process_orders()
    assert_equal(r1, u1.region)
end

function test_dolphin_to_land()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f, r1, 1)
    u1.race = "dolphin"
    u1:clear_orders()
    u1:add_order("NACH O")
    process_orders()
    assert_equal(r2, u1.region)
end

function test_dolphin_in_ocean()
    local r1 = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f, r1, 1)
    u1.race = "dolphin"
    u1:clear_orders()
    u1:add_order("NACH O")
    process_orders()
    assert_equal(r2, u1.region)
end

function test_follow()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r1, 1)
    u1:clear_orders()
    u2:clear_orders()
    u1:add_item("money", 100)
    u2:add_item("money", 100)
    u1:add_order("NACH O")
    u2:add_order("FOLGE EINHEIT " .. itoa36(u1.id))
    process_orders()
    assert_equal(u1.region, r2)
    assert_equal(u2.region, r2)
end

function test_follow_ship()
    local r1 = region.create(0, 0, "plain")
    region.create(1, 0, "ocean")
    region.create(2, 0, "ocean")
    local f = faction.create("test@example.com", "human", "de")
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r1, 1)
    u1:add_item("money", 100)
    u2:add_item("money", 100)
    u1.ship = ship.create(r1, "boat")
    assert(u1.ship)
    u1:set_skill("sailing", 2)
    u1:clear_orders()
    u1:add_order("NACH O O")
    u2.ship = ship.create(r1, "boat")
    u2:set_skill("sailing", 2)
    u2:clear_orders()
    u2:add_order("FOLGE SCHIFF " .. itoa36(u1.ship.id))
    process_orders()
    assert_equal(2, u1.region.x)
    assert_equal(2, u2.region.x)
end
