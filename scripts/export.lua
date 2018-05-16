local path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'
require 'eressea'
require 'eressea.xmlconf' -- read xml data

local rules = {}
if config.rules then
    rules = require('eressea.' .. config.rules)
    eressea.log.info('loaded ' .. #rules .. ' modules for ' .. config.rules)
else
    eressea.log.warning('no rule modules loaded, specify a game in eressea.ini or with -r')
end
export_locales()
