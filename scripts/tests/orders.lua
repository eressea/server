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

function test_make_temp()
	u:add_order("MACHE TEMP 123 'Herpderp'")
	u:add_order("// this comment will be copied")
	u:add_order("ENDE")
	eressea.process.make_temp()

	for x in f.units do
		if x.name == 'Herpderp' then u=x end
	end
	assert_equal('Herpderp', u.name)
	assert_equal(0, u.number)
	local c = 0
	for o in u.orders do
		assert_equal('// this comment will be copied', o)
		c = c + 1
	end
	assert_equal(1, c)
end

function test_give_temp()
	u.number = 2
	u:add_order("GIB TEMP 123 1 PERSON")
	u:add_order("MACHE TEMP 123 'Herpderp'")
	u:add_order("ENDE")
	_G.process_orders()
	assert_equal(1, u.number)

	for x in f.units do
		if x.name == 'Herpderp' then u=x end
	end
	assert_equal('Herpderp', u.name)
	assert_equal(1, u.number)
end

function test_process_settings()
	f.options = 0
	u:add_order("EMAIL herp@derp.com")
	u:add_order("BANNER 'Herpderp'")
	u:add_order("PASSWORT 'HerpDerp'")
	u:add_order("OPTION AUSWERTUNG")
	eressea.process.settings()
	assert_equal("herp@derp.com", f.email)
	assert_equal("Herpderp", f.info)
	assert_equal("HerpDerp", f.password)
	assert_equal(1, f.options)
end

function test_process_group()
	u:add_order("GRUPPE herp")
	eressea.process.set_group()
	assert_equal('herp', u.group)
end

function test_process_origin()
	u:add_order("URSPRUNG 1 2")
	eressea.process.set_origin()
	x, y = u.faction:get_origin()
	assert_equal(1, x)
	assert_equal(2, y)
end

function test_process_quit()
	fno = f.id
	u:add_order("STIRB '" .. u.faction.password .. "'")
	assert_not_equal(nil, _G.get_faction(fno))
	eressea.process.quit()
	eressea.write_game('test.dat')
	eressea.free_game()
	eressea.read_game('test.dat')
	assert_equal(nil, _G.get_faction(fno))
end
