require "lunit"

module("tests.e3.stealth", package.seeall, lunit.testcase)

local f
local u

function setup()
	eressea.game.reset()
	eressea.settings.set("rules.food.flags", "4")

	local r = region.create(0,0, "plain")
	f = faction.create("stealth1@eressea.de", "human", "de")
	u = unit.create(f, r, 1)
	f = faction.create("stealth2@eressea.de", "human", "de")
end

function test_stealth_faction_on()
	u:clear_orders()
	u:add_order("TARNE PARTEI")

	eressea.settings.set("rules.stealth.faction", 1)
	process_orders()
	assert_not_match("Partei", report.report_unit(u, f))
	assert_match("anonym", report.report_unit(u, f))
end

function test_stealth_faction_off()
	u:clear_orders()
	u:add_order("TARNE PARTEI")

	eressea.settings.set("rules.stealth.faction", 0)
	process_orders()
	assert_match("Partei", report.report_unit(u, f))
	assert_not_match("anonym", report.report_unit(u, f))
end
