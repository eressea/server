local tcname = 'tests.shared.economy'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("study.produceexp", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4") -- FOOD_IS_FREE
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("rules.peasants.growth.factor", "0")
end

function test_work()
    local r = region.create(0, 0, "plain")
    r:set_resource('tree', 0)
    r:set_resource('seed', 0)
    r:set_resource('sapling', 0)
    r:set_resource('peasant', 100)
    r:set_resource('money', 0)
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_order("ARBEITE")
    process_orders()
    assert_equal(10, u:get_item('money'))
    assert_equal(100, r:get_resource('money'))
end

function test_bug_2361_forget_magic()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    local uf = unit.create(f, r, 1)
    u:clear_orders()
    u:add_order("VERGESSE Magie")
    u:set_skill('magic', 5)
    uf.race = 'unicorn'
    u.familiar = uf
    process_orders()
    assert_equal(0, u:get_skill('magic'))
    assert_nil(u.familiar)
    -- without a mage, familiars become ghosts:
    assert_equal('ghost', uf.race_name)
    assert_equal(0, uf:get_skill('magic'))
end

function test_bug_2361_familiar_cannot_forget_magic_()
    -- https://bugs.eressea.de/view.php?id=2361
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    local uf = unit.create(f, r, 1)
    u:clear_orders()
    u:set_skill('magic', 5)
    uf.race = 'unicorn'
    uf:clear_orders()
    uf:add_order("VERGESSE Magie")
    uf:set_skill('magic', 5)
    u.familiar = uf
    process_orders()
    -- familiars cannot forget magic:
    assert_equal(5, uf:get_skill('magic'))
end

function test_mine_bonus()
    local r = region.create(0, 0, "mountain")
    r:set_resource("iron", 100)
    local level = r:get_resourcelevel("iron")
    assert_equal(1, level)
    local u = unit.create(faction.create("human"), r)
    u.number = 10
    u:set_skill("mining", 1)
    u:add_order("MACHE EISEN")
    process_orders()
    assert_equal(10, u:get_item("iron"))
    assert_equal(90, r:get_resource("iron"))

    u.building = building.create(r, "mine")
    u.building.size = 10
    u:add_item("money", 500) -- maintenance
    process_orders()
    assert_equal(30, u:get_item("iron"))
    assert_equal(80, r:get_resource("iron"))
end

function test_smithy_bonus()
    local r = region.create(0, 0, "mountain")
    local u = unit.create(faction.create("human"), r)
    u:set_skill("weaponsmithing", 5)
    u:add_item("iron", 20)
    u:add_order("MACHE SCHWERT")
    process_orders()
    assert_equal(1, u:get_item('sword'))
    assert_equal(19, u:get_item('iron'))

    u.building = building.create(r, "smithy")
    u.building.size = 10
    u:add_item("money", 300) -- maintenance
    u:add_item("log", 1) -- maintenance
    process_orders()
    assert_equal(3, u:get_item('sword'))
    assert_equal(18, u:get_item('iron'))
end

function test_no_guards()
    local r = region.create(0, 0, "plain")
    r:set_resource("tree", 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill("forestry", 1)
    u:add_order("MACHE HOLZ")
    process_orders()
    assert_equal(1, u:get_item("log"))
    process_orders()
    assert_equal(2, u:get_item("log"))
end

function test_elf_guards_trees()
    local r = region.create(0, 0, "plain")
    r:set_resource("tree", 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill("forestry", 1)
    local guard = unit.create(faction.create("elf"), r, 1, "elf")
    guard:add_order("BEWACHEN") -- fails, because unarmed
    u:add_order("MACHE HOLZ")
    process_orders()
    assert_equal(1, u:get_item("log"))
    guard:add_item("sword", 1)
    guard:set_skill("melee", 1)
    guard:add_order("BEWACHEN") -- success
    process_orders()
    -- GUARD starts after MAKE:
    assert_equal(2, u:get_item("log"))
    process_orders()
    -- GUARD was active this turn:
    assert_equal(2, u:get_item("log"))
end

function test_catapults_dont_guard()
    local r = region.create(0, 0, "plain")
    r:set_resource("tree", 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill("forestry", 1)
    local guard = unit.create(faction.create("elf"), r, 1, "elf")
    guard:add_order("BEWACHEN")
    u:add_order("MACHE HOLZ")
    process_orders()
    assert_equal(1, u:get_item("log"))
    guard:add_item("catapult", 1)
    guard:set_skill("catapult", 1)
    guard:add_order("BEWACHEN")
    process_orders()
    -- GUARD starts after MAKE:
    assert_equal(2, u:get_item("log"))
    process_orders()
    -- GUARD was active this turn, but catapults do not count:
    assert_equal(3, u:get_item("log"))
end

function test_ent_guards_trees()
    local r = region.create(0, 0, "plain")
    r:set_resource("tree", 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill("forestry", 1)
    local guard = unit.create(get_monsters(), r, 1, "ent")
    guard:add_order("BEWACHEN")
    u:add_order("MACHE HOLZ")
    process_orders()
    assert_equal(1, u:get_item("log"))
    process_orders()
    assert_equal(1, u:get_item("log"))
end

function test_guard_stops_recruit()
    local r = region.create(0, 0, "plain")
    r:set_resource("peasant", 100)
    local u = unit.create(faction.create("human"), r)
    local guard = unit.create(get_monsters(), r, 1)
    guard:add_order("BEWACHEN")

    u:add_item("money", 1000)
    assert_equal(1, u.number)
    u:add_order("REKRUTIERE 1")
    process_orders()
    assert_equal(2, u.number)
    u:add_order("REKRUTIERE 1")
    process_orders()
    assert_equal(2, u.number)
end

function test_guard_protects_iron()
    local r = region.create(0, 0, "plain")
    r:set_resource("iron", 100)
    local level = r:get_resourcelevel("iron")
    local u = unit.create(faction.create("human"), r)
    u:set_skill("mining", level)
    local guard = unit.create(get_monsters(), r, 1)
    guard:add_order("BEWACHEN")

    u:add_order("MACHE EISEN")
    process_orders()
    assert_equal(level, u:get_item("iron"))
    process_orders()
    assert_equal(level, u:get_item("iron"))
end

function test_ironkeeper_guards_iron()
    local r = region.create(0, 0, "plain")
    r:set_resource("iron", 100)
    local level = r:get_resourcelevel("iron")
    local u = unit.create(faction.create("human"), r)
    u:set_skill("mining", level)
    u:set_skill("stealth", 1) -- does not help against ironkeeper
    local guard = unit.create(faction.create("mountainguard"), r, 1, "mountainguard")
    guard:add_order("BEWACHEN")
    u:add_order("MACHE EISEN")
    process_orders()
    assert_equal(level, u:get_item("iron"))
    process_orders()
    assert_equal(level, u:get_item("iron"))
end

-- bug 2679
function test_stealthy_iron_producer()
    local r = region.create(0, 0, "plain")
    r:set_resource("iron", 100)
    local level = r:get_resourcelevel("iron")
    local u = unit.create(faction.create("human"), r)
    u:set_skill("stealth", 1)
    u:set_skill("mining", level)
    local guard = unit.create(faction.create("human"), r, 1, "human")
    guard:add_order("BEWACHEN")
    u:add_order("MACHE EISEN")
    process_orders()
    assert_equal(level, u:get_item("iron"))
    process_orders()
    assert_equal(2 * level, u:get_item("iron"))
end

function test_sawmill()
    local r = region.create(0, 0, "plain")
    r:set_resource("tree", 100)
    local u = unit.create(faction.create("human"), r)
    u:add_item("money", 250) -- sawmill maintenance
    u:set_skill("forestry", 6)
    u.building = building.create(r, "sawmill")
    u.building.size = 1
    u:add_order("MACHE 6 HOLZ")
    process_orders()
    assert_equal(6, u:get_item("log"))
    assert_equal(97, r:get_resource("tree"))
end
