local tcname = 'tests.faction'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

local f

function setup()
    conf = [[{
        "races": {
        "human" : {}
        }
    }]]
    eressea.config.reset()
    assert(eressea.config.parse(conf)==0)
    f = faction.create("human", "faction@eressea.de", "de")
    assert(f~=nil)
end

function test_faction_flags()
    assert_equal(6, f.flags) -- FFL_ISNEW|FFL_PWMSG
    f.flags = 42
    assert_equal(42, f.flags)
end

function test_get_faction()
    assert_equal(f, get_faction(f.id))
    assert_equal(f, faction.get(f.id))
    local nf = f.id
    if nf>1 then nf = nf - 1 else nf = 1 end
    assert_equal(nil, faction.get(nf))
end
