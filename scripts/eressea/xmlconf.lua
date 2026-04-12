if config.config then
    -- custom config file
    local conffile = config.config
    assert(0 == eressea.config.read(conffile), "could not read JSON data from " .. conffile)
elseif config.rules then
    -- standard ruleset
    rules = 'conf/' .. config.rules
    assert(0 == eressea.config.read(rules .. '/config.json', config.install), "could not read JSON data from " .. rules)
end

eressea.game.reset()
