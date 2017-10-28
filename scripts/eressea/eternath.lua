-- DEPRECATED
if not config.eternath or config.eternath==0 then return nil end
-- implements parts of a quest in E2
-- this module is deprecated, because it puts functions in the global environment for at_building_action

local gates = require('eressea.gates')

local b1 = nil
local b2 = nil

function eternathgate_action(b)
  if b1 == nil then
    b1 = b
  elseif b2 == nil then
    b2 = b
  else
    eressea.log.warning("data contains more than two Eternath gates")
  end
  return 1
end

local eternath = {}

function eternath.update()
    if b1 and b2 then
        local size = 5
        local units1 = gates.units(b1, size)
        local units2 = gates.units(b2, size)

        gates.travel(b2, units1)
        gates.travel(b1, units2)
    else
        eressea.log.warning("data contains fewer than two Eternath gates")
    end
end

return eternath
