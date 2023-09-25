-- forward declarations required:
local self = {}

local function equip_first(u)
    name = 'seed_' .. u.race
    equip_unit(u, name, 255)
    local r = u.region
    r.terrain = 'plain'
    r:set_flag(1, false) -- kein mallorn
    r:set_resource('tree', 500)
    r:set_resource('sapling', 100)
    r:set_resource('seed', 50)
    r:set_resource('peasant', 4000)
    r:set_resource('money', 80000)
    r:set_resource('horse', 50)
end

local mysets = {
    ['first_unit'] = {
        ['items'] = {
            ['money'] = 2500,
            ['log'] = 10,
            ['stone'] = 4
        },
        ['callback'] = equip_first
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
        },
    },
    ['seed_insect'] = {
        ['items'] = {
            ['nestwarmth'] = 9
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

EQUIP_SKILLS   = 2
EQUIP_SPELLS  = 4
EQUIP_ITEMS   = 8
EQUIP_SPECIAL = 16
EQUIP_ALL     = 255


function equip_unit(u, name, flags)
    local set = mysets[name]
    if set then
        local items = set['items']
        -- hmmm, lua 5.3 has actual bitwise operators ...
        if items and bit32.band(flags, EQUIP_ITEMS) > 0 then
            for name, count in pairs(items) do
                u:add_item(name, count * u.number)
            end
        end
        local skills = set['skills']
        if skills and bit32.band(flags, EQUIP_SKILLS) > 0 then
            for name, level in pairs(skills) do
                u:set_skill(name, level)
            end
        end
        local spells = set['spells']
        if spells and bit32.band(flags, EQUIP_SPELLS) > 0 then
            for name, level in pairs(spells) do
                u:add_spell(name, level)
            end
        end
        local callback = set['callback']
        if callback and bit32.band(flags, EQUIP_SPECIAL) > 0 and type(callback) == 'function' then
            callback(u, flags)
        end

        u.hp = u.hp_max * u.number
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
