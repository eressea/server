local tcname = 'tests.e2.undead'
local lunit = require("lunit")
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, 'seeall')
end

function setup()
    eressea.free_game()
end

function test_undead_reserve_self()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human")
    local u1 = unit.create(f1, r1, 1)
    u1.race = "undead"
    u1:clear_orders()
    u1:add_item("log", 2)
    u1:add_order("RESERVIERE 1 Holz")
    u1:add_order("GIB 0 ALLES Holz")
    process_orders()
    assert_equal(1, u1:get_item("log"))
end

function test_undead_reserve_other()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human")
    local u1 = unit.create(f1, r1, 1)
    local u2 = unit.create(f1, r1, 1)
    u2:add_item("log", 2)
    u1.race = "undead"
    u1:clear_orders()
    u1:add_order("RESERVIERE 1 Holz")
    process_orders()

    if 0 ~= u1:get_item("log") then
        -- try to catch that intermittent bug:
        print(u1:show())
    end
    assert_equal(0, u1:get_item("log"))
    if 2 ~= u2:get_item("log") then
        -- try to catch that intermittent bug:
        print(u2:show())
    end
    assert_equal(2, u2:get_item("log"))
end

function test_undead_give_item()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human", "hodor@eressea.de", "de")
    local u1 = unit.create(f1, r1, 1)
    u1.race = "undead"
    u1:clear_orders()
    u1:add_item("log", 1)
    u1:add_order("GIB 0 1 Holz")
    process_orders()
    assert_equal(0, u1:get_item("log"))
end

function test_clones_dont_give_person()
    local r1 = region.create(0, 0, "plain")
    local f1 = faction.create("human", "hodor@eressea.de", "de")
    local u1 = unit.create(f1, r1, 2)
    u1.race = "clone"
    u1:clear_orders()
    u1:add_item("log", 1)
    u1:add_order("GIB 0 1 Person")
    process_orders()
    assert_equal(2, u1.number)
end

-- bug 2504
function test_skeleton_cannot_learn()
    local r = region.create(0, 0, "plain")
    local f = faction.create("elf")
    local u = unit.create(f, r, 2)
    u.race = "skeleton"
    u:add_order("LERNE Wahrnehmung")
    process_orders()
    assert_equal(0, u:get_skill('perception'))
end
