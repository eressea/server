local this = {}

local function score(r, res)
    assert(r)
    res = res or "peasant"
    local x, y, rn
    local peas = r:get_resource(res)
    for _, rn in pairs(r.adj) do
        if rn then
            peas = peas + rn:get_resource(res)
        end
    end
    return peas
end

local function select(regions, limit)
    local sel = {}
    for r in regions do
        if r.terrain~="ocean" and r.units()==nil then
            s = score(r)
            if s >= limit then
                table.insert(sel, r)
            end
        end
    end
    return sel
end

return {
    score = score,
    select = select
}
