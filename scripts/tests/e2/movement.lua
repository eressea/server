require "lunit"

module("tests.e2.movement", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
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
