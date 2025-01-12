local tcname = 'tests.e2.economy'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("study.produceexp", "0")
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("rules.peasants.growth.factor", "0")
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

function test_make_limited()
    local r = region.create(0, 0, "mountain")
    r:set_resource("iron", 100)
    local level = r:get_resourcelevel("iron")
    assert_equal(1, level)
    local u = unit.create(faction.create("human"), r)
    u.number = 10
    u:set_skill("mining", 1)
    u:add_order("MACHE 5 EISEN")
    process_orders()
    assert_equal(5, u:get_item("iron"))
    assert_equal(95, r:get_resource("iron"))
end

function test_guards_block_forestry()
    local r = region.create(0, 0, "plain")
    r:set_resource("tree", 100)
    local u = unit.create(faction.create("human"), r)
    u:set_skill("forestry", 1)
    local guard = unit.create(faction.create("human"), r)
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

function test_guards_stop_recruiting()
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

function test_bug_2361_forget_magic()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    local uf = unit.create(f, r, 1)
    u:clear_orders()
    u:add_order("VERGESSE Magie")
    u:set_skill('magic', 5)
    uf.race = 'lynx'
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
    uf.race = 'lynx'
    uf:clear_orders()
    uf:add_order("VERGESSE Magie")
    uf:set_skill('magic', 5)
    u.familiar = uf
    process_orders()
    -- familiars cannot forget magic:
    assert_equal(5, uf:get_skill('magic'))
end

function test_trade_limits()
    local r = region.create(0, 0, "plain")
    local f = faction.create("elf")
    local u = unit.create(f, r, 1)
    u.building = building.create(r, "castle", 2)
    u:set_skill("trade", 7)
    local lux = "jewel"
    local loc = "Juwel"
    if lux == r.luxury then
        lux = "balm"
        loc = "Balsam"
    end
    u:add_item(lux, 80)
    u:set_orders("VERKAUFE 80 " .. loc)
    process_orders()
    assert_equal(10, u:get_item(lux))
end

-- MACHE 3 Schwert verhält sich anders als MACHE 3 Burg/Schiff
-- @see test_repeated_create_n_building
function test_repeated_create_n_items()
    local r = region.create(0,0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:set_skill("weaponsmithing", 3)
    u:add_item("iron", 50)
    u:set_orders("MACHE 3 Schwert")
    assert_equal("MACHE 3 Schwert", u.orders[1])
    process_orders()
    assert_equal(1, u:get_item("sword"))
    assert_equal("MACHE 3 Schwert", u.orders[1])
end

-- MACHE n BURG countdown
function test_repeated_create_n_building()
    local r = region.create(0,0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:set_skill("building", 3)
    u:add_item("stone", 50)
    u:set_orders("MACHE 7 Burg")
    assert_equal("MACHE 7 Burg", u.orders[1])
    process_orders()
    assert_equal(3, u.building.size)
    assert_equal("MACHE 4 Burg " .. itoa36(u.building.id), u.orders[1])
    process_orders()
    assert_equal(6, u.building.size)
    assert_equal("MACHE 1 Burg " .. itoa36(u.building.id), u.orders[1])
    process_orders()
    assert_equal(7, u.building.size)
    assert_equal("ARBEITE", u.orders[1])
end

-- MACHE n SCHIFF has no countdown
function test_repeated_create_n_ship()
    local r = region.create(0,0, "plain")
    local f = faction.create("insect")
    local u = unit.create(f, r, 1)
    u:set_skill("shipcraft", 1)
    u:add_item("log", 50)
    u:set_orders("MACHE 3 Boot")
    assert_equal("MACHE 3 Boot", u.orders[1])
    process_orders()
    assert_equal(1, u.ship.size)
    assert_equal("MACHE SCHIFF " .. itoa36(u.ship.id), u.orders[1])
end
