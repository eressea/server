require 'lunit'

module('tests.e3.parser', package.seeall, lunit.testcase)

function setup()
    eressea.game.reset()
end

function test_set_status_en()
    local r = region.create(0, 0, "plain")    
    local f = faction.create("bug_1882@eressea.de", "human", "en")
    local u = unit.create(f, r, 1)
    u.status = 1
    u:clear_orders()
    u:add_order("COMBAT AGGRESSIVE")
    process_orders()
    assert_equal(u.status, 0)
    u.status = 1
    u:add_order("FIGHT AGGRESSIVE")
    process_orders()
    assert_equal(u.status, 0)
end

function test_set_status_de()
    local r = region.create(0, 0, "plain")    
    local f = faction.create("bug_1882@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.status = 1
    u:clear_orders()
    u:add_order("KAEMPFE AGGRESSIV")
    process_orders()
    assert_equal(u.status, 0)
end

