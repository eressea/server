require "spells"
require "e3a.xmas2009"
require "e3a.rules"
require "e3a.markets"

local srcpath = config.source_dir
tests = {
  srcpath .. '/eressea/scripts/tests/castles.lua', 
  srcpath .. '/eressea/scripts/tests/morale.lua', 
  srcpath .. '/server/scripts/tests/common.lua', 
  srcpath .. '/eressea/scripts/tests/e3a.lua', 
}
