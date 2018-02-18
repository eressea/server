require "lunit"

module("tests.process", package.seeall, lunit.testcase)

local u, r, f

function setup()
    eressea.free_game()
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
    turn_begin()
    turn = get_turn()
    turn_process()
    turn_end()
    assert_equal(turn, get_turn())
    turn_begin()
    assert_equal(turn+1, get_turn())
    turn_process()
    turn_end()
end

function test_write_reports()
    turn_begin()
    turn = get_turn()
    u:add_order("NUMMER PARTEI 777")
    turn_process()
    assert_equal(0, init_reports())
    assert_equal(0, write_reports())
    assert_file("reports/" .. turn .. "-777.nr")
    assert_file("reports/" .. turn .. "-777.cr")
    assert_file("reports/" .. turn .. "-777.txt")
    assert_file("reports/reports.txt")
    os.remove("reports")
    assert_equal(0, eressea.write_game("test.dat"))
    assert_file("data/test.dat")
    os.remove("data/test.dat")
    turn_end()
end
