local autoseed = {}

-- minimum required resources in the 7-hex neighborhood:
local peasants = 20000
local trees = 1000
-- number of starters per region:
local per_region = 2
    
local function score(r, res)
    assert(r)
    res = res or "peasant"
    local x, y, rn
    local peas = r:get_resource(res)
    for _, rn in pairs(r.adj) do
        if rn and not rn.units() then
            peas = peas + rn:get_resource(res)
        end
    end
    return peas
end

local function select_regions(regions, peasants, trees)
    local sel = {}
    for r in regions do
        if not r.plane and r.terrain~="ocean" and not r.units() then
            if score(r, "peasant") >= peasants and score(r, "tree") >= trees then
                table.insert(sel, r)
            end
        end
    end
    return sel
end

local function read_players()
--    return {{ email = "noreply@mailinator.com", race = "dwarf",  lang = "de" }}
    local players =  {}
    local input = io.open("newfactions", "r")
    while input do
        local str = input:read("*line")
        if str==nil then break end
        local email, race, lang = str:match("([^ ]*) ([^ ]*) ([^ ]*)")
        if string.char(string.byte(email, 1))~='#' then
            table.insert(players, { race = race, lang = lang, email = email })
        end
    end
    return players
end

local function seed(r, email, race, lang)
    local f = faction.create(email, race, lang)
    local u = unit.create(f, r)
    equip_unit(u, "new_faction")
    equip_unit(u, "first_unit")
    equip_unit(u, "first_" .. race, 7) -- disable old callbacks
    unit.create(f, r, 5):set_skill("mining", 30)
    unit.create(f, r, 5):set_skill("quarrying", 30)
    f:set_origin(r)
    return f
end

local function get_faction_by_email(email)
    for f in factions() do
        if f.email == email then
            return f
        end
    end
    return nil
end

function autoseed.init()
    -- local newbs = {}
    local num_seeded = 2
    local start = nil

    eressea.log.info('autoseed new players')
    players = read_players()
    if players and #players > 0 then
        local sel
        eressea.log.info(#players .. ' new players')
        sel = select_regions(regions(), peasants, trees)
    end
    for _, p in ipairs(players) do
        if num_seeded == per_region then
            while not start or start.units() do
                local index = 1 + (rng_int() % #sel)
                start = sel[index]
            end
            num_seeded = 0
        end
        local dupe = get_faction_by_email(p.email)
        if dupe then
            eressea.log.warning("seed: duplicate email " .. p.email .. " already used by faction " .. tostring(f))
        else
            local f = seed(start, p.email, p.race or "human", p.lang or "de")
            num_seeded = num_seeded + 1
            print("new faction ".. tostring(f) .. " starts in ".. tostring(start))
            -- table.insert(newbs, f)
        end
    end
end

return autoseed
