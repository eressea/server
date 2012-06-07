require "lunit"

local eressea = eressea
local _G = _G

module("tests.bindings", lunit.testcase)

function test_eressea()
	assert_equal("function", _G.type(eressea.free_game))
	assert_equal("function", _G.type(eressea.read_game))
	assert_equal("function", _G.type(eressea.write_game))
	assert_equal("function", _G.type(eressea.read_orders))
end

function test_process()
	assert_equal("function", _G.type(eressea.process.markets))
	assert_equal("function", _G.type(eressea.process.produce))
	assert_equal("function", _G.type(eressea.process.make_temp))
	assert_equal("function", _G.type(eressea.process.settings))
	assert_equal("function", _G.type(eressea.process.set_group))
	assert_equal("function", _G.type(eressea.process.set_origin))
	assert_equal("function", _G.type(eressea.process.quit))
end

function test_settings()
	assert_equal("function", _G.type(eressea.settings.set))
	assert_equal("function", _G.type(eressea.settings.get))
end
