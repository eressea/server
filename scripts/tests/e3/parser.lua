require 'lunit'

module('tests.e3.parser', package.seeall, lunit.testcase)

function setup()
    eressea.game.reset()
end

function test_set_status_en()
    local r = region.create(0, 0, "plain")    
    local f = faction.create("human", "bug_1882@eressea.de", "en")
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
    local f = faction.create("human", "bug_1882@eressea.de", "de")
    local u = unit.create(f, r, 1)
    u.status = 1
    u:clear_orders()
    u:add_order("KAEMPFE AGGRESSIV")
    process_orders()
    assert_equal(u.status, 0)
end

function test_breed_horses()
    local r = region.create(0, 0, "plain")    
    local f = faction.create("human", "bug_1886@eressea.de", "en")
    local u = unit.create(f, r, 1)
    local b = building.create(r, "stables")
    b.size = 10
    u.building = b
    u:add_item("horse", 2)
    u:add_item("money", 2000)
    u:set_skill("training", 100) -- 100% chance to duplicate each horse
    u:clear_orders()
    u:add_order("GROW horses")
    process_orders()
    assert_equal(4, u:get_item("horse"))
    u:clear_orders()
    u:add_order("BREED horses")
    process_orders()
    assert_equal(8, u:get_item("horse"))
    u:clear_orders()
    u:add_order("PLANT horses")
    process_orders()
    assert_equal(16, u:get_item("horse"))
end
