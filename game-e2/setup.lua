local srcpath = config.source_dir
local respath = srcpath .. '/eressea/res'
local paths = {
  'eressea/scripts/?.lua',
  'server/scripts/?.lua',
  'external/lunit/?.lua'
}

tests = {
  srcpath .. '/server/scripts/tests/common.lua', 
  srcpath .. '/eressea/scripts/tests/eressea.lua', 
}

for idx, path in pairs(paths) do
  package.path = srcpath .. '/' .. path .. ';' .. package.path
end

read_xml(respath..'/config-eressea.xml', respath..'/catalog-eressea.xml')

require "init"
