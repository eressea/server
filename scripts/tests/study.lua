require "lunit"

module("tests.eressea.study", package.seeall, lunit.testcase)

function setup()
    conf = [[{
    "races" : { "human" : {} },
    "terrains" : { "plain" : { "flags" : [ "land" ] } },
    "keywords" : { "de" : { "study": "LERNEN", "teach": "LEHREN" } },
    "skills" : { "de": {
        "tactics" : "Taktik",
        "alchemy" : "Alchemie",
        "crossbow" : "Armbrust"
    } },
    "spells" : { "fireball" : { "syntax" : "u+" } }
    }]]
    eressea.game.reset()
    eressea.config.reset();
    eressea.settings.set('rules.magic.playerschools', '')
    eressea.settings.set("rules.economy.food", "4")
    eressea.settings.set('study.random_progress', '0')
    eressea.config.parse(conf)
end

function test_study()
    local r = region.create(0, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_order("LERNEN Armbrust")
    process_orders()
    assert_equal(1, u:get_skill("crossbow"))
end

function test_study_expensive()
    local r = region.create(0, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u = unit.create(f, r, 1)
    eressea.settings.set("skills.cost.alchemy", "50")
    eressea.settings.set("rules.encounters", "0")
    u:add_order("LERNEN Alchemie")
    u:add_item("money", 50)
    process_orders()
    assert_equal(1, u:get_skill("alchemy"))
    assert_equal(0, u:get_item("money"))
end

function test_unit_spells()
    local r = region.create(0, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u = unit.create(f, r, 1)
    u.magic = "gray"
    u:set_skill("magic", 1)
    u:add_spell("toast")
    assert_equal(nil, u.spells)
    u:add_spell("fireball", 2)
    local sp = u.spells()
    assert_equal("fireball", sp.name)
    assert_equal(2, sp.level)
end

local function make_teacher(student, f, skill)
    f = f or student.faction
    local u = unit.create(f, student.region, 1)
    u:clear_orders()
    u:add_order("LEHRE " .. itoa36(student.id))
    u:set_skill(skill or "crossbow", 10)
    return u
end

local function make_student(f, r, num, skill)
    local u = unit.create(f, r, num or 1)
    u:clear_orders()
    u:add_order("LERNE " .. (skill or "Armbrust"))
    return u
end

function test_study_no_teacher()
    local r = region.create(0, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u1 = make_student(f, r, 1)
    u1:set_skill("crossbow", 1)
    process_orders()
    assert_equal(1, u1:get_skill("crossbow"))
end

function test_study_with_teacher()
    local r = region.create(0, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u1 = make_student(f, r, 1)

    make_teacher(u1)
    u1:set_skill("crossbow", 1)
    process_orders()
    assert_equal(2, u1:get_skill("crossbow"))
end

function test_study_too_many_students()
    local r = region.create(0, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u1 = make_student(f, r, 20, "Taktik")
    u1.name = "Student"
    u1:add_item("money", 201*u1.number)
    make_teacher(u1, f, "tactics")
    process_orders()
    assert_equal(u1.number, u1:get_item("money"))
end

function test_study_multiple_teachers()
    local r = region.create(0, 0, "plain")
    local f = faction.create("test@example.com", "human", "de")
    local u1 = make_student(f, r, 20, "Taktik")
    u1.name = "Student"
    u1:add_item("money", 201*u1.number)
    make_teacher(u1, f, "tactics")
    make_teacher(u1, f, "tactics")
    process_orders()
    assert_equal(u1.number, u1:get_item("money"))
end
