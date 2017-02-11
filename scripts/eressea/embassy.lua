-- Muschelplateau

if not config.embassy then return nil end

local embassy = {}
local home = nil

function embassy.init()
    home = get_region(165,30)
    if home==nil then
        eressea.log.error("cannot find embassy region 'Muschelplateau'")
    end
end

function embassy.update()
    -- Muschelplateau
    if home==nil then
        return
    end
    eressea.log.debug("updating embassies in " .. tostring(home))
    local u
    for u in home.units do
        if not u.faction:get_key('mupL') then
            if (u.faction:add_item('seashell', 1)>0) then
                eressea.log.debug("new seashell for " .. tostring(u.faction))
                u.faction:set_key('mupL', get_turn())
            end
        end
    end
end

return embassy
