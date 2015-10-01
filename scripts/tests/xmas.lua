local function use_tree(terrain)
    local r = region.create(0,0, terrain)
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
