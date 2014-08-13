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
end

function test_faction_flags()
	assert_equal(0, f.flags)
	f.flags = 42
	assert_equal(42, f.flags)
end
