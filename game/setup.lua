local srcpath = config.source_dir
local respath = srcpath .. '/example/res'
local paths = {
  'example/scripts/?.lua',
  'shared/scripts/?.lua',
  'external/lunit/?.lua'
}

for idx, path in pairs(paths) do
  package.path = srcpath .. '/' .. path .. ';' .. package.path
end

read_xml(respath..'/config-example.xml', respath..'/catalog-example.xml')

require "init"
