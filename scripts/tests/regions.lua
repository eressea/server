require "lunit"

module("tests.regions", package.seeall, lunit.testcase)

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
