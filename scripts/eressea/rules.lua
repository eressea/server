local rules = {}
if config.rules then
    rules = require('eressea.' .. config.rules)
    eressea.log.info('loaded ' .. #rules .. ' modules for ' .. config.rules)
else
    eressea.log.warning('no rule modules loaded, specify a game in eressea.ini or with -r')
end

return rules
