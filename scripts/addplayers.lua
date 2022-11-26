require 'eressea.path'

require 'config'
config.autoseed = 1
local auto = require('eressea.autoseed')

local tile7 = {
	xof = { x = 3, y = -1 },
	yof = { x = 1, y = 2 },
	length = 2,
	hexes = {
		{ x = 0, y = 0 },
		{ x = -1, y = 0 },
		{ x = -1, y = 1 },
		{ x = 0, y = 1 },
		{ x = 1, y = 0 },
		{ x = 1, y = -1 },
		{ x = 0, y = -1 }
	}
}

local tile19 = {
	xof = { x = 5, y = -2 },
	yof = { x = 2, y = 3 },
	length = 3,
	hexes = {
		{ x = 0, y = 0 },

		{ x = -1, y = 0 },
		{ x = -1, y = 1 },
		{ x = 0, y = 1 },
		{ x = 1, y = 0 },
		{ x = 1, y = -1 },
		{ x = 0, y = -1 },

		{ x = -2, y = 0 },
		{ x = -2, y = 1 },
		{ x = -2, y = 2 },
		{ x = -1, y = 2 },
		{ x = 0, y = 2 },
		{ x = 1, y = 1 },
		{ x = 2, y = 0 },
		{ x = 2, y = -1 },
		{ x = 2, y = -2 },
		{ x = 1, y = -2 },
		{ x = 0, y = -2 },
		{ x = -1, y = -1 }
	}
}

local pattern = tile19

local function random_terrain()
    return "glacier"
end

local function tile_create(tile_x, tile_y, terrain)
    for _, hex in ipairs(pattern.hexes) do
        local t = terrain or random_terrain()
        region.create(tile_x + hex.x, tile_y + hex.y, t)
    end
end

local function tile_center(tile_x, tile_y)
    local x = tile_x * pattern.xof.x + tile_y * pattern.yof.x
    local y = tile_x * pattern.xof.y + tile_y * pattern.yof.y
    return get_region(x, y)
end

players = auto.read_players()
if #players == 0 then
    print("ERROR: no new players found")
end

local turn = get_turn()
if turn > 0 then
    eressea.read_game(turn .. ".dat")
else
    tile_create(0, 0)
end

local Tiles = require('tiles')

local function scan_tiles(world)
    local queue = { { 0, 0 } }
    local avail = {}
    while #queue > 0 do
        local tile_x, tile_y = table.unpack(queue[1])
        table.remove(queue, 1)
        local r = tile_center(tile_x, tile_y)
        local age = 0
        if r then
            for adj = 2, 7 do
                local hex = pattern.hexes[adj]
                local adj_x = tile_x + hex.x
                local adj_y = tile_x + hex.y
                local exist = world:get(adj_x, adj_y)
                if exist then
                    if r then
                        if exist.max_age < r.age then
                            exist.max_age = r.age
                        end
                    end
                else
                    table.insert(queue, { adj_x, adj_y })
                end
            end
            age = r.age
        else
            table.insert(avail, { tile_x, tile_y } )
        end
        world:add(tile_x, tile_y, { region = r, max_age = age })
    end
    return avail
end

local world = Tiles()
avail = scan_tiles(world)

for _, tile in ipairs(avail) do
    local x, y = table.unpack(tile)
    local tile = world:get(x, y)
    print(x, y, tostring(tile.region), tile.max_age)
end

-- eressea.write_game(turn .. ".dat")
