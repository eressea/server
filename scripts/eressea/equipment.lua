-- forward declaration required:

local self = {}
local mysets = {
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
    },
    ['seed_dwarf'] = {
        ['items'] = {
            ['axe'] = 1,
            ['chainmail'] = 1,
        },
        ['skills'] = {
            ['melee'] = 1
        }
    },
    ['seed_elf'] = {
        ['items'] = {
            ['fairyboot'] = 1,
        },
        ['callback'] = equip_newunits
    },
    ['seed_orc'] = {
        ['skills'] = {
            ['polearm'] = 4,
            ['melee'] = 4,
            ['crossbow'] = 4,
            ['catapult'] = 4,
            ['bow'] = 4,
        }
    },
    ['seed_goblin'] = {
        ['items'] = {
            ['roi'] = 1
        },
        ['callback'] = equip_newunits
    },
    ['seed_human'] = {
        ['callback'] = equip_newunits
    },
    ['seed_troll'] = {
        ['items'] = {
            ['stone'] = 50,
        },
        ['skills'] = {
            ['building'] = 1,
            ['perception'] = 3,
        }
    },
    ['seed_demon'] = {
        ['skills'] = {
            ['stamina'] = 15
        }
    },
    ['seed_insect'] = {
        ['items'] = {
            ['nestwarmth'] =9
        }
    },
    ['seed_halfling'] = {
        ['items'] = {
            ['horse'] = 2,
            ['cart'] = 1,
            ['balm'] = 5,
            ['spice'] = 5,
            ['myrrh'] = 5,
            ['jewel'] = 5,
            ['oil'] = 5,
            ['silk'] = 5,
            ['incense'] = 5
        },
        ['skills'] = {
            ['trade'] = 1,
            ['riding'] = 2
        }
    },
    ['seed_cat'] = {
        ['items'] = {
            ['roi'] = 1
        },
        ['callback'] = equip_newunits
    },
    ['seed_aquarian'] = {
        ['skills'] = {
            ['sailing'] = 1
        },
        ['callback'] = equip_newunits
    },
}

function equip_unit(u, name, flags)
    local set = mysets[name]
    if set then
        local items = set['items']
        if items then
            for name, count in pairs(items) do
                u:add_item(name, count * u.number)
            end
        end
        local skills = set['skills']
        if skills then
            for name, level in pairs(skills) do
                u:set_skill(name, level)
            end
        end
        local spells = set['spells']
        if spells then
            for name, level in ipairs(spells) do
                u:add_spell(name, level)
            end
        end
        local callback = set['callback']
        if callback and type(callback) == 'function' then
            callback(u, flags)
        end
        return true
    end
    return false
end

self.add = function(name, set)
    mysets[name] = set
end

self.add_multiple = function(sets)
    for name, set in pairs(sets) do
        mysets[name] = set
    end
end

self.get = function(name)
    return mysets[name]
end

return self
