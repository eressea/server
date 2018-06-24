require 'eressea.path'
require 'eressea.resources'
require 'eressea.equipment'
require 'eressea.spells'

local self = {}

function self.update()
    eressea.log.info('spawn dragons')
    spawn_dragons()
    eressea.log.info('spawn undead')
    spawn_undead()

    update_guards()
    update_scores()
end

return self
