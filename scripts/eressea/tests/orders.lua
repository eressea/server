require "lunit"

local _G = _G
local eressea = eressea
local default_ship = config.ships[1]
local default_building = config.buildings[1]

module('tests.eressea.orders', package.seeall, lunit.testcase)

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
    os.remove('test.dat')
    assert_equal(nil, _G.get_faction(fno))
end

function test_process_make()
    u.region:set_resource('tree', 100)
    u:set_skill('forestry', 1)
    u:add_order('MACHE HOLZ')
    eressea.process.produce()
    assert_equal(1, u:get_item('log'))
end

function test_process_study()
    u:add_order("LERNEN Holzfaellen")
    eressea.process.update_long_order()
    eressea.process.study()
    x, y = u.faction:get_origin()
    assert_equal(1, u:get_skill('forestry'))
end

function test_process_teach()
    eressea.settings.set("study.random_progress", "0")
    u:set_skill('forestry', 3)
    u2 = _G.unit.create(f, r, 10)
    u2:clear_orders()
    u2:set_skill('forestry', 1)
    u2:add_order("LERNEN Holzfaellen")
    u:add_order("LEHREN " .. _G.itoa36(u2.id))
    eressea.process.update_long_order()
    eressea.process.study()
    assert_equal(2, u2:get_skill('forestry'))
end

function test_process_move()
    r2 = _G.region.create(1, 0, 'plain')
    u:add_order('NACH O')
    assert_not_equal(r2.id, u.region.id)
    eressea.process.update_long_order()
    eressea.process.movement()
    assert_equal(r2, u.region)
end

function test_process_leave()
    r2 = _G.region.create(1, 0, 'plain')
    b = _G.building.create(r, default_building)
    assert_not_nil(b)
    u.building = b
    assert_equal(b, u.building)
    u:add_order('VERLASSEN')
    eressea.process.leave()
    assert_not_equal(b, u.building)
end

function test_process_name_unit()
    u:add_order("BENENNE EINHEIT 'Weasel'")
    u:add_order("BESCHREIBE EINHEIT 'Juanita'")
    eressea.process.set_name()
    assert_equal('Weasel', u.name)
    assert_equal('Juanita', u.info)
end

function test_process_name_faction()
    u:add_order("BENENNE PARTEI 'Herpderp'")
    eressea.process.set_name()
    assert_equal('Herpderp', f.name)
end

function test_process_name_building()
    u:add_order("BENENNE GEBAEUDE 'Herpderp'")
    u.building = _G.building.create(r, default_building)
    eressea.process.set_name()
    assert_equal('Herpderp', u.building.name)
end

function test_process_name_ship()
    u:add_order("BENENNE SCHIFF 'Herpderp'")
    u.ship = _G.ship.create(r, default_ship)
    eressea.process.set_name()
    assert_equal('Herpderp', u.ship.name)
end

function test_process_renumber()
    u:add_order("NUMMER EINHEIT 'ii'")
    eressea.process.renumber()
    assert_equal(666, u.id)
end

function test_process_enter()
    b = _G.building.create(r, default_building)
    assert_not_nil(b)
    u:add_order("BETRETEN GEBAEUDE " .. _G.itoa36(b.id))
    eressea.process.enter(1)
    assert_equal(b, u.building)
end

function test_process_restack()
    eressea.process.restack()
end

function test_process_setspells()
    eressea.process.set_spells()
end

function test_process_help()
    eressea.process.set_help()
end

function test_process_contact()
    eressea.process.contact()
end

function test_process_battle()
    eressea.process.battle()
end

function test_process_magic()
    eressea.process.magic()
end

function test_process_give_control()
    eressea.process.give_control()
end

function test_process_regeneration()
    eressea.process.regeneration()
end

function test_process_guard_on()
    eressea.process.guard_on()
end

function test_process_guard_off()
    eressea.process.guard_off()
end

function test_process_explain()
    eressea.process.explain()
end

function test_process_messages()
    eressea.process.messages()
end

function test_process_reserve()
    eressea.process.reserve()
end

function test_process_claim()
    eressea.process.claim()
end

function test_process_follow()
    eressea.process.follow()
end

function test_process_idle()
    eressea.process.idle()
end

function test_process_set_default()
    eressea.process.set_default()
end
