local tcname = 'tests.skills'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.config.parse([[
{
    "races": {
        "human": {}
    },
    "terrains": {
        "plain" : {}
    }
}
    ]])
end

function test_unit_get_set_skill()
    local r = region.create(0, 0, 'plain')
    local f = faction.create('human')
    local u = unit.create(f, r, 1)
    u:set_skill('alchemy', 5)
    assert_equal(5, u:get_skill('alchemy'))
    assert_equal(5, u:eff_skill('alchemy'))
end

local function getTableSize(t)
    local count = 0
    for _, __ in pairs(t) do
        count = count + 1
    end
    return count
end

function test_unit_skills()
    local r = region.create(0, 0, 'plain')
    local f = faction.create('human')
    local u = unit.create(f, r, 1)
    u:set_skill('alchemy', 5)
    u:set_skill('melee', 6)
    local skills = u.skills
    assert_equal(2, getTableSize(skills))
    assert_equal(5, skills['alchemy'])
    assert_equal(6, skills['melee'])
    assert_nil(skills['crossbow'])
end
