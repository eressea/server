local function get_tile(obj, x, y)
    local row = obj._index[y]
    if row then
        local i = obj._index[y][x]
        if i then
            return obj._tiles[i]
        end
    end
    return nil
end

local function add_tile(obj, x, y, data)
    if not obj._index[y] then obj._index[y] = {} end
    if not obj._index[y][x] then
        table.insert(obj._tiles, data)
        obj._index[y][x] = #obj._tiles
    end
end

local function all_tiles(obj)
    local index = 0
    local count = #obj._tiles
    return function()
        index = index + 1
        if index <= count then
            return obj._tiles[index]
        end
        return nil
    end
end

return function()
    local obj = {}
    obj.get = get_tile
    obj.add = add_tile
    obj.all = all_tiles
    obj._tiles = {}
    obj._index = {}
    return obj
end
