require "lunit"

module("tests.xmas", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.grow.formula", "0")
end

function test_snowglobe_fail()
    local r1 = region.create(0, 0, "glacier")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("snowglobe1@eressea.de", "human", "de")
    local u = unit.create(f, r1, 1)
    u:add_item("snowglobe", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Schneekugel Ost")
    unit.create(f, r2, 1) -- unit in target region => fail
    process_orders()
    assert_equal('ocean', r2.terrain)
    assert_equal(1, u:get_item('snowglobe'))
    assert_equal(1, f:count_msg_type('target_region_not_empty'))
end

function test_snowglobe_missing_direction()
    local r1 = region.create(0, 0, "glacier")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("snowglobe1@eressea.de", "human", "de")
    local u = unit.create(f, r1, 1)
    u:add_item("snowglobe", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Schneekugel")
    process_orders()
    assert_equal('ocean', r2.terrain)
    assert_equal(1, u:get_item('snowglobe'))
    assert_equal(1, f:count_msg_type('missing_direction'))
end

function test_snowglobe()
    local r1 = region.create(0, 0, "glacier")
    local r2 = region.create(1, 0, "ocean")
    local f = faction.create("snowglobe2@eressea.de", "human", "de")
    local u = unit.create(f, r1, 1)
    local have = 6
    local fail = 0
    u:add_item("snowglobe", have)
    local xform = { ocean = "glacier", glacier = "glacier", firewall = "volcano", volcano = "mountain", desert = "plain", plain = "plain" }
    u:clear_orders()
    u:add_order("BENUTZEN 1 Schneekugel Ost")
    for k, v in pairs(xform) do
        r2.terrain = k
        process_orders()
        assert_equal(v, r2.terrain)
        if k~=v then
            have=have - 1 
        else
            fail = fail + 1
            assert_equal(fail, f:count_msg_type('target_region_invalid'))
        end
        assert_equal(have, u:get_item("snowglobe"))
    end
end

local function use_tree(terrain)
    local r = region.create(0, 0, terrain)
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f, r, 5)
    r:set_resource("tree", 0)
    u1:add_item("xmastree", 1)
    u1:clear_orders()
    u1:add_order("BENUTZEN 1 Weihnachtsbaum")
    process_orders()
    return r
end

function test_xmastree()
    local r
    r = use_tree("ocean")
    assert_equal(0, r:get_resource("tree"))
    eressea.free_game()
    r = use_tree("plain")
    assert_equal(10, r:get_resource("tree"))
end
