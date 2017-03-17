require "lunit"

module("tests.report", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4")
end

local function find_in_report(f, pattern, extension)
    extension = extension or "nr"
    local filename = config.reportpath .. "/" .. get_turn() .. "-" .. itoa36(f.id) .. "." .. extension
    local report = io.open(filename, 'r');
    assert_not_nil(report)
    t = report:read("*all")
    report:close()

    local start, _ = string.find(t, pattern)
    return start~=nil
end

local function remove_report(faction)
    local filetrunk = config.reportpath .. "/" .. get_turn() .. "-" .. itoa36(faction.id)
    os.remove(filetrunk .. ".nr")    
    os.remove(filetrunk .. ".cr")    
    os.remove(filetrunk .. ".txt")    
end

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4")
end

function test_coordinates_unnamed_plane()
    local p = plane.create(0, -3, -3, 7, 7)
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human", "unnamed@eressea.de", "de")
    local u = unit.create(f, r, 1)
    init_reports()
    write_report(f)
    assert_true(find_in_report(f, r.name .. " %(0,0%), Berg"))
    remove_report(f)
end

function test_coordinates_no_plane()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human", "noplane@eressea.de", "de")
    local u = unit.create(f, r, 1)
    init_reports()
    write_report(f)
    assert_true(find_in_report(f, r.name .. " %(0,0%), Berg"))
    remove_report(f)
end

function test_show_shadowmaster_attacks()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    u.race = "shadowmaster"
    u:clear_orders()
    u:add_order("ZEIGE Schattenmeister")
    process_orders()
    init_reports()
    write_report(f)
    assert_false(find_in_report(f, ", ,"))
    remove_report(f)
end

function test_coordinates_named_plane()
    local p = plane.create(0, -3, -3, 7, 7, "Hell")
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    init_reports()
    write_report(f)
    assert_true(find_in_report(f, r.name .. " %(0,0,Hell%), Berg"))
    remove_report(f)
end

function test_coordinates_noname_plane()
    local p = plane.create(0, -3, -3, 7, 7, "")
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    local u = unit.create(f, r, 1)
    init_reports()
    write_report(f)
    assert_true(find_in_report(f, r.name .. " %(0,0%), Berg"))
    remove_report(f)
end

function test_lighthouse()
    eressea.free_game()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("human", "noreply@eressea.de", "de")
    region.create(1, 0, "mountain")
    region.create(2, 0, "ocean")
    region.create(0, 1, "firewall")
    region.create(3, 0, "mountain")
    region.create(4, 0, "plain")
    local u = unit.create(f, r, 1)
    local b = building.create(r, "lighthouse")
    b.size = 100
    b.working = true
    u.building = b
    u:set_skill("perception", 9)
    u:add_item("money", 1000)
    assert_not_nil(b)

    init_reports()
    write_report(f)
    assert_true(find_in_report(f, " %(1,0%) %(vom Turm erblickt%)"))
    assert_true(find_in_report(f, " %(2,0%) %(vom Turm erblickt%)"))
    assert_true(find_in_report(f, " %(3,0%) %(vom Turm erblickt%)"))

    assert_false(find_in_report(f, " %(0,0%) %(vom Turm erblickt%)"))
    assert_false(find_in_report(f, " %(0,1%) %(vom Turm erblickt%)"))
    assert_false(find_in_report(f, " %(4,0%) %(vom Turm erblickt%)"))
    remove_report(f)
end

