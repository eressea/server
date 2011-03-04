require "example.rules"

local srcpath = config.source_dir
tests = {
  srcpath .. '/shared/scripts/tests/common.lua', 
  srcpath .. '/shared/scripts/tests/bson.lua', 
}

-- variables used in testsuite
test = {  
swordmakeskill = 3, -- weaponsmithing skill for swords
swordiron = 1, -- iron per sword
horsecapacity = 2000, -- carrying capacity of horses (minus weight)
cartcapacity = 10000 -- carrying capacity of carts (minus weight)
}