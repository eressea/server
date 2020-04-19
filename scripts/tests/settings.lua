local tcname = 'tests.settings'
local lunit = require("lunit")
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, 'seeall')
end

function setup()
    eressea.free_game()
end

function test_settings()
    assert_equal(nil, eressea.settings.get('foo'))
    eressea.settings.set('foo', 'bar')
    assert_equal('bar', eressea.settings.get('foo'))
end
