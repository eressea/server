local function is_winter(turn)
    local season = get_season(turn)
    return season == "winter"
end

local function is_spring(turn)
    local season = get_season(turn)
    return season == "spring"
end

local function freeze(r, chance)
    for i, rn in ipairs(r.adj) do
        -- each region has a chance to freeze
        if rn.terrain=="ocean" and (chance>=100 or math.fmod(rng_int(), 100)<chance) then
            rn.terrain = "packice"
        end
    end
end

local function thaw(r, chance)
    if chance>=100 or math.fmod(rng_int(), 100)<chance then
        r.terrain = "ocean"
        for s in r.ships do
            s.coast = nil
        end
    end
end

local frost = {}
function frost.update()
    local turn = get_turn()
    if is_winter(turn) then
        for r in regions() do
            if r.terrain=="glacier" then
                freeze(r, 20)
            end
        end
    elseif is_spring(turn) then
        for r in regions() do
            if r.terrain=="packice" then
                thaw(r, 20)
            end
        end
    elseif is_spring(turn-1) then
        for r in regions() do
            if r.terrain=="packice" then
                thaw(r, 100)
            end
        end
    end
end
return frost
