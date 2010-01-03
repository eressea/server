local srcpath = config.source_dir
local paths = { 'lunit/?.lua','external/lunit/?.lua','scripts/?.lua';'scripts/?' }

tests = {'common', config.game}
for idx, test in pairs(tests) do
  tests[idx] = srcpath .. '/scripts/tests/' .. test .. '.lua'
end

for idx, path in pairs(paths) do
  package.path = srcpath .. '/' .. path .. ';' .. package.path
end

read_xml(srcpath .. '/res/' .. config.game .. '.xml')

require "config-test"
require "init"
