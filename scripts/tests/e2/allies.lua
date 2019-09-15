require "lunit"

module("tests.e2.allies", package.seeall, lunit.testcase)

function skip_test_get_set_ally()
    local f1 = faction.create("human")
    local f2 = faction.create("human")

    assert_equal(false, f1:get_ally(f2, "guard"))
    f1:set_ally(f2, "guard", true)
    assert_equal(true, f1:get_ally(f2, "guard"))
    assert_equal(false, f1:get_ally(f2, "give"))
    f1:set_ally(f2, "give", true)
    assert_equal(true, f1:get_ally(f2, "give"))
end

function test_get_allies()
    local f1 = faction.create("human")
    local f2 = faction.create("human")
    
    local allies = f1.allies
    assert_equal('table', type(allies))
    assert_equal(0, #allies)
    f1:set_ally(f2, "give", true)
    allies = f1.allies
    assert_not_nil(allies[f2.id])
    assert_equal('table', type(allies[f2.id]))
    assert_equal(1, #allies[f2.id])
    assert_equal("give", allies[f2.id][1])
end
