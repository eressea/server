require 'eressea.spells'
eressea.log.debug('rules for game E2')

local equipment = require('eressea.equipment')
local sets = {
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
