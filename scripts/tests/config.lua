require "lunit"

module("tests.eressea.config", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
end

function test_read_race()
    local f
    eressea.free_game()
    assert_not_nil(eressea.config)
    eressea.config.parse('{ "races": { "orc" : {}}}')
    f = faction.create("orc", "orc@example.com", "en")
    assert_not_nil(f)
end

function disable_test_read_ship()
    local s
    eressea.free_game()
    assert_not_nil(eressea.config)
    conf = [[{
        "ships": {
            "boat" : {
                "construction" : {
                    "maxsize" : 20
                }
            }
        }
    }]]
    eressea.config.parse(conf);
    s = ship.create(nil, "boat")
    assert_not_nil(s)
end
