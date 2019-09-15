require 'config'

eressea.read_game('1137.dat')

local dead = {"cwz", "rama"}


local function list_items(f)
    local items = {}
    for u in f.units do
        local r = u.region
        for name in u.items do
            local count = u:get_item(name)
            if not items[r.id] then
                items[r.id] = {}
            end
            if not items[r.id][name] then
                items[r.id][name] = count
            else
                items[r.id][name] = items[r.id][name] + count
            end
        end
    end
    return items
end

gifts = {}
info = {}

for _, no in ipairs(dead) do
    f = get_faction(no)
    gifts[f.id] = list_items(f)
    local allies = {}
    for fno, as in pairs(f.allies) do
        local f2 = get_faction(fno)
        if f2:get_ally(f, 'give') then
            allies[fno] = as
        end
    end
    info[f.id] = {
        ['name'] = f.name,
        ['race'] = f.race,
        ['allies'] = allies
    }
end

eressea.free_game()
eressea.read_game('1138.dat')

newf = {}

for fid, rlist in pairs(gifts) do
    local name = "Erben von " . info[fid].name
    local race = info[fid].race
    local f = faction.create(race, "noreply@eressea.de")
    f.name = name
    f.age = 10
    f.lastturn = 1130
    table.insert(newf, f)
    for rid, items in pairs(rlist) do
        local r = get_region_by_id(rid)
        local u = unit.create(f, r, 1)
        for name, count in pairs(items) do
            u:add_item(name, count)
        end
    end
    for fno, as in pairs(info[fid].allies) do
        local f2 = get_faction(fno)
        for _, s in ipairs(as) do
            f:set_ally(f2, s)
        end
        f2:set_ally(f, "give")
    end
end

eressea.write_game('1138.new.dat')
