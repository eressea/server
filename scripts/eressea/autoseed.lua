if not config.autoseed or config.autoseed==0 then return nil end
local autoseed = {}

-- minimum required resources in the 7-hex neighborhood:
local peasants = 10000
local trees = 800
-- minimum resources in the region itself:
local min_peasants = 2000
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
            if r:get_resource("peasant") >= min_peasants then
                sp = score(r, "peasant")
                st = score(r, "tree")
                if sp >= peasants then
                    if st >= trees then
                        table.insert(sel, r)
                    end
                end
            end
        end
    end
    return sel
end

local function read_players()
--    return {{ email = "noreply@mailinator.com", race = "dwarf",  lang = "de" }}
    local players =  {}
    local input = io.open("newfactions", "r")
    if input then
        local str = input:read("*line")
        while str do
            if str==nil then break end
            local email, race, lang = str:match("([^ ]*) ([^ ]*) ([^ ]*)")
            if email and string.char(string.byte(email, 1))~='#' then
                table.insert(players, { race = race, lang = lang, email = email })
            end
            str = input:read("*line")
        end
        input:close()
    end
    return players
end

local function seed(r, email, race, lang)
    assert(r)
    local f = faction.create(race, email, lang)
    assert(f)
    local u = unit.create(f, r)
    assert(u)
    equip_unit(u, "seed_faction")
    equip_unit(u, "seed_unit")
    equip_unit(u, "seed_" .. race, 7)
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
    local num_seeded = per_region
    local start = nil

    eressea.log.info('autoseed new players')
    players = read_players()
    if players then
        print('autoseed ' .. #players .. ' new players')
    end
    if players and #players >= per_region then
        local sel
        eressea.log.info(#players .. ' new players')
        sel = select_regions(regions(), peasants, trees)
        if #sel == 0 then
            eressea.log.error("autoseed could not select regions for new factions")
        else
            for _, p in ipairs(players) do
                if num_seeded == per_region then
                    local index = rng_int() % #sel
                    start = nil
                    while not start do
                        start = sel[index + 1]
                        sel[index+1] = nil
                        index = (index + 1) % #sel
                    end
                    num_seeded = 0
                end
                local dupe = get_faction_by_email(p.email)
                if dupe then
                    eressea.log.warning("seed: duplicate email " .. p.email .. " already used by " .. tostring(dupe))
                else
                    print("new faction ".. p.email .. " starts in ".. tostring(start))
                    local f = seed(start, p.email, p.race or "human", p.lang or "de")
                    num_seeded = num_seeded + 1
                end
            end
        end
    end
end

return autoseed
