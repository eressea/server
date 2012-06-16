require "lunit"

module("tests.eressea.attrib", package.seeall, lunit.testcase)

function has_attrib(u, value)
    for a in u.attribs do
        if (a.data==value) then return true end
    end
    return false
end

function test_attrib_global()
    a = attrib.create('global', {})
    eressea.write_game('attrib.dat')
    eressea.free_game()
    eressea.read_game('attrib.dat')
end

function test_attrib()
    local r = region.create(0,0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    data = { arr = { 'a', 'b', 'c' }, name = 'familiar', events = { die = 'familiar_died' }, data = { mage = u2 } }
    a = { 'a' }
    b = { 'a' }
    uno = u.id
    u2no = u2.id
    a = attrib.create(u, 12)
    a = attrib.create(u, "enno")
    a = attrib.create(u, u2)
    a = attrib.create(u, data)
    eressea.write_game("attrib.dat")
    eressea.free_game()
    eressea.read_game("attrib.dat")
    u = get_unit(uno)
    u2 = get_unit(u2no)
    assert_false(has_attrib(u, 42))
    assert_true(has_attrib(u, "enno"))
    assert_true(has_attrib(u, 12))

    for a in u.attribs do
        x = a.data
        if (type(x)=="table") then
            assert_equal('a', x.arr[1])
            assert_equal('familiar', x.name)
            assert_equal('familiar_died', x.events.die)
            assert_equal(u2, x.data.mage)
            break
        end
    end
end

