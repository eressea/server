require "lunit"

module("tests.e3.spells", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("magic.fumble.enable", "0")
end

function test_blessedharvest_lasts_n_turn()
    eressea.free_game()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "halfling", "de")
    local u = unit.create(f, r)
    local err = 0
    r:set_resource("peasant", 100)
    r:set_resource("money", 0)
    u:add_item("money", 1000)
    u.magic = "gwyrrd"
    u.race = "dwarf"
    u:set_skill("magic", 20)
    u.aura = 200
    err = err + u:add_spell("raindance")
    err = err + u:add_spell("blessedharvest")
    assert_equal(0, err)
    
    u:clear_orders()
    local level = 5
    u:add_order("ZAUBERE STUFE " .. level .. " Regentanz")
    assert_equal(0, r:get_resource("money"), 0)
    
    local m = 0
    local p = 100
    for i=1,level+2 do
        process_orders()
        local income = p * 12
        p = r:get_resource("peasant")
        income = income - p * 10
        m = m + income
        -- print(i, m, p, r:get_resource("money"))
        if (i>level+1) then
            assert_not_equal(m, r:get_resource("money"))
        else
            assert_equal(m, r:get_resource("money"))
        end
        u:clear_orders()
        u:add_order("ARBEITEN")
    end
end
