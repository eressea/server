local rules = 'conf'

if config.rules then
    rules = rules  .. '/' .. config.rules
    assert(0 == eressea.config.read(rules .. '/config.json', config.install), "could not read JSON data from " .. rules)
end

eressea.game.reset()
