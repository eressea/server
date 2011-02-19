require "spells"
require "e3a.xmas2009"
require "e3a.rules"
require "e3a.markets"

local srcpath = config.source_dir
tests = {
  srcpath .. '/shared/scripts/tests/common.lua', 
  srcpath .. '/shared/scripts/tests/spells.lua', 
  srcpath .. '/shared/scripts/tests/bson.lua', 
  srcpath .. '/eressea/scripts/tests/castles.lua', 
  srcpath .. '/eressea/scripts/tests/morale.lua', 
  srcpath .. '/eressea/scripts/tests/e3a.lua', 
  srcpath .. '/eressea/scripts/tests/stealth.lua', 
}
