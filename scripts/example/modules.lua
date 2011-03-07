require "example.rules"

local srcpath = config.source_dir
tests = {
  srcpath .. '/shared/scripts/tests/common.lua', 
  srcpath .. '/shared/scripts/tests/bson.lua', 
}
