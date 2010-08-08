local srcpath = config.source_dir
local respath = srcpath .. '/eressea/res'
local paths = {
  'eressea/scripts/?.lua',
  'server/scripts/?.lua',
  'external/lunit/?.lua'
}

for idx, path in pairs(paths) do
  package.path = srcpath .. '/' .. path .. ';' .. package.path
end

read_xml(respath..'/config-e3a.xml', respath..'/catalog-e3a.xml')

require "init"
