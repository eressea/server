require 'eressea.spells'

eressea.log.debug("rules for game E3")

local equipment = require('eressea.equipment')
local sets = {
    ['spo_seaserpent'] = {
        ['items'] = {
            ['dragonblood'] = 2,
            ['seaserpenthead'] = 1
        }
    }
}
equipment.add_multiple(sets)

return {
    require('eressea'),
    require('eressea.xmasitems'),
    require('eressea.frost'),
    require('eressea.ents')
}
