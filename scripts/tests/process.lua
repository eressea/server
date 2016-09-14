require "lunit"

module("tests.process", package.seeall, lunit.testcase)

local u, r, f,turn

function setup()
    eressea.free_game()
    r = region.create(0, 0, "plain")
    f = faction.create("bernd@eressea.de", "human", "de")
    u = unit.create(f, r, 1)
    u:add_item("money", 10)
    turn = get_turn()
end

local function file_exists(name)
    local f=io.open(name,"r")
    if f~=nil then io.close(f) return true else return false end
end

local function assert_file(filename)
    assert_equal(true, file_exists(filename))
    os.remove(filename)
end

function test_process_turn()
    u:add_order("NUMMER PARTEI 777")
    process_orders()
    assert_equal(0, init_reports())
    assert_equal(0, write_reports())
    assert_equal(0, eressea.write_game("test.dat"))
    assert_file("data/test.dat")
    -- assert_file("reports/" .. get_turn() .. "-ii.nr")
    -- assert_file("reports/" .. get_turn() .. "-ii.cr")
    -- assert_file("reports/" .. get_turn() .. "-ii.txt")
    -- assert_file("reports/" .. get_turn() .. "-777.nr")
    -- assert_file("reports/" .. get_turn() .. "-777.cr")
    -- assert_file("reports/" .. get_turn() .. "-777.txt")
    assert_file("reports/reports.txt")
    os.remove("reports")
    os.remove("data")
    assert_equal(turn+1, get_turn())
end
