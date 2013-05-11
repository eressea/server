require "lunit"

module("tests.eressea.bson", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
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
    i = eressea.write_game("test_read_write.dat")
    assert_equal(0, i)
    eressea.free_game()
    r = get_region(0, 0)
    assert_equal(nil, r)
    i = eressea.read_game("test_read_write.dat")
    assert_equal(0, i)
    r = get_region(0, 0)
    assert_not_equal(nil, r)
    for a in attrib.get(r) do
        assert_equal(a.data, 42)
    end
end

function test_bson()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    assert_not_equal(nil, u)
    assert_not_equal(nil, r)
    assert_not_equal(nil, f)
    attrib.create(r, 1)
    assert_equal(attrib.get(r)().data, 1)
    attrib.create(u, 3)
    assert_equal(attrib.get(u)().data, 3)
    attrib.create(f, 5)
    assert_equal(attrib.get(f)().data, 5)
end

function test_bson_with_multiple_attribs()
    local r = region.create(0, 0, "mountain")
    attrib.create(r, { a=1})
    attrib.create(r, { a=5})
    local total = 0
    for a in attrib.get(r) do
        total = total + a.data.a;
    end
    assert_equal(6, total)
end
