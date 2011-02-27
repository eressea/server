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

function test_illegal_arg()
    local a = attrib.create(nil, 42)
    assert_equal(nil, a)
    a = attrib.create("fred", 42)
    assert_equal(nil, a)
end

function test_bson_readwrite()
    local i, r = region.create(0, 0, "mountain")
    attrib.create(r, 42)
    i = write_game("test_read_write.dat")
    assert_equal(0, i)
    free_game()
    r = get_region(0, 0)
    assert_equal(nil, r)
    i = read_game("test_read_write.dat")
    assert_equal(0, i)
    r = get_region(0, 0)
    assert_not_equal(nil, r)
    for a in attrib.get(r) do
        assert_equal(a.data, 42)
    end
end
