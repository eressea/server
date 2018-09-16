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
	assert_equal("function", _G.type(eressea.process.update_long_order))
	assert_equal("function", _G.type(eressea.process.markets))
	assert_equal("function", _G.type(eressea.process.produce))

	assert_equal("function", _G.type(eressea.process.make_temp))
	assert_equal("function", _G.type(eressea.process.settings))
	assert_equal("function", _G.type(eressea.process.set_allies))
	assert_equal("function", _G.type(eressea.process.set_prefix))
	assert_equal("function", _G.type(eressea.process.set_stealth))
	assert_equal("function", _G.type(eressea.process.set_status))
	assert_equal("function", _G.type(eressea.process.set_name))
	assert_equal("function", _G.type(eressea.process.set_group))
	assert_equal("function", _G.type(eressea.process.set_origin))
	assert_equal("function", _G.type(eressea.process.quit))
	assert_equal("function", _G.type(eressea.process.study))
	assert_equal("function", _G.type(eressea.process.movement))
	assert_equal("function", _G.type(eressea.process.use))
	assert_equal("function", _G.type(eressea.process.battle))
	assert_equal("function", _G.type(eressea.process.leave))
	assert_equal("function", _G.type(eressea.process.promote))
	assert_equal("function", _G.type(eressea.process.restack))
	assert_equal("function", _G.type(eressea.process.set_spells))
	assert_equal("function", _G.type(eressea.process.set_help))
	assert_equal("function", _G.type(eressea.process.contact))
	assert_equal("function", _G.type(eressea.process.enter))
	assert_equal("function", _G.type(eressea.process.magic))
	assert_equal("function", _G.type(eressea.process.give_control))
	assert_equal("function", _G.type(eressea.process.regeneration))
	assert_equal("function", _G.type(eressea.process.guard_on))
	assert_equal("function", _G.type(eressea.process.guard_off))
	assert_equal("function", _G.type(eressea.process.explain))
	assert_equal("function", _G.type(eressea.process.messages))
	assert_equal("function", _G.type(eressea.process.reserve))
	assert_equal("function", _G.type(eressea.process.claim))
	assert_equal("function", _G.type(eressea.process.follow))
	assert_equal("function", _G.type(eressea.process.alliance))
	assert_equal("function", _G.type(eressea.process.idle))
	assert_equal("function", _G.type(eressea.process.set_default))
end

function test_settings()
	assert_equal("function", _G.type(eressea.settings.set))
	assert_equal("function", _G.type(eressea.settings.get))
end
