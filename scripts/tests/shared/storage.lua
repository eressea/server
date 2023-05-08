local tcname = 'tests.shared.storage'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
end

function test_store_unit()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply15@eressea.de", "de")
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
    f = faction.create("human", "noreply16@eressea.de", "de")
    f.id = fid
    store = storage.create(filename, "rb")
    assert_not_nil(store)
    u = store:read_unit()
    assert_not_nil(u)
    assert_equal(f, u.faction)
    assert_equal(nil, u.region)
    store:close()
    os.remove(filename)
    assert_not_nil(u)
    assert_equal(u:get_item("money"), u.number * 100)
    os.remove(filename)
end
