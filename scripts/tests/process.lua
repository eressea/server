require "lunit"

module("tests.process", package.seeall, lunit.testcase)

local u, r, f, turn

function setup()
    eressea.free_game()
    turn = get_turn()
    r = region.create(0, 0, "plain")
    f = faction.create("human", "bernd@eressea.de", "de")
    u = unit.create(f, r, 1)
    u:add_item("money", 10)
end

local function file_exists(name)
    local f=io.open(name,"r")
    if f~=nil then io.close(f) return true else return false end
end

local function assert_file(filename, exists)
    if exists == nil then exists = true end
    assert_equal(exists, file_exists(filename))
    os.remove(filename)
end

function test_process_turn()
    u:add_order("NUMMER PARTEI 777")
    process_orders()
    assert_equal(0, init_reports())
    assert_equal(0, write_reports())
    assert_equal(0, eressea.write_game("test.dat"))
    assert_file("data/test.dat")
    assert_file("reports/" .. get_turn() .. "-777.nr")
    assert_file("reports/" .. get_turn() .. "-777.cr")
    assert_file("reports/" .. get_turn() .. "-777.txt")
    assert_file("reports/reports.txt")
    os.remove("reports")
    os.remove("data")
    assert_equal(turn+1, get_turn())
end
