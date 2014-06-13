require "lunit"

module("tests.eressea.config", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
end

function test_read()
    local f
    f = faction.create("orc@example.com", "orc", "en")
    assert_equal(nil, f)
    assert_not_nil(eressea.config)
    eressea.config.parse('{ "races": { "orc" : {}}}')
    f = faction.create("orc@example.com", "orc", "en")
    assert_not_nil(f)
end

