require "lunit"

module("e3-stealth", package.seeall, lunit.testcase)

function setup_stealth()
	local result = {}
	free_game()
    result.r = region.create(0,0, "plain")
	result.f1 = faction.create("noreply@eressea.de", "human", "de")
	result.f2 = faction.create("noreply@eressea.de", "human", "de")
	result.u1 = unit.create(result.f1, result.r, 1)
	result.u2 = unit.create(result.f2, result.r, 1)
    result.u1:add_item("money", 1000)
    result.u2:add_item("money", 1000)
	return result
end

function test_stealth_faction_on()
	local result = setup_stealth()
	local f = result.f2
	local u = result.u1
	u:clear_orders()
	u:add_order("TARNEN PARTEI")

	settings.set("rules.stealth.faction", 1)
	process_orders()
	assert_not_match("Partei", report.report_unit(u, f))
	assert_match("anonym", report.report_unit(u, f))
end

function test_stealth_faction_off()
	local result = setup_stealth()
	local f = result.f2
	local u = result.u1
	u:clear_orders()
	u:add_order("TARNEN PARTEI")

	settings.set("rules.stealth.faction", 0)
	process_orders()
	assert_match("Partei", report.report_unit(u, f))
	assert_not_match("anonym", report.report_unit(u, f))
end
