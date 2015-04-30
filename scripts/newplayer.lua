function seed(r, email, race, lang)
    local f = faction.create(email, race, lang)
    local u = unit.create(f, r)
    u:set_skill("perception", 30)
    u:add_item("money", 10000)
    items = {
    log = 50,
    stone = 50,
    iron = 50,
    laen = 10,
    mallorn = 10,
    skillpotion = 5
    }
    for it, num in pairs(items) do
        u:add_item(it, num)
    end
    skills ={
    "crossbow",
    "mining",
    "bow",
    "building",
    "trade",
    "forestry",
    "catapult",
    "herbalism",
    "training",
    "riding",
    "armorer",
    "shipcraft",
    "melee",
    "sailing",
    "polearm",
    "espionage",
    "quarrying",
    "roadwork",
    "tactics",
    "stealth",
    "entertainment",
    "weaponsmithing",
    "cartmaking",
    "taxation",
    "stamina"
    }
    for _, sk in ipairs(skills) do
        u = unit.create(f, r, 5)
        u:set_skill(sk, 15)
    end
    return f
end

turn = 923
dofile("config.lua")
p = require("populate")
eressea.read_game(("%d.dat"):format(turn))
best = { score = 0, r = nil }
limit = 30000
sel = p.select(regions(), limit)
for _, r in ipairs(sel) do
    score = p.score(r)
    if score > best.score then
        best.r = r
        best.score = score
    end
    print(score, r, r.terrain)
end
-- print(best.r, best.score)
math.randomseed(os.time())

print(#sel, limit)

players = {
{ email = "Reinhardt.Karnapke@Freenet.de", race = "dwarf",  lang = "de" }
}

for _, p in ipairs(players) do
    local index = math.random(#sel)
    local start = nil
    while not start or start.units() do
        start = sel[index]
    end
    f = seed(start, p.email, p.race or "human", p.lang or "de")
    print(f, start)
    init_reports()
    write_report(f)
end

eressea.write_game(("%d.dat.new"):format(turn))

