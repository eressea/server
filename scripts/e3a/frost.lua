module('frost', package.seeall)

local function is_winter(turn)
    local season = get_season(turn)
    return season == "calendar::winter"
end

local function freeze(r)
    for i, rn in ipairs(r.adj) do
        -- each region has a chance to freeze
        if rn.terrain=="ocean" and math.mod(rng_int(), 100)<20 then
            print("terraforming .. " .. tostring(rn))
            rn.terrain = "iceberg_sleep"
        end
    end
end

local function thaw(r)
    r.terrain = "ocean"
end

function update()
    local turn = get_turn()
    if is_winter(turn) then
        print "it is winter"
        for r in regions() do
            if r.terrain=="glacier" then
                freeze(r)
            end
        end
    elseif is_winter(turn-1) then
        for r in regions() do
            if r.terrain=="iceberg_sleep" then
                thaw(r)
            end
        end
    end
end
