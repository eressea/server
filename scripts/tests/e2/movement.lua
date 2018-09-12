require "lunit"

module("tests.e2.movement", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
end

function test_piracy()
    local r = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local r3 = region.create(-1, 0, "ocean")
    local f = faction.create("human", "pirate@eressea.de", "de")
    local f2 = faction.create("human", "elf@eressea.de", "de")
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

    if r2~=u1.region then
        write_reports()
    end
    assert_equal(r2, u1.region) -- should pass, but fails!!!
end 

function test_dolphin_on_land()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
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
    local f = faction.create("human", "noreply@eressea.de", "de")
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
    local f = faction.create("human", "noreply@eressea.de", "de")
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
    local f = faction.create("human", "test@example.com", "de")
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
    local f = faction.create("human", "test@example.com", "de")
    local u1 = unit.create(f, r1, 1)
    local u2 = unit.create(f, r1, 1)
    u2.name = 'Xolgrim'
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

function assert_nomove(text, u)
    if text == nil then text = "" else text = text .. "; " end
    local r = u.region
    u:add_order("NACH O O")
    process_orders()
    assert_equal(r, u.region, text .. "unit should never move")
end

function assert_capacity(text, u, silver, r1, r2, rx)
    if text == nil then text = "" else text = text .. "; " end
    if rx == nil then rx = r1 end
    u.region = r1
    u:add_item("money", silver-u:get_item("money"))
    u:add_order("NACH O O")
    process_orders()
    assert_equal(r2, u.region, text .. "unit should move")

    u.region = r1
    u:add_item("money", 1)
    process_orders()
    assert_equal(rx, u.region, text .. "unit should not move")
end

function test_dwarf_example()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    region.create(2, 0, "plain")
    local f = faction.create("dwarf", "dwarf@example.com", "de")
    local u = unit.create(f, r1, 5)
    u:add_item("horse", 5)
    u:add_item("cart", 2)

    -- 5 dwarves + 5 horse - 2 carts = 27 + 100 - 80 = 47.00
    assert_capacity("dwarves", u, 4700, r1, r2)

    u:set_skill("riding", 3)
    assert_equal(1, u:eff_skill("riding"))
    -- 5 dwarves + 5 horses + 2 carts = 327.00
    assert_capacity("riding", u, 32700, r1, r2)

end

function test_troll_example()
    local r1 = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "plain")
    local r3 = region.create(2, 0, "plain")
    local f = faction.create("troll", "troll@example.com", "de")
    local u1 = unit.create(f, r1, 3)

    u1:add_item("cart", 1)
    u1:clear_orders()

    -- 3 trolls - 1 cart = 320, but not allowed?
    u1.name='XXX'
    assert_nomove("3 trolls", u1)

    u1.number = 4

    -- 4 trolls + 1 cart = 14320
    assert_capacity("1 cart", u1, 14320, r1, r2)

    
    u1:add_item("horse", 4)
    -- 4 horses, 4 trolls, 1 cart
    assert_capacity("4 horses", u1, 22320, r1, r2)

    
    u1:add_item("cart", 1)

    -- 4 horses + 4 trolls + 1 cart - 1 cart
    assert_capacity("2 carts", u1, 18320, r1, r2)

    u1:set_skill("riding", 3)
    assert_equal(1, u1:eff_skill("riding"))

    -- 4 horses + 4 trolls + 2 carts = 323.20
    assert_capacity("walking", u1, 32320, r1, r2)

    -- 4 horses + 2 carts - 4 trolls = 200.00
    assert_capacity("riding", u1, 20000, r1, r3, r2)

end
