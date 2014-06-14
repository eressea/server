require "lunit"

module("tests.regions", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    conf = [[{
        "terrains" : {
            "ocean": {},
            "plain": {}
        }
    }]]
    eressea.config.parse(conf)
end

function test_create()
    local r
    r = region.create(0, 0, "ocean")
    assert_not_nil(r)
end


