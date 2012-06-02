require "spells"
require "arda.rules"

local srcpath = config.source_dir
tests = {
  srcpath .. '/core/scripts/tests/spells.lua', 
  srcpath .. '/core/scripts/tests/common.lua', 
  srcpath .. '/core/scripts/tests/bson.lua',
  srcpath .. '/arda/scripts/tests/rules.lua', 
}
