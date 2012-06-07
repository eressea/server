local srcpath = config.source_dir
local respath = srcpath .. '/res'
local paths = {
  'scripts/?.lua',
  'eressea/scripts/?.lua',
  'lunit/?.lua'
}

for idx, path in pairs(paths) do
  package.path = srcpath .. '/' .. path .. ';' .. package.path
end

read_xml(respath..'/config-arda.xml', respath..'/catalog-arda.xml')

require "init"
