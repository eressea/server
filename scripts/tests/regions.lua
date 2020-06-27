local tcname = 'tests.regions'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.game.reset()
    conf = [[{
        "terrains" : {
            "ocean": {},
            "plain": {}
        }
    }]]
    eressea.config.reset()
    eressea.settings.set('rules.magic.playerschools', '')
    eressea.config.parse(conf)
end

function test_create()
    local r
    r = region.create(0, 0, "ocean")
    assert_not_nil(r)
end
