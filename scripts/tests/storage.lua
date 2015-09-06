require "lunit"

module("tests.storage", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
end

function test_store_unit()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply15@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local fid = f.id
    u:add_item("money", u.number * 100)
    local filename = "test.dat"
    store = storage.create(filename, "wb")
    assert_not_equal(store, nil)
    store:write_unit(u)
    store:close()
    eressea.free_game()
    -- recreate world:
    r = region.create(0, 0, "plain")
    f = faction.create("noreply16@eressea.de", "human", "de")
    f.id = fid
    store = storage.create(filename, "rb")
    assert_not_nil(store)
    u = store:read_unit()
    store:close()
    assert_not_nil(u)
    assert_equal(u:get_item("money"), u.number * 100)
end
