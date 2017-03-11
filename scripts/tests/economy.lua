require "lunit"

module("tests.economy", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("study.produceexp", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4") -- FOOD_IS_FREE
    eressea.settings.set("rules.encounters", "0")
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
    local guard = unit.create(faction.create("mountainguard"), r, 1, "mountainguard")
    guard:add_order("BEWACHEN")
    u:add_order("MACHE EISEN")
    process_orders()
    assert_equal(level, u:get_item("iron"))
    process_orders()
    assert_equal(level, u:get_item("iron"))
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

function test_ent_guards_trees()
    local r = region.create(0, 0, "plain")
    r:set_resource("tree", 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill("mining", 1)
    local guard = unit.create(get_monsters(), r, 1, "ent")
    u:set_skill("forestry", 1)
    guard:clear_orders()
    u:clear_orders()

    guard:add_order("BEWACHEN")
    u:add_order("MACHE HOLZ")
    process_orders()
    assert_equal(1, u:get_item("log"))
    process_orders()
    assert_equal(1, u:get_item("log"))
end
