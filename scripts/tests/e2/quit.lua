require "lunit"

module("tests.e2.quit", package.seeall, lunit.testcase)

function test_quit_faction()
    local r = region.create(47, 0, "plain")
    local f1 = faction.create("human")
    f1.password = "steamedhams"
    local f2 = faction.create("human")
    local u1 = unit.create(f1, r, 8)
    local u2 = unit.create(f2, r, 9)
    local u3 = unit.create(f1, r, 10)
    u1:clear_orders()
    u2:clear_orders()
    u1:add_order("STIRB steamedhams PARTEI " .. itoa36(f2.id))
    u2:add_order("KONTAKTIERE " .. itoa36(u1.id))
    process_orders()
    assert_equal(f2, u1.faction)
    assert_equal(f2, u2.faction)
    assert_equal(f2, u3.faction)
end
