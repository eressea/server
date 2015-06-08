require "lunit"

module ('tests.eressea.faction', package.seeall, lunit.testcase)

local f

function setup()
    conf = [[{
        "races": {
        "human" : {}
        }
    }]]
    eressea.config.reset()
    assert(eressea.config.parse(conf)==0)
    f = faction.create("faction@eressea.de", "human", "de")
    assert(f~=nil)
end

function test_faction_flags()
    assert_equal(0, f.flags)
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
