require "spells"
require "gates"
require "eressea.alp"
require "eressea.eternath"
require "eressea.wedding-jadee" 
require "eressea.ponnuki"
require "eressea.items"
require "eressea.rules"
-- require "eressea.10years"
require "eressea.xmas2004"
require "eressea.xmas2005"
require "eressea.xmas2006"
require "eressea.embassy"
require "eressea.tunnels"
require "eressea.ents"

local srcpath = config.source_dir
tests = {
  srcpath .. '/server/scripts/tests/common.lua', 
  srcpath .. '/eressea/scripts/tests/eressea.lua', 
}
