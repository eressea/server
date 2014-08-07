require 'eressea.path'
require 'eressea.resources'
require 'eressea.spells'

local self = {}

function self.update()
    print('spawn dragons')
    spawn_dragons()
    print('spawn undead')
    spawn_undead()

    update_guards()
    update_scores()
end

return self
