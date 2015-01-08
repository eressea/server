require "lunit"

module('eressea.tests.stealth', package.seeall, lunit.testcase)

local f
local u

local settings

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
    set_rule('rules.economy.food', '4')
    set_rule('rules.magic.playerschools', '')

    local r = region.create(0,0, "plain")
    f = faction.create("stealthy@eressea.de", "human", "de")
    u = unit.create(f, r, 1)
    f = faction.create("stealth@eressea.de", "human", "de")
end

function teardown()
    set_rule('rules.economy.food')
    set_rule('rules.magic.playerschools')
    set_rule('rules.stealth.faction')
end

function test_stealth_faction_on()
	u:clear_orders()
	u:add_order("TARNEN PARTEI")

    set_rule("rules.stealth.faction", 1)
	process_orders()
	assert_not_match("Partei", report.report_unit(u, f))
	assert_match("anonym", report.report_unit(u, f))
end

function test_stealth_faction_off()
	u:clear_orders()
	u:add_order("TARNEN PARTEI")

    set_rule("rules.stealth.faction", 0)
	process_orders()
	assert_match("Partei", report.report_unit(u, f))
	assert_not_match("anonym", report.report_unit(u, f))
end
