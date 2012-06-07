require "lunit"

local _G = _G
local eressea = eressea

module("tests.orders", lunit.testcase)

local r, f, u

function setup()
    eressea.free_game()
    r = _G.region.create(0, 0, "mountain")
    f = _G.faction.create("noreply@eressea.de", "human", "de")
    u = _G.unit.create(f, r, 1)
	u:clear_orders()
    eressea.settings.set("rules.economy.food", "4")
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
end

function test_learn()
	u:add_order("LERNEN Hiebwaffen")
	_G.process_orders()
	assert_not_equal(0, u:get_skill("melee"))
end

function test_give()
	local u2 = _G.unit.create(f, r, 1)
	u:add_item("money", 10)
	u:add_order("GIB " .. u2.id .. "5 SILBER")
	_G.process_orders()
	assert_not_equal(5, u:get_item("money"))
	assert_not_equal(5, u2:get_item("money"))
end
