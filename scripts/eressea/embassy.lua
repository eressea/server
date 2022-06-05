-- Ring für die Treueschwur

function use_ring_of_levitation(u, amount, token, ord)
    if not u.ship then
        return -331 -- error331: Die Einheit befindet sich nicht auf einem Schiff
    end
    local seeds = u:get_pooled('mallornseed')
    if seeds == 0 then
        local msg = message.create('resource_missing')
        msg:set_unit('unit', u)
        msg:set_region('region', u.region)
        msg:set_order('command', ord)
        msg:set_resource('missing', 'mallornseed')
        msg:send_faction(u.faction)
        return 0
    end
    if seeds > 0 then
        if seeds > 2 then seeds = 2 end
        u:use_pooled('mallornseed', seeds)
        levitate_ship(u.ship, u, 1, seeds)
        local msg = message.create('flying_ship_result')
        msg:set_unit('mage', u)
        msg:set_ship('ship', u.ship)
        msg:send_region(u.region)
        return 0 -- the ring has unlimited charges
    end
    return 0
end

-- Muschelplateau

if not config.embassy or config.embassy==0 then return nil end

local embassy = {}
local home = nil

-- global exports (use item)
function use_seashell(u, amount)
-- Muschelplateau...
    local visit = u.faction:get_key('mupL')
    if visit and u.region~= home then
        local turns = get_turn() - visit
        local msg = message.create('msg_event')
        msg:set_string("string", u.name .. " (" .. itoa36(u.id) .. ") erzählt den Bewohnern von " .. u.region.name .. " von Muschelplateau, das die Partei " .. u.faction.name .. " vor " .. turns .. " Wochen besucht hat." )
        msg:set_unit('unit', u)
        msg:set_region('region', u.region)
        msg:send_region(u.region)
        return 0
    end
    return -4
end

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
    eressea.log.info("updating embassies in " .. tostring(home))
    local u
    for u in home.units do
        if not u.faction:get_key('mupL') then
            if u.faction:add_item('seashell', 1) > 0 then
                eressea.log.info("new seashell for " .. tostring(u.faction))
                u.faction:set_key('mupL', get_turn())
            end
        end
    end
end

return embassy
