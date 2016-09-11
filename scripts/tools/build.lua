function new_faction(email, race, lang, r)
    f = faction.create(email, race, lang)
    u = unit.create(f, r, 10)
    u:add_item("log", 5);
    u:add_item("horse", 2);
    u:add_item("money", 1000);
    u:add_item("adamantium", 1);
end

function get_homes(f)
    homes={}
    for u in f.units do
        table.insert(homes, u.region)
    end
    return homes
end

if eressea~=nil then
    eressea.free_game()
    eressea.read_game("game4.dat")
    homes = get_homes(get_faction("xin8"))
else
    -- running in the lua interpreter, not eressea. fake it.
    new_faction = print
    eressea = { ['write_game'] = function(s) print("writing " .. s) end }
    homes = { "Andune", "Bedap", "Curtis", "Dovre" }
end

local f=assert(io.open("factions", "r"))
if f then
    line=f:read("*line")
    players = {}
    emails = {}
    patrons = {}
    nplayers = 0
    while line~=nil do
        fields = {}
        line:gsub("([^\t]*)\t*", function(c) table.insert(fields, c) end)
        line=f:read("*line")
        email = fields[1]
        if fields[2]=='yes' then
            table.insert(patrons, email)
        else
            table.insert(emails, email)
        end
        if fields[3]=='German' then lang='de' else lang='en' end
        race=string.gsub(fields[4], "/.*", ''):lower()
        players[email] = { ['lang'] = lang, ['race'] = race }
        nplayers = nplayers + 1
    end
    f:close()
end
for k, r in ipairs(homes) do
    print(k, r)
end
npatrons = #patrons
print(#homes .. " regions.")
print(nplayers .. " players.")
print(npatrons .. " patrons.")

maxfactions = 20
selected = {}
if maxfactions > nplayers then maxfactions = nplayers end
while maxfactions > 0 do
    if npatrons > 0 then
        email = patrons[npatrons]
        patrons[npatrons] = nil
        npatrons = npatrons - 1
    else
        local np = #emails
        local i = math.random(np)
        email = emails[i]
        emails[i] = emails[np]
        emails[np] = nil
    end
    local player = players[email]
    player.email = email
    table.insert(selected, player)
    maxfactions = maxfactions - 1
end

-- random shuffle
for j=1,#selected do
    k = math.random(j)
    if k ~= j then
        local temp = selected[j]
        selected[j] = selected[k]
        selected[k] = temp
    end
end

print('## players')
for k, player in ipairs(selected) do
    local r = homes[1 + k % #homes]
    new_faction(player.email, player.race, player.lang, r)
    print(player.email)
end
eressea.write_game("game4.dat")
print("## no faction")
for i, email in ipairs(emails) do
    print(email)
end
