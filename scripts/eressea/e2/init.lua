require 'eressea.spells'
eressea.log.debug('rules for game E2')

math.randomseed(rng.random())

local equipment = require('eressea.equipment')
local sets = {
    ['seed_faction'] = {
        ['items'] = {
            ['adamantium'] = 1
        }
    },
    ['new_orc'] = {
        ['skills'] = {
            ['polearm'] = 1,
            ['melee'] = 1
        }
    },
    ['spo_seaserpent'] = {
        ['items'] = {
            ['dragonblood'] = 6,
            ['seaserpenthead'] = 1
        }
    },
    ['spo_dragon'] = {
        ['items'] = {
            ['dragonblood'] = 4,
            ['seaserpenthead'] = 1
        }
    },
    ['spo_dragon'] = {
        ['items'] = {
            ['dragonblood'] = 4,
            ['dragonhead'] = 1
        }
    },
    ['spo_youngdragon'] = {
        ['items'] = {
            ['dragonblood'] = 1
        }
    },
    ['spo_wyrm'] = {
        ['items'] = {
            ['dragonblood'] = 10,
            ['dragonhead'] = 1
        }
    },
    ['seed_dragon'] = {
        ['skills'] = {
            ['magic'] = 4,
            ['stealth'] = 1,
            ['stamina'] = 1,
        },
        ['callback'] = function(u)
            u:add_item('money', u.number * (math.random(500)+99))
            u:set_skill('perception', math.random(3))
        end
    },
    ['seed_braineater'] = {
        ['skills'] = {
            ['stealth'] = 1,
            ['perception'] = 1,
        }
    },
    ['seed_seaserpent'] = {
        ['skills'] = {
            ['magic'] = 4,
            ['stealth'] = 2,
            ['stamina'] = 1,
            ['perception'] = 3,
        }
    },
    ['rising_undead'] = {
        ['items'] = {
            ['rustysword'] = 1
        },
        ['callback'] = function(u)
            if (math.random(2)==1) then
                u:add_item('rustychainmail', u.number)
            end
            if (math.random(3)==1) then
                u:add_item('rustyshield', u.number)
            end
        end
    },
    ['new_dracoid'] = {
        ['callback'] = function(u)
            local pick = math.random(3)
            if pick==1 then
                u:set_skill('melee', math.random(4)+2)
                u:add_item('sword', u.number)
            elseif pick==2 then
                u:set_skill('polearm', math.random(4)+2)
                u:add_item('spear', u.number)
            else
                u:set_skill('bow', math.random(3)+1)
                u:add_item('bow', u.number)
            end
        end
    }
}

equipment.add_multiple(sets)

return {
    require('eressea'),
    require('eressea.autoseed'),
    require('eressea.xmas'),
    require('eressea.xmasitems'),
    require('eressea.wedding'),
    require('eressea.embassy'),
    require('eressea.tunnels'),
    require('eressea.ponnuki'),
    require('eressea.astral'),
    require('eressea.jsreport'),
    require('eressea.ents'),
    require('eressea.cursed'),
}
