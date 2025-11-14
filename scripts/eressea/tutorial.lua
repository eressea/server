local self = {}

local function random_terrain()
    if rng_int() % 3 < 2 then
        return 'plain'
    end
    return 'ocean'
end

self.make_island = function(x_origin, y_origin, radius)
    local barrier = radius + 1
    for x = -barrier, barrier do
        for y = -barrier, barrier do
            local d = distance(0, 0, x, y)
            local ter = nil
            if d == barrier then ter = 'barrier'
            elseif d == radius then ter = 'ocean'
            elseif d < radius then
                ter = random_terrain()
            end
            if ter then
                local r = region.create(x_origin + x, y_origin + y, ter)
            end
        end
    end
end

return self
