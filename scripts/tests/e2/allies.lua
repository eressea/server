require "lunit"

module("tests.e2.allies", package.seeall, lunit.testcase)

function test_get_set_ally()
    local f1 = faction.create("human")
    local f2 = faction.create("human")

    assert_equal(false, f1:get_ally(f2, "guard"))
    f1:set_ally(f2, "guard", true)
    assert_equal(true, f1:get_ally(f2, "guard"))
    assert_equal(false, f1:get_ally(f2, "give"))
    f1:set_ally(f2, "give", true)
    assert_equal(true, f1:get_ally(f2, "give"))
end
