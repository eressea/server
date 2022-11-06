local warp = {}

local warps = {}

function warp.init()
    local r, b
    for r in regions() do
        local uid = r:get_key('warp')
        if uid then
            local target = get_region_by_id(uid)
            if target then
                table.insert(warps, {from = r, to = target})
            end
        end
    end
end

function move_all(from, to)
    -- move all units not in a ship
    local units = {}
    for u in from.units do
        if u.ship == nil and u.faction ~= 666 then
            table.insert(units, u)
        end
    end
    for _, u in ipairs(units) do
        u.region = to
    end

    -- move all non-empty ships and units in them
    local ships = {}
    for s in from.ships do
        table.insert(ships, s)
    end
    for _, s in ipairs(ships) do
        local units = {}
        for u in s.units do
            table.insert(units, u)
        end
        s.region = to
        s.coast = nil
        for _, u in ipairs(units) do
            u.region = to
            u.ship = s
        end
    end
end

function warp.update()
    for _, w in ipairs(warps) do
        print("warp from " .. tostring(w.from) .. " to " .. tostring(w.to))
        move_all(w.from, w.to)
    end
end

function warp.mark(r, destination)
    r:set_key('warp', destination.id)
end

function warp.gm_edit()
    local sel = { nil, nil }
    local r
    local i = 1
    local destination = gmtool.get_cursor()
    for r in gmtool.get_selection() do
        r:set_key('warp', destination.id)
    end
end

return warp
