require "spells"
require "e3a.xmas2009"
require "e3a.rules"
require "e3a.markets"

local srcpath = config.source_dir
tests = {
  srcpath .. '/core/scripts/tests/common.lua', 
  srcpath .. '/core/scripts/tests/spells.lua', 
--  srcpath .. '/eressea/scripts/tests/bson.lua',
--  srcpath .. '/eressea/scripts/tests/attrib.lua',
  srcpath .. '/scripts/tests/spells.lua', 
  srcpath .. '/scripts/tests/castles.lua', 
  srcpath .. '/scripts/tests/morale.lua', 
  srcpath .. '/scripts/tests/e3a.lua', 
  srcpath .. '/scripts/tests/stealth.lua', 
}
