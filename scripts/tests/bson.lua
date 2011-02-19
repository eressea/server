require "lunit"

module("tests.bson", package.seeall, lunit.testcase)

function setup()
    free_game()
end

function test_bson_create()
    local a = attrib.create("global", 12)
    assert_not_equal(nil, a)
    for a in attrib.get("global") do
        assert_equal(a.data, 12)
    end
end

function test_bson_readwrite()
    local r = region.create(0, 0, "mountain")
    attrib.create(r, 42)
    write_game("test_read_write.dat")
    free_game()
    r = get_region(0, 0)
    assert_equal(nil, r)
    read_game("test_read_write.dat")
    r = get_region(0, 0)
    assert_not_equal(nil, r)
    for a in attrib.get(r) do
        assert_equal(a.data, 42)
    end
end
