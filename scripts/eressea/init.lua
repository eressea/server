if config.paths ~= nil then
    for path in string.gmatch(config.paths, "([^:]+)") do
        package.path = package.path .. ';' .. path .. '\\?.lua;' .. path .. '\\?\\init.lua'
    end
end
print(package.path)

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
