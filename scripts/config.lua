local path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'
require 'eressea'
require 'eressea.xmlconf' -- read xml data

