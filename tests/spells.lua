require "lunit"

module("tests.spells", package.seeall, lunit.testcase)

function setup()
    eressea.game.reset()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    conf = [[{
        "races": {
            "human" : {}
        },
        "terrains" : {
            "plain": { "flags" : [ "land", "walk", "sail" ] }
        },
        "spells" : {
            "resist_magic" : {
                "index" : 97,
                "parameters" : "u+"
            }
        },
        "keywords" : {
            "de" : {
                "cast" : "ZAUBERE"
            }
        },
        "strings" : {
            "de" : {
                "harbour" : "Hafen"
            }
        }
    }]]

    eressea.config.reset()
    assert(eressea.config.parse(conf)==0)
end

function test_antimagic_visibility()
    local r = region.create(0, 0, "plain")
    local f1 = faction.create("test@example.com", "human", "de")
    local mage = unit.create(f1, r, 1)
    local target = unit.create(f1, r, 1)
    mage:set_skill("magic", 10)
    mage:add_spell("resist_magic")
    mage:add_order("ZAUBERE Antimagie " .. target.id)
    process_orders()
end
