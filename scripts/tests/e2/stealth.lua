require "lunit"

module('tests.e2.stealth', package.seeall, lunit.testcase)

local f
local u

local settings = {}

local function set_rule(key, value)
    if value==nil then
        eressea.settings.set(key, settings[key])
    else
        settings[key] = settings[key] or eressea.settings.get(key)
        eressea.settings.set(key, value)
    end
end

function setup()
    eressea.game.reset()
    set_rule('rules.food.flags', '4')

    local r = region.create(0,0, "plain")
    f = faction.create("stealthy@eressea.de", "human", "de")
    u = unit.create(f, r, 1)
    f = faction.create("stealth@eressea.de", "human", "de")
    unit.create(f, r, 1) -- TARNE PARTEI NUMMER <no> must have a unit in the region
end

function teardown()
    set_rule('rules.food.flags')
end

function test_stealth_faction_on()
	u:clear_orders()
	u:add_order("TARNEN PARTEI")

	process_orders()
	assert_not_match("Partei", report.report_unit(u, f))
	assert_match("anonym", report.report_unit(u, f))
end

function test_stealth_faction_other()
    u.name = "Enno"
	u:clear_orders()
	u:add_order("TARNEN PARTEI NUMMER " .. itoa36(f.id))

	process_orders()
	assert_match(itoa36(f.id), report.report_unit(u, f))
	assert_not_match("anonym", report.report_unit(u, f))
end
