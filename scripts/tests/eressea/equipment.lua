local tcname = 'tests.eressea.equipment'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

local equipment

function setup()
    equipment = require 'eressea.equipment'
    eressea.free_game()
    eressea.game.reset()
    eressea.config.reset();
    eressea.config.parse([[
{
    "races": {
        "human": { "hp" : 20 },
        "demon": { "hp" : 25 }
    },
    "terrains": {
        "plain" : {}
    },
    "items" : {
        "stone" : {}
    },
    "spells" : { "bug" : {} }
}
    ]])
end

function test_start()
    assert_equal(2500, equipment.get('first_unit')['items']['money'])
    equipment.add('test_start', { ['hodor'] = 'Hodor!'})
    assert_equal('Hodor!', equipment.get('test_start')['hodor'])
end

function add_test_equipment(u, flags)
    u:add_item("money", flags)
    u.number = 2
end


function test_equip_unit()
    local r = region.create(0,0,'plain')
    local f = faction.create('human')
    local u = unit.create(f, r)
    u.magic = 'illaun'

    equipment.add('test_equip', {
        ['items'] = { ['stone'] = 10, },
        ['skills'] = { ['magic'] = 1, },
        ['spells'] = { ['bug'] = 2},
        ['callback'] = add_test_equipment
    })
    equip_unit(u, 'test_equip', 0)
    assert_equal(0, u:get_item('stone'))
    local flags = EQUIP_ITEMS
    equip_unit(u, 'test_equip', flags)
    assert_equal(10, u:get_item('stone'))
    assert_equal(0, u:get_skill('magic'))
    flags = EQUIP_ITEMS + EQUIP_SKILLS
    equip_unit(u, 'test_equip', flags)
    assert_equal(20, u:get_item('stone'))
    assert_equal(1, u:get_skill('magic'))
    assert_equal(nil, u.spells)
    flags = EQUIP_ITEMS + EQUIP_SKILLS + EQUIP_SPELLS
    equip_unit(u, 'test_equip', flags)
    assert_equal(30, u:get_item('stone'))
    assert_equal(1, u:get_skill('magic'))
    for sp in u.spells do
        assert_equal("bug", sp.name)
        assert_equal(2, sp.level)
    end
    assert_equal(0, u:get_item('money'))
    flags = EQUIP_SKILLS + EQUIP_SPECIAL
    equip_unit(u, 'test_equip', flags)
    assert_equal(30, u:get_item('stone'))
    assert_equal(1, u:get_skill('magic'))
    for sp in u.spells do
        assert_equal("bug", sp.name)
        assert_equal(2, sp.level)
    end
    assert_equal(flags, u:get_item('money'))
    assert_equal(2, u.number)
    assert_equal(u.hp_max * u.number, u.hp)
end

function test_equip_hp()
    local r = region.create(0,0,'plain')
    local f = faction.create('human')
    local u = unit.create(f, r)

    equipment.add('test_equip', {
        ['skills'] = { ['stamina'] = 1, },
        ['callback'] = add_test_equipment
    })
    equip_unit(u, 'test_equip', EQUIP_SKILLS + EQUIP_SPECIAL)
    assert_equal(1, u:get_skill('stamina'))
    assert_equal(math.floor(2*20*1.07), u.hp)

end

function test_equip_demo()
    local r = region.create(0,0,'plain')
    local f = faction.create('demon')
    local u = unit.create(f, r)

    equip_unit(u, 'seed_demon', EQUIP_SKILLS + EQUIP_SPECIAL)
    assert_equal(15, u:get_skill('stamina'))
    assert_equal(u.hp_max, u.hp)

end