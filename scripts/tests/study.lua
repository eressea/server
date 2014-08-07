require "lunit"

module("tests.eressea.study", package.seeall, lunit.testcase)

function setup()
    conf = [[{
    "races" : { "human" : {} },
    "terrains" : { "plain" : { "flags" : [ "land" ] } },
    "keywords" : { "de" : { "study": "LERNEN" } },
    "skills" : { "de": { "alchemy" : "Alchemie", "crossbow" : "Armbrust" } }
    }]]
    eressea.game.reset()
    eressea.config.reset();
    eressea.settings.set('rules.magic.playerschools', '')
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
