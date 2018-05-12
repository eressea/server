-- Equipment

local sets = {
    ['first_unit'] = {
        ['items'] = {
            ['money'] = 2500,
            ['log'] = 10,
            ['stone'] = 4
        }
    },
    ['seed_unit'] = {
        ['items'] = {
            ['log'] = 50,
            ['stone'] = 50,
            ['iron'] = 50,
            ['laen'] = 10,
            ['sword'] = 1,
            ['mallorn'] = 10,
            ['skillpotion'] = 5,
            ['lifepotion'] = 5,
            ['money'] = 20000
        },
        ['skills'] = {
            ['perception'] = 30,
            ['melee'] = 1
        }
    }
}

function equip_unit(u, name, flags)
    set = sets[name]
    if set then
        items = set['items']
        if items then
            for k,v in pairs(items) do
                u:add_item(k, v)
            end
        end
        skills = set['skills']
        if skills then
            for k,v in pairs(skills) do
                u:set_skill(k, v)
            end
        end
        return true
    end
    return false
end

return nil
