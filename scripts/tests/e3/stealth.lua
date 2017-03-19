require "lunit"

module("tests.e3.stealth", package.seeall, lunit.testcase)

local f
local u

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4")

    local r = region.create(0,0, "plain")
    f = faction.create("human", "stealth1@eressea.de", "de")
    u = unit.create(f, r, 1)
    f = faction.create("human", "stealth2@eressea.de", "de")
end

function test_stealth_faction_on()
    u:clear_orders()
    u:add_order("TARNE PARTEI")

    process_orders()
    assert_not_match("Partei", report.report_unit(u, f))
    assert_match("anonym", report.report_unit(u, f))
end

function test_stealth_faction_other()
    u:clear_orders()
    u:add_order("TARNE PARTEI " .. itoa36(f.id))

    process_orders()
    assert_not_match("anonym", report.report_unit(u, f))
    assert_not_match(itoa36(f.id), report.report_unit(u, f))
end
